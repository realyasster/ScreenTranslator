#pragma once

#include "stfwd.h"

#include <QObject>
#include <QRect>

#include <memory>

class QTimer;

class SubtitleMode : public QObject
{
  Q_OBJECT
public:
  SubtitleMode(Manager &manager, const Settings &settings);
  ~SubtitleMode() override;

  bool isActive() const { return active_; }

  void start();
  void stop();

  void onRecognized(const TaskPtr &task);

private:
  void tick();
  void grabAndSubmit();

  Manager &manager_;
  const Settings &settings_;
  std::unique_ptr<class SubtitleSelector> selector_;
  QTimer *timer_{nullptr};
  QRect region_;
  Generation generation_{0};
  QString lastRecognized_;
  QString lastTargetLanguage_;
  bool active_{false};
  bool awaitingSelection_{false};
};