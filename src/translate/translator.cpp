#include "translator.h"

#include "debug.h"
#include "languagecodes.h"
#include "manager.h"
#include "ollamabackend.h"
#include "openaibackend.h"
#include "settings.h"
#include "task.h"
#include "webtranslatorbackend.h"
#include "widgetstate.h"

#include <QBoxLayout>
#include <QCloseEvent>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QSplitter>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>

#include <unordered_set>

namespace
{
std::map<QString, QString> loadScripts(const QString &dir,
                                       const QStringList &scriptNames)
{
  std::map<QString, QString> result;
  for (const auto &name : scriptNames) {
    QFile f(dir + QLatin1Char('/') + name);
    if (!f.open(QFile::ReadOnly))
      continue;
    const auto script = QString::fromUtf8(f.readAll());
    if (!script.isEmpty())
      result.emplace(name, script);
  }
  return result;
}

bool isWebScript(const QString &spec)
{
  return spec.endsWith(QStringLiteral(".js"), Qt::CaseInsensitive);
}

bool isOllamaSpec(const QString &spec)
{
  return spec.startsWith(QStringLiteral("ollama:"), Qt::CaseInsensitive);
}

bool isOpenAISpec(const QString &spec)
{
  return spec.startsWith(QStringLiteral("openai:"), Qt::CaseInsensitive);
}
}  // namespace

Translator::Translator(Manager &manager, const Settings &settings)
  : manager_(manager)
  , settings_(settings)
  , showDebugAction_(new QAction(QIcon(":/icons/debug.png"), tr("Debug"), this))
  , tabs_(new QTabWidget(this))
{
  {
    QTcpSocket socket;
    if (socket.bind()) {
      debugPort_ = socket.localPort();
      qputenv("QTWEBENGINE_REMOTE_DEBUGGING",
              QString::number(debugPort_).toUtf8());
      socket.close();
    }
  }

  setObjectName("Translator");
  setWindowTitle(tr("Translator"));

  auto toolBar = new QToolBar(this);
  toolBar->addAction(showDebugAction_);

  auto layout = new QVBoxLayout(this);
  layout->addWidget(toolBar);
  layout->addWidget(tabs_);

  startTimer(1000);

  connect(showDebugAction_, &QAction::triggered,  //
          this, &Translator::showDebugView);

  new service::WidgetState(this);
}

Translator::~Translator() = default;

void Translator::translate(const TaskPtr &task)
{
  SOFT_ASSERT(task, return );

  if (task->corrected.isEmpty()) {
    LTRACE() << "Corrected text is empty. Skipping translation";
    manager_.translated(task);
    return;
  }

  queue_.push_back(task);
  processQueue();
}

void Translator::updateSettings()
{
  tabs_->blockSignals(true);
  for (auto i = 0, end = tabs_->count(); i < end; ++i) {
    auto tab = tabs_->widget(0);
    tabs_->removeTab(0);
    tab->deleteLater();
  }
  tabs_->blockSignals(false);

  backends_.clear();
  tabsByName_.clear();

  rebuildBackends();
}

void Translator::rebuildBackends()
{
  if (settings_.translators.empty())
    return;

  QStringList webScripts;
  for (const auto &spec : settings_.translators) {
    if (isWebScript(spec))
      webScripts << spec;
  }

  const auto loaded = loadScripts(settings_.translatorsPath, webScripts);
  if (webScripts.isEmpty() && settings_.translators.isEmpty()) {
    return;
  }
  if (!webScripts.isEmpty() && loaded.empty()) {
    manager_.fatalError(
        tr("No translators loaded from\n%1\n(%2)")
            .arg(settings_.translatorsPath, webScripts.join(", ")));
    return;
  }

  for (const auto &spec : settings_.translators) {
    if (isWebScript(spec)) {
      const auto it = loaded.find(spec);
      if (it == loaded.end())
        continue;
      auto backend = createWebBackend(spec, it->second);
      WebTranslatorBackend *raw = backend.get();
      backends_.emplace(spec, std::move(backend));
      createWebTab(raw);
    } else if (isOllamaSpec(spec)) {
      const auto model = spec.mid(7);
      auto backend = std::make_unique<OllamaBackend>(
          model, settings_.ollamaUrl.isEmpty()
                     ? QStringLiteral("http://localhost:11434")
                     : settings_.ollamaUrl);
      auto *raw = backend.get();
      backends_.emplace(spec, std::move(backend));
      createLogTab(raw);
    } else if (isOpenAISpec(spec)) {
      const auto model = spec.mid(7);
      auto backend = std::make_unique<OpenAIBackend>(
          model,
          settings_.openaiEndpoint.isEmpty()
              ? QStringLiteral("https://api.openai.com")
              : settings_.openaiEndpoint,
          settings_.openaiKey);
      auto *raw = backend.get();
      backends_.emplace(spec, std::move(backend));
      createLogTab(raw);
    }
  }
}

