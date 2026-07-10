#pragma once

#include "itranslatorbackend.h"
#include "webpage.h"

#include <QString>

class Translator;

class WebTranslatorBackend : public ITranslatorBackend
{
  Q_OBJECT
public:
  WebTranslatorBackend(Translator &translator, QString scriptName,
                       QString scriptText);

  QString name() const override { return scriptName_; }
  TranslatorBackendType type() const override
  {
    return TranslatorBackendType::Web;
  }
  QString displayName() const override { return scriptName_; }
  QString scriptText() const { return scriptText_; }

  void setIgnoreSslErrors(bool ignore) override;
  void setTimeout(std::chrono::seconds timeout) override;

  void start(TaskPtr task) override;
  void cancel() override;
  bool isBusy() const override { return page_.isBusy(); }
  TaskPtr task() const override { return page_.task(); }

  WebPage *page() { return &page_; }

private:
  QString scriptName_;
  QString scriptText_;
  WebPage page_;
};