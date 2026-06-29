#include "SettingsDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFormLayout>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <QComboBox>

static const QString CONFIG_KEY_TYPE   = QStringLiteral("apiType");
static const QString CONFIG_KEY_URL    = QStringLiteral("apiUrl");
static const QString CONFIG_KEY_KEY    = QStringLiteral("apiKey");
static const QString CONFIG_KEY_MODEL  = QStringLiteral("modelId");

SettingsDialog::SettingsDialog(const LLMClient::Config &current, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Settings"));
    setMinimumWidth(520);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 16);
    layout->setSpacing(14);

    // Title
    auto *titleLabel = new QLabel(QStringLiteral("API Configuration"));
    QFont tf = titleLabel->font();
    tf.setBold(true);
    tf.setPointSize(tf.pointSize() + 2);
    titleLabel->setFont(tf);
    layout->addWidget(titleLabel);

    // Form
    auto *form = new QFormLayout;
    form->setSpacing(10);

    // API Type combo
    m_apiType = new QComboBox;
    m_apiType->addItem(QStringLiteral("OpenAI-Compatible"), static_cast<int>(LLMClient::OpenAI));
    m_apiType->addItem(QStringLiteral("Anthropic"), static_cast<int>(LLMClient::Anthropic));
    m_apiType->setCurrentIndex(current.apiType == LLMClient::Anthropic ? 1 : 0);
    m_apiType->setObjectName(QStringLiteral("settingsCombo"));
    form->addRow(QStringLiteral("API Type:"), m_apiType);

    m_apiUrl = new QLineEdit(current.apiUrl);
    m_apiUrl->setPlaceholderText(QStringLiteral("https://api.example.com/anthropic"));
    m_apiUrl->setObjectName(QStringLiteral("settingsInput"));
    form->addRow(QStringLiteral("API URL:"), m_apiUrl);

    m_apiKey = new QLineEdit(current.apiKey);
    m_apiKey->setEchoMode(QLineEdit::Password);
    m_apiKey->setPlaceholderText(QStringLiteral("sk-..."));
    m_apiKey->setObjectName(QStringLiteral("settingsInput"));

    auto *keyRow = new QHBoxLayout;
    keyRow->setSpacing(8);
    keyRow->addWidget(m_apiKey);
    auto *showKeyBtn = new QPushButton(QStringLiteral("Show"));
    showKeyBtn->setObjectName(QStringLiteral("settingsBtn"));
    showKeyBtn->setFixedWidth(50);
    showKeyBtn->setCheckable(true);
    connect(showKeyBtn, &QPushButton::toggled, this, [this, showKeyBtn](bool checked) {
        m_apiKey->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        showKeyBtn->setText(checked ? QStringLiteral("Hide") : QStringLiteral("Show"));
    });
    keyRow->addWidget(showKeyBtn);
    form->addRow(QStringLiteral("API Key:"), keyRow);

    m_modelId = new QLineEdit(current.modelId);
    m_modelId->setPlaceholderText(QStringLiteral("gpt-4o / claude-3.5-sonnet"));
    m_modelId->setObjectName(QStringLiteral("settingsInput"));
    form->addRow(QStringLiteral("Model ID:"), m_modelId);

    // Update placeholders based on API type selection
    connect(m_apiType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index == 0) {
            // OpenAI
            m_apiUrl->setPlaceholderText(QStringLiteral("https://api.openai.com/v1"));
            m_modelId->setPlaceholderText(QStringLiteral("gpt-4o"));
        } else {
            // Anthropic
            m_apiUrl->setPlaceholderText(QStringLiteral("https://api.anthropic.com"));
            m_modelId->setPlaceholderText(QStringLiteral("claude-3.5-sonnet"));
        }
    });

    layout->addLayout(form);

    // Status
    m_statusLabel = new QLabel(QStringLiteral("Config saved to: ") + configFilePath());
    m_statusLabel->setObjectName(QStringLiteral("settingsStatus"));
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    // Buttons
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Save"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

LLMClient::Config SettingsDialog::config() const
{
    LLMClient::Config cfg;
    cfg.apiType = static_cast<LLMClient::ApiType>(m_apiType->currentData().toInt());
    cfg.apiUrl  = m_apiUrl->text().trimmed();
    cfg.apiKey  = m_apiKey->text().trimmed();
    cfg.modelId = m_modelId->text().trimmed();
    return cfg;
}

QString SettingsDialog::configFilePath()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/config.json");
}

LLMClient::Config SettingsDialog::loadConfig(const LLMClient::Config &fallback)
{
    QFile f(configFilePath());
    if (!f.open(QIODevice::ReadOnly))
        return fallback;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject())
        return fallback;

    QJsonObject obj = doc.object();
    LLMClient::Config cfg = fallback;
    if (obj.contains(CONFIG_KEY_TYPE))
        cfg.apiType = static_cast<LLMClient::ApiType>(obj.value(CONFIG_KEY_TYPE).toInt());
    if (obj.contains(CONFIG_KEY_URL))
        cfg.apiUrl = obj.value(CONFIG_KEY_URL).toString();
    if (obj.contains(CONFIG_KEY_KEY))
        cfg.apiKey = obj.value(CONFIG_KEY_KEY).toString();
    if (obj.contains(CONFIG_KEY_MODEL))
        cfg.modelId = obj.value(CONFIG_KEY_MODEL).toString();
    return cfg;
}

void SettingsDialog::saveConfig(const LLMClient::Config &cfg)
{
    QJsonObject obj;
    obj[CONFIG_KEY_TYPE]  = static_cast<int>(cfg.apiType);
    obj[CONFIG_KEY_URL]   = cfg.apiUrl;
    obj[CONFIG_KEY_KEY]   = cfg.apiKey;
    obj[CONFIG_KEY_MODEL] = cfg.modelId;

    QJsonDocument doc(obj);
    QFile f(configFilePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(doc.toJson(QJsonDocument::Indented));
        f.close();
    }
}
