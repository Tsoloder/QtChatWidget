#pragma once

#include <QDialog>
#include "LLMClient.h"

class QLineEdit;
class QLabel;
class QComboBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(const LLMClient::Config &current, QWidget *parent = nullptr);

    LLMClient::Config config() const;

    // config.json persistence
    static LLMClient::Config loadConfig(const LLMClient::Config &fallback);
    static void saveConfig(const LLMClient::Config &cfg);
    static QString configFilePath();

private:
    QComboBox *m_apiType;
    QLineEdit *m_apiUrl;
    QLineEdit *m_apiKey;
    QLineEdit *m_modelId;
    QLabel *m_statusLabel;
};
