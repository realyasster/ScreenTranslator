#pragma once

#include "httptranslatorbackend.h"

#include <QString>

class OpenAIBackend : public HttpTranslatorBackend
{
  Q_OBJECT
public:
  OpenAIBackend(QString model, QString endpoint = QStringLiteral("https://api.openai.com"),
                QString apiKey = QString());

  void setEndpoint(const QString &e) { endpoint_ = e; }
  void setApiKey(const QString &k) { apiKey_ = k; }
  void setModel(const QString &m) { model_ = m; }
  QString model() const { return model_; }
  QString endpoint() const { return endpoint_; }

protected:
  PreparedRequest buildRequest(const QString &text, const QString &from,
                               const QString &to) override;
  bool parseResponse(const QByteArray &data, QString &translated,
                     QString &error) override;

private:
  QString endpoint_;
  QString apiKey_;
  QString model_;
};