#include "webtranslatorbackend.h"
#include "translator.h"

WebTranslatorBackend::WebTranslatorBackend(Translator &translator,
                                           QString scriptName,
                                           QString scriptText)
  : scriptName_(std::move(scriptName))
  , scriptText_(std::move(scriptText))
  , page_(translator, scriptText_, scriptName_)
{
  connect(&page_, &WebPage::log, this, &ITranslatorBackend::log);
  connect(&page_, &WebPage::translated, this, &ITranslatorBackend::translated);
  connect(&page_, &WebPage::failed, this, &ITranslatorBackend::failed);
}

void WebTranslatorBackend::setIgnoreSslErrors(bool ignore)
{
  page_.setIgnoreSslErrors(ignore);
}

void WebTranslatorBackend::setTimeout(std::chrono::seconds timeout)
{
  page_.setTimeout(timeout);
}

void WebTranslatorBackend::start(TaskPtr task)
{
  page_.start(task);
}

void WebTranslatorBackend::cancel()
{
  page_.setFailed(QStringLiteral("canceled"));
}