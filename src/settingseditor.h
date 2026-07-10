#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

#include "commonmodels.h"
#include "settings.h"

namespace Ui
{
class SettingsEditor;
}
class QAbstractButton;
class QStandardItemModel;

class SettingsEditor : public QDialog
{
  Q_OBJECT

public:
  SettingsEditor(Manager &manager, update::Updater &updater);
  ~SettingsEditor();

  Settings settings() const;
  void setSettings(const Settings &settings);

private:
  void handleButtonBoxClicked(QAbstractButton *button);
  void pickColor(QWidget *widget);
  void updateResultFont();
  QStringList enabledTranslators() const;

  void updateState();
  void updateCurrentPage();
  void updateTranslators(const QStringList &translators);
  void updateModels();
  void validateSettings();

  void setupAiTranslation();
  void refreshOllamaModels();
  void addOllamaToList(const QString &model);
  void addOpenAIToList(const QString &model);

  Ui::SettingsEditor *ui;
  QLineEdit *ollamaUrlEdit_{nullptr};
  QComboBox *ollamaModelCombo_{nullptr};
  QPushButton *ollamaRefreshBtn_{nullptr};
  QLineEdit *openaiEndpointEdit_{nullptr};
  QLineEdit *openaiModelEdit_{nullptr};
  QLineEdit *openaiKeyEdit_{nullptr};
  QCheckBox *openaiSaveKeyCheck_{nullptr};
  QPushButton *openaiAddBtn_{nullptr};

  Manager &manager_;
  update::Updater &updater_;
  CommonModels models_;
  bool wasPortable_{false};
  QStandardItemModel *pageModel_;
};
