#include "openaibackend.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

OpenAIBackend::OpenAIBackend(QString model, QString endpoint, QString apiKey)
  : HttpTranslatorBackend(TranslatorBackendType::OpenAI,
                          QStringLiteral("openai:") + model,
                          QStringLiteral("OpenAI — ") + model)
  , endpoint_(std::move(endpoint))
  , apiKey_(std::move(apiKey))
  , model_(std::move(model))
{
}

HttpTranslatorBackend::PreparedRequest
OpenAIBackend::buildRequest(const QString &text, const QString &from,
                            const QString &to)
{
  PreparedRequest out;
  out.req = QNetworkRequest(
      QUrl(endpoint_ + QStringLiteral("/v1/chat/completions")));
  out.req.setHeader(QNetworkRequest::ContentTypeHeader,
                    QStringLiteral("application/json"));
  if (!apiKey_.isEmpty())
    out.req.setRawHeader(QByteArrayLiteral("Authorization"),
                         QByteArrayLiteral("Bearer ") + apiKey_.toUtf8());

  QJsonObject userMsg;
  userMsg.insert(QStringLiteral("role"), QStringLiteral("user"));
  userMsg.insert(QStringLiteral("content"), text);

  QJsonObject systemMsg;
  systemMsg.insert(QStringLiteral("role"), QStringLiteral("system"));
  systemMsg.insert(QStringLiteral("content"),
                   QStringLiteral("You translate from ") + from +
                       QStringLiteral(" to ") + to +
                       QStringLiteral(". Output only the translation."));

  QJsonArray messages;
  messages.append(systemMsg);
  messages.append(userMsg);

  QJsonObject body;
  body.insert(QStringLiteral("model"), model_);
  body.insert(QStringLiteral("messages"), messages);
  body.insert(QStringLiteral("temperature"), 0.2);
  body.insert(QStringLiteral("stream"), false);

  out.body = QJsonDocument(body).toJson(QJsonDocument::Compact);
  return out;
}

bool OpenAIBackend::parseResponse(const QByteArray &data, QString &translated,
                                  QString &error)
{
  const auto json = QJsonDocument::fromJson(data).object();
  if (json.contains(QStringLiteral("error"))) {
    const auto errObj = json.value(QStringLiteral("error")).toObject();
    error = errObj.value(QStringLiteral("message")).toString();
    if (error.isEmpty())
      error = json.value(QStringLiteral("error")).toString();
    return false;
  }
  const auto choices = json.value(QStringLiteral("choices")).toArray();
  if (choices.isEmpty()) {
    error = QStringLiteral("no choices in response");
    return false;
  }
  const auto content =
      choices.first().toObject().value(QStringLiteral("message")).toObject().value(
          QStringLiteral("content"));
  translated = content.toString();
  if (translated.isEmpty()) {
    error = QStringLiteral("empty content in response");
    return false;
  }
  return true;
}