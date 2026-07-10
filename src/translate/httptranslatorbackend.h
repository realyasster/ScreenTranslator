#pragma once

#include "itranslatorbackend.h"
#include "languagecodes.h"

#include <QNetworkAccessManager>
#include <QString>

class Task;

class QNetworkReply;

class HttpTranslatorBackend : public ITranslatorBackend
{
  Q_OBJECT
public:
  HttpTranslatorBackend(TranslatorBackendType type, QString name,
                         QString displayName);

  QString name() const override { return name_; }
  TranslatorBackendType type() const override { return type_; }
  QString displayName() const override { return displayName_; }

  void setIgnoreSslErrors(bool ignore) override
  {
    ignoreSslErrors_ = ignore;
  }
  void setTimeout(std::chrono::seconds timeout) override
  {
    timeout_ = timeout;
    nam_.setTransferTimeout(int(timeout.count()) * 1000);
  }

  void start(TaskPtr task) override;
  void cancel() override;
  bool isBusy() const override { return busy_; }
  TaskPtr task() const override { return task_; }

protected:
  struct PreparedRequest {
    QNetworkRequest req;
    QByteArray body;
  };
  virtual PreparedRequest buildRequest(const QString &text,
                                        const QString &from,
                                        const QString &to) = 0;
  virtual bool parseResponse(const QByteArray &data, QString &translated,
                             QString &error) = 0;

  static QString buildPrompt(const QString &text, const QString &from,
                             const QString &to);

private:
  TranslatorBackendType type_;
  QString name_;
  QString displayName_;
  QNetworkAccessManager nam_;
  QNetworkReply *reply_{nullptr};
  TaskPtr task_;
  bool busy_{false};
  bool ignoreSslErrors_{false};
  std::chrono::seconds timeout_{30};

  void sendRequest(TaskPtr task, const QString &text, const QString &from,
                   const QString &to);
};