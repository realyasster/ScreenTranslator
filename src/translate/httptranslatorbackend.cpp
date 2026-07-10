#include "httptranslatorbackend.h"
#include "task.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSslConfiguration>

namespace
{
constexpr const char *kSystemPrompt =
    "You are a professional translator. Translate the user's text from "
    "{FROM} to {TO}. Output ONLY the translation, with no explanations, "
    "no quotes, and no source text. Preserve formatting, line breaks, and "
    "punctuation.";

QString substituteLanguages(QString promptTemplate,
                            const QString &from, const QString &to)
{
  const auto fromName = LanguageCodes::name(from);
  const auto toName = LanguageCodes::name(to);
  return promptTemplate
      .replace(QStringLiteral("{FROM}"),
               fromName.isEmpty() ? from : fromName)
      .replace(QStringLiteral("{TO}"), toName.isEmpty() ? to : toName);
}

QString cleanResult(QString text)
{
  text = text.trimmed();
  static const QRegularExpression codeFenceStart(
      QStringLiteral("^```[a-zA-Z]*\\s*"));
  text.remove(codeFenceStart);
  if (text.endsWith(QLatin1String("```"))) {
    text.chop(3);
    text = text.trimmed();
  }
  if (text.startsWith(QLatin1String("\"")) && text.endsWith(QLatin1String("\"")))
    text = text.mid(1, text.size() - 2).trimmed();
  return text;
}
}  // namespace

HttpTranslatorBackend::HttpTranslatorBackend(TranslatorBackendType type,
                                             QString name, QString displayName)
  : type_(type)
  , name_(std::move(name))
  , displayName_(std::move(displayName))
{
}

QString HttpTranslatorBackend::buildPrompt(const QString &text,
                                           const QString &from,
                                           const QString &to)
{
  const auto instruction =
      substituteLanguages(QString::fromLatin1(kSystemPrompt), from, to);
  return instruction + QStringLiteral("\n\n") + text;
}

void HttpTranslatorBackend::start(TaskPtr task)
{
  if (!task)
    return;
  task_ = task;
  busy_ = true;

  const auto from = LanguageCodes::iso639_1(task->sourceLanguage);
  const auto to = LanguageCodes::iso639_1(task->targetLanguage);
  if (from.isEmpty() || to.isEmpty()) {
    busy_ = false;
    task_.reset();
    emit failed(tr("unknown translation languages"));
    return;
  }

  sendRequest(task, task->corrected, from, to);
}

void HttpTranslatorBackend::cancel()
{
  if (reply_) {
    reply_->abort();
    reply_ = nullptr;
  }
  busy_ = false;
  task_.reset();
}

void HttpTranslatorBackend::sendRequest(TaskPtr task, const QString &text,
                                        const QString &from,
                                        const QString &to)
{
  Q_UNUSED(task);
  if (reply_) {
    reply_->abort();
    reply_ = nullptr;
  }

  const auto prepared = buildRequest(text, from, to);
  reply_ = nam_.post(prepared.req, prepared.body);
  if (ignoreSslErrors_)
    reply_->ignoreSslErrors();

  connect(reply_, &QNetworkReply::finished, this, [this]() {
    const auto reply = reply_;
    reply_ = nullptr;
    if (!reply)
      return;

    if (reply->error() != QNetworkReply::NoError) {
      busy_ = false;
      const auto err = reply->errorString();
      task_.reset();
      emit failed(err);
      reply->deleteLater();
      return;
    }

    const auto data = reply->readAll();
    reply->deleteLater();

    QString resultText;
    QString error;
    if (!parseResponse(data, resultText, error)) {
      busy_ = false;
      task_.reset();
      emit failed(error);
      return;
    }

    busy_ = false;
    task_.reset();
    emit translated(cleanResult(resultText));
  });
}