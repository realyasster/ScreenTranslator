#include "ollamabackend.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

OllamaBackend::OllamaBackend(QString model, QString baseUrl)
  : HttpTranslatorBackend(TranslatorBackendType::Ollama,
                          QStringLiteral("ollama:") + model,
                          QStringLiteral("Ollama — ") + model)
  , baseUrl_(std::move(baseUrl))
  , model_(std::move(model))
{
}

QStringList OllamaBackend::discoverModels(const QString &baseUrl)
{
  QStringList result;
  QNetworkAccessManager nam;
  QNetworkRequest req(QUrl(baseUrl + QStringLiteral("/api/tags")));
  auto *reply = nam.get(req);
  QObject::connect(reply, &QNetworkReply::finished, [&]() {
    if (reply->error() == QNetworkReply::NoError) {
      const auto data = reply->readAll();
      const auto json = QJsonDocument::fromJson(data).object();
      const auto models = json.value(QStringLiteral("models")).toArray();
      for (const auto &m : models) {
        const auto name = m.toObject().value(QStringLiteral("name")).toString();
        if (!name.isEmpty())
          result << name;
      }
    }
    reply->deleteLater();
  });
  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();
  return result;
}

HttpTranslatorBackend::PreparedRequest
OllamaBackend::buildRequest(const QString &text, const QString &from,
                            const QString &to)
{
  PreparedRequest out;
  out.req = QNetworkRequest(QUrl(baseUrl_ + QStringLiteral("/api/generate")));
  out.req.setHeader(QNetworkRequest::ContentTypeHeader,
                    QStringLiteral("application/json"));

  QJsonObject body;
  body.insert(QStringLiteral("model"), model_);
  body.insert(QStringLiteral("prompt"), buildPrompt(text, from, to));
  body.insert(QStringLiteral("stream"), false);
  QJsonObject options;
  options.insert(QStringLiteral("temperature"), 0.2);
  body.insert(QStringLiteral("options"), options);

  out.body = QJsonDocument(body).toJson(QJsonDocument::Compact);
  return out;
}

bool OllamaBackend::parseResponse(const QByteArray &data, QString &translated,
                                  QString &error)
{
  const auto json = QJsonDocument::fromJson(data).object();
  if (json.contains(QStringLiteral("error"))) {
    error = json.value(QStringLiteral("error")).toString();
    return false;
  }
  translated = json.value(QStringLiteral("response")).toString();
  if (translated.isEmpty()) {
    error = QStringLiteral("empty response");
    return false;
  }
  return true;
}