std::unique_ptr<WebTranslatorBackend>
Translator::createWebBackend(const QString &scriptName,
                             const QString &scriptText)
{
  auto backend =
      std::make_unique<WebTranslatorBackend>(*this, scriptName, scriptText);
  auto *raw = backend.get();

  raw->setIgnoreSslErrors(settings_.ignoreSslErrors);
  raw->setTimeout(settings_.translationTimeout);

  connect(raw, &ITranslatorBackend::log, this,
          [this, name = scriptName](const QString &m) {
            onBackendLog(name, m);
          });
  connect(raw, &ITranslatorBackend::translated, this,
          [this, name = scriptName](const QString &t) {
            onBackendTranslated(name, t);
          });
  connect(raw, &ITranslatorBackend::failed, this,
          [this, name = scriptName](const QString &e) {
            onBackendFailed(name, e);
          });

  return backend;
}

void Translator::createWebTab(WebTranslatorBackend *backend)
{
  auto *page = backend->page();
  page->setIgnoreSslErrors(settings_.ignoreSslErrors);
  page->setTimeout(settings_.translationTimeout);
  page->setVisible(true);

  auto log = new QTextEdit(tabs_);
  tabs_->addTab(log, backend->name());
  log->document()->setMaximumBlockCount(1000);

  connect(page, &WebPage::visibleChanged, page, [page](bool on) {
    if (!on)
      page->setVisible(true);
  });
  connect(page, &WebPage::log, log, &QTextEdit::append);

  tabsByName_.emplace(backend->name(), Tab{log, backend});
}

void Translator::createLogTab(ITranslatorBackend *backend)
{
  auto log = new QTextEdit(tabs_);
  tabs_->addTab(log, backend->displayName());
  log->document()->setMaximumBlockCount(1000);
  tabsByName_.emplace(backend->name(), Tab{log, backend});

  connect(backend, &ITranslatorBackend::log, log, &QTextEdit::append);
}

void Translator::setBusyTab(ITranslatorBackend *backend)
{
  const auto it = tabsByName_.find(backend->name());
  if (it == tabsByName_.end())
    return;
  const auto idx = tabs_->indexOf(it->second.log);
  if (idx >= 0)
    tabs_->setCurrentIndex(idx);
}

void Translator::showDebugView()
{
  if (!debugView_)
    debugView_ = std::make_unique<QWebEngineView>();
  debugView_->load(
      QUrl::fromUserInput("http://localhost:" + QString::number(debugPort_)));
  debugView_->show();
  debugView_->activateWindow();
}

ITranslatorBackend *Translator::findBackend(const QString &name)
{
  const auto it = backends_.find(name);
  return it != backends_.end() ? it->second.get() : nullptr;
}

void Translator::onBackendTranslated(const QString &name, const QString &text)
{
  auto *backend = findBackend(name);
  if (!backend || !backend->isBusy())
    return;
  auto task = backend->task();
  if (!task)
    return;
  task->translated = text;
  task->usedTranslator = name;
  finish(task);
}

void Translator::onBackendFailed(const QString &name, const QString &error)
{
  auto *backend = findBackend(name);
  if (!backend)
    return;
  auto task = backend->task();
  if (task)
    task->translatorErrors.append(QStringLiteral("%1: %2").arg(name, error));
  backend->cancel();
}

void Translator::onBackendLog(const QString & /*name*/, const QString &message)
{
  LTRACE() << "translator:" << message;
}

void Translator::processQueue()
{
  if (queue_.empty())
    return;

  std::unordered_set<QString> idleNames;
  for (const auto &[name, backend] : backends_) {
    if (!backend->isBusy())
      idleNames.insert(name);
  }

  if (idleNames.empty())
    return;

  std::vector<TaskPtr> finishedTasks;
  for (const auto &task : queue_) {
    if (idleNames.empty())
      break;

    if (task->translators.isEmpty()) {
      task->error = tr("All translators failed\n%1")
                        .arg(task->translatorErrors.join(QStringLiteral("\n")));
      finishedTasks.push_back(task);
      continue;
    }

    for (const auto &spec : task->translators) {
      if (!idleNames.count(spec))
        continue;
      auto *backend = findBackend(spec);
      if (!backend)
        continue;
      backend->start(task);
      task->translators.removeOne(spec);
      idleNames.erase(spec);
      setBusyTab(backend);
      LTRACE() << "Started translation at" << spec << task;
      break;
    }
  }

  if (!finishedTasks.empty()) {
    for (const auto &task : finishedTasks) markTranslated(task);
  }
}

void Translator::markTranslated(const TaskPtr &task)
{
  manager_.translated(task);
  queue_.erase(std::remove(queue_.begin(), queue_.end(), task),
               queue_.end());
}

void Translator::finish(const TaskPtr &task)
{
  markTranslated(task);
  processQueue();
}

QStringList Translator::availableTranslators(const QString &path)
{
  if (path.isEmpty())
    return {};

  QDir dir(path);
  if (!dir.exists())
    return {};

  const auto names = dir.entryList({"*.js"}, QDir::Files);
  return names;
}

QStringList Translator::availableLanguageNames()
{
  QStringList names;

  for (const auto &id : LanguageCodes::allIds()) {
    const auto iso = LanguageCodes::iso639_1(id);
    if (!iso.isEmpty())
      names.append(LanguageCodes::name(id));
  }

  return names;
}

void Translator::timerEvent(QTimerEvent * /*event*/)
{
  processQueue();
}