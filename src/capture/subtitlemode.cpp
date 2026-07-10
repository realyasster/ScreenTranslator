#include "subtitlemode.h"

#include "debug.h"
#include "manager.h"
#include "settings.h"
#include "task.h"

#include <QApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScreen>
#include <QTimer>
#include <QWidget>

#include <functional>

namespace
{
constexpr int kDefaultIntervalMs = 500;
}

class SubtitleSelector : public QWidget
{
public:
  using SelectedCallback = std::function<void(const QRect &)>;
  using CanceledCallback = std::function<void()>;

  explicit SubtitleSelector(QWidget *parent = nullptr)
    : QWidget(parent)
  {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                   | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setCursor(Qt::CrossCursor);
  }

  void activate(const QRect &fullBounds)
  {
    bounds_ = fullBounds;
    setGeometry(fullBounds);
    selected_ = {};
    selecting_ = true;
    show();
    raise();
    grabMouse();
    grabKeyboard();
    setMouseTracking(true);
  }

  void deactivate()
  {
    selecting_ = false;
    releaseMouse();
    releaseKeyboard();
    hide();
  }

  bool isSelecting() const { return selecting_; }
  QRect selectedRect() const { return selected_; }

  void setOnSelected(SelectedCallback cb) { onSelected_ = std::move(cb); }
  void setOnCanceled(CanceledCallback cb) { onCanceled_ = std::move(cb); }

protected:
  void paintEvent(QPaintEvent * /*event*/) override
  {
    if (!selecting_)
      return;
    QPainter p(this);
    p.fillRect(bounds_, QColor(0, 0, 0, 100));
    if (!selected_.isNull()) {
      p.fillRect(selected_, Qt::transparent);
      p.setPen(QPen(Qt::red, 2));
      p.drawRect(selected_);
    }
  }

  void mousePressEvent(QMouseEvent *event) override
  {
    if (event->button() == Qt::LeftButton) {
      origin_ = event->pos();
      selected_ = {};
      update();
    }
  }

  void mouseMoveEvent(QMouseEvent *event) override
  {
    if (!(event->buttons() & Qt::LeftButton))
      return;
    selected_ = QRect(origin_, event->pos()).normalized();
    update();
  }

  void keyPressEvent(QKeyEvent *event) override
  {
    if (event->key() == Qt::Key_Escape) {
      selected_ = {};
      selecting_ = false;
      if (onCanceled_)
        onCanceled_();
      deactivate();
    }
  }

  void mouseReleaseEvent(QMouseEvent *event) override
  {
    if (event->button() != Qt::LeftButton)
      return;
    selected_ = QRect(origin_, event->pos()).normalized();
    if (selected_.width() > 4 && selected_.height() > 4) {
      if (onSelected_)
        onSelected_(selected_);
    }
    deactivate();
  }

private:
  QRect bounds_;
  QRect selected_;
  QPoint origin_;
  bool selecting_{false};
  SelectedCallback onSelected_;
  CanceledCallback onCanceled_;
};

SubtitleMode::SubtitleMode(Manager &manager, const Settings &settings)
  : manager_(manager)
  , settings_(settings)
  , selector_(std::make_unique<SubtitleSelector>())
  , timer_(new QTimer(this))
{
  timer_->setSingleShot(false);
  connect(timer_, &QTimer::timeout, this, &SubtitleMode::tick);
  selector_->setOnSelected([this](const QRect &rect) {
    region_ = rect;
    awaitingSelection_ = false;
    active_ = true;
    lastRecognized_.clear();
    timer_->start(kDefaultIntervalMs);
    tick();
  });
  selector_->setOnCanceled([this]() {
    awaitingSelection_ = false;
    active_ = false;
  });
}

SubtitleMode::~SubtitleMode()
{
  stop();
}

void SubtitleMode::start()
{
  if (active_ || awaitingSelection_)
    return;

  QRect bounds;
  for (auto *screen : QApplication::screens())
    bounds |= screen->geometry();

  awaitingSelection_ = true;
  selector_->activate(bounds);
}

void SubtitleMode::stop()
{
  timer_->stop();
  if (selector_->isSelecting())
    selector_->deactivate();
  awaitingSelection_ = false;
  active_ = false;
  lastRecognized_.clear();
}

void SubtitleMode::tick()
{
  if (!active_ || region_.isNull())
    return;
  grabAndSubmit();
}

void SubtitleMode::grabAndSubmit()
{
  QPixmap pixmap(region_.size());
  pixmap.fill(Qt::black);

  auto *screen = QApplication::screenAt(region_.center());
  if (!screen)
    screen = QApplication::primaryScreen();
  if (!screen)
    return;

  const auto screenGeom = screen->geometry();
  const auto grabRect = region_.intersected(screenGeom);
  if (grabRect.isNull())
    return;

  const auto localRect = grabRect.translated(-screenGeom.topLeft());
  const auto grabbed =
      screen->grabWindow(0, localRect.x(), localRect.y(), localRect.width(),
                         localRect.height());
  if (grabbed.isNull())
    return;

  QPainter p(&pixmap);
  const auto dx = grabRect.x() - region_.x();
  const auto dy = grabRect.y() - region_.y();
  p.drawPixmap(dx, dy, grabbed);

  auto task = std::make_shared<Task>();
  task->captured = pixmap;
  task->capturePoint = region_.topLeft();
  task->sourceLanguage = settings_.sourceLanguage;
  task->targetLanguage = settings_.targetLanguage;
  task->translators = settings_.translators;
  task->useHunspell = settings_.useHunspell;
  task->generation = ++generation_;
  lastTargetLanguage_ = task->targetLanguage;

  manager_.captured(task);
}

void SubtitleMode::onRecognized(const TaskPtr &task)
{
  if (!active_ || !task)
    return;
  if (task->generation != generation_)
    return;

  if (task->recognized == lastRecognized_)
    return;

  lastRecognized_ = task->recognized;
}