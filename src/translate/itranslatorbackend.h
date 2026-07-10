#pragma once

#include "stfwd.h"

#include <QObject>

#include <chrono>

enum class TranslatorBackendType { Web, Ollama, OpenAI };

class ITranslatorBackend : public QObject
{
  Q_OBJECT
public:
  ~ITranslatorBackend() override = default;

  virtual QString name() const = 0;
  virtual TranslatorBackendType type() const = 0;
  virtual QString displayName() const = 0;

  virtual void setIgnoreSslErrors(bool /*ignore*/) {}
  virtual void setTimeout(std::chrono::seconds /*timeout*/) {}

  virtual void start(TaskPtr task) = 0;
  virtual void cancel() = 0;
  virtual bool isBusy() const = 0;
  virtual TaskPtr task() const = 0;

signals:
  void translated(const QString &text);
  void failed(const QString &error);
  void log(const QString &message);
};