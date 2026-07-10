#pragma once

#include "stfwd.h"
#include "translate/itranslatorbackend.h"

#include <QWidget>

#include <map>
#include <memory>
#include <vector>

class QTabWidget;

class ITranslatorBackend;
#ifdef ST_HAS_WEBENGINE
class WebTranslatorBackend;
#endif

class Translator : public QWidget
{
  Q_OBJECT
public:
  Translator(Manager &manager, const Settings &settings);
  ~Translator() override;

  void translate(const TaskPtr &task);
  void updateSettings();
  void finish(const TaskPtr &task);

  void onBackendTranslated(const QString &name, const QString &text);
  void onBackendFailed(const QString &name, const QString &error);
  void onBackendLog(const QString &name, const QString &message);

  static QStringList availableTranslators(const QString &path);
  static QStringList availableLanguageNames();

protected:
  void timerEvent(QTimerEvent *event) override;

private:
  struct Tab {
    QWidget *log;
    ITranslatorBackend *backend;
  };

  ITranslatorBackend *findBackend(const QString &name);
#ifdef ST_HAS_WEBENGINE
  std::unique_ptr<WebTranslatorBackend>
  createWebBackend(const QString &scriptName, const QString &scriptText);
  void createWebTab(WebTranslatorBackend *backend);
  void showDebugView();
#else
  void showDebugView() {}
#endif
  void rebuildBackends();
  void processQueue();
  void markTranslated(const TaskPtr &task);
  void createLogTab(ITranslatorBackend *backend);
  void setBusyTab(ITranslatorBackend *backend);

  Manager &manager_;
  const Settings &settings_;
#ifdef ST_HAS_WEBENGINE
  std::unique_ptr<QWebEngineView> debugView_;
  quint16 debugPort_{0};
#endif
  QAction *showDebugAction_;
  QTabWidget *tabs_;
  std::vector<TaskPtr> queue_;
  std::map<QString, std::unique_ptr<ITranslatorBackend>> backends_;
  std::map<QString, Tab> tabsByName_;
};