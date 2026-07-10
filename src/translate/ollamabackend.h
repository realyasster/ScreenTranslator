#pragma once

#include "httptranslatorbackend.h"

#include <QString>
#include <QStringList>

class OllamaBackend : public HttpTranslatorBackend
{
  Q_OBJECT
public:
  OllamaBackend(QString model, QString baseUrl = QStringLiteral("http://localhost:11434"));

  static QStringList discoverModels(const QString &baseUrl);

  void setBaseUrl(const QString &url) { baseUrl_ = url; }
  void setModel(const QString &model) { model_ = model; }
  QString model() const { return model_; }
  QString baseUrl() const { return baseUrl_; }

protected:
  PreparedRequest buildRequest(const QString &text, const QString &from,
                               const QString &to) override;
  bool parseResponse(const QByteArray &data, QString &translated,
                     QString &error) override;

private:
  QString baseUrl_;
  QString model_;
};