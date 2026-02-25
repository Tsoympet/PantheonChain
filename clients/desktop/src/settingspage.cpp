// ParthenonChain Desktop Wallet - Settings Page Implementation

#include "settingspage.h"

#include "rpc_client.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

SettingsPage::SettingsPage(RPCClient *rpc, QWidget *parent)
    : QWidget(parent), rpcClient(rpc), rpcHostEdit(nullptr), rpcPortEdit(nullptr),
      rpcUserEdit(nullptr), rpcPasswordEdit(nullptr), networkCombo(nullptr),
      autoConnectCheck(nullptr), statusLabel(nullptr), saveButton(nullptr), resetButton(nullptr) {
    setupUI();
    loadSettings();
}

void SettingsPage::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);

    // Title
    QLabel *titleLabel = new QLabel(tr("Settings"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // RPC Connection settings
    QGroupBox *rpcBox = new QGroupBox(tr("RPC Connection"), this);
    QFormLayout *rpcLayout = new QFormLayout(rpcBox);

    rpcHostEdit = new QLineEdit(this);
    rpcHostEdit->setPlaceholderText("127.0.0.1");
    rpcLayout->addRow(tr("Host:"), rpcHostEdit);

    rpcPortEdit = new QLineEdit(this);
    rpcPortEdit->setPlaceholderText("8332");
    rpcLayout->addRow(tr("Port:"), rpcPortEdit);

    rpcUserEdit = new QLineEdit(this);
    rpcUserEdit->setPlaceholderText(tr("RPC username"));
    rpcLayout->addRow(tr("Username:"), rpcUserEdit);

    rpcPasswordEdit = new QLineEdit(this);
    rpcPasswordEdit->setPlaceholderText(tr("RPC password"));
    rpcPasswordEdit->setEchoMode(QLineEdit::Password);
    rpcLayout->addRow(tr("Password:"), rpcPasswordEdit);

    mainLayout->addWidget(rpcBox);

    // Network settings
    QGroupBox *networkBox = new QGroupBox(tr("Network"), this);
    QFormLayout *networkLayout = new QFormLayout(networkBox);

    networkCombo = new QComboBox(this);
    networkCombo->addItems({tr("Mainnet"), tr("Testnet"), tr("Regtest")});
    networkLayout->addRow(tr("Network:"), networkCombo);

    autoConnectCheck = new QCheckBox(tr("Auto-connect on startup"), this);
    networkLayout->addRow(autoConnectCheck);

    mainLayout->addWidget(networkBox);

    // Status label
    statusLabel = new QLabel(this);
    statusLabel->setWordWrap(true);
    mainLayout->addWidget(statusLabel);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    saveButton = new QPushButton(tr("Save Settings"), this);
    saveButton->setMinimumHeight(40);
    resetButton = new QPushButton(tr("Reset to Defaults"), this);
    resetButton->setMinimumHeight(40);

    connect(saveButton, &QPushButton::clicked, this, &SettingsPage::onSaveClicked);
    connect(resetButton, &QPushButton::clicked, this, &SettingsPage::onResetClicked);

    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(resetButton);
    mainLayout->addLayout(buttonLayout);

    mainLayout->addStretch();
}

void SettingsPage::loadSettings() {
    QSettings settings("ParthenonChain", "Wallet");
    rpcHostEdit->setText(settings.value("rpc/host", "127.0.0.1").toString());
    rpcPortEdit->setText(settings.value("rpc/port", "8332").toString());
    rpcUserEdit->setText(settings.value("rpc/user", "").toString());
    const QString network = settings.value("network", "mainnet").toString();
    if (network == "testnet") {
        networkCombo->setCurrentIndex(1);
    } else if (network == "regtest") {
        networkCombo->setCurrentIndex(2);
    } else {
        networkCombo->setCurrentIndex(0);
    }
    autoConnectCheck->setChecked(settings.value("autoConnect", true).toBool());
}

void SettingsPage::onSaveClicked() {
    const QString host = rpcHostEdit->text().trimmed();
    const QString portStr = rpcPortEdit->text().trimmed();

    if (host.isEmpty()) {
        statusLabel->setText(tr("Error: Host cannot be empty"));
        statusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    bool ok = false;
    const int port = portStr.toInt(&ok);
    if (!ok || port <= 0 || port > 65535) {
        statusLabel->setText(tr("Error: Invalid port number"));
        statusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    QSettings settings("ParthenonChain", "Wallet");
    settings.setValue("rpc/host", host);
    settings.setValue("rpc/port", port);
    settings.setValue("rpc/user", rpcUserEdit->text().trimmed());
    const QStringList networks = {"mainnet", "testnet", "regtest"};
    settings.setValue("network", networks.value(networkCombo->currentIndex(), "mainnet"));
    settings.setValue("autoConnect", autoConnectCheck->isChecked());

    if (rpcClient) {
        rpcClient->setCredentials(rpcUserEdit->text().trimmed(), rpcPasswordEdit->text());
        rpcClient->connectToServer(host, port);
    }

    statusLabel->setText(tr("Settings saved successfully"));
    statusLabel->setStyleSheet("QLabel { color: green; }");
    emit settingsSaved();
}

void SettingsPage::onResetClicked() {
    QMessageBox::StandardButton reply =
        QMessageBox::question(this, tr("Reset Settings"),
                              tr("Reset all settings to defaults?"),
                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        rpcHostEdit->setText("127.0.0.1");
        rpcPortEdit->setText("8332");
        rpcUserEdit->clear();
        rpcPasswordEdit->clear();
        networkCombo->setCurrentIndex(0);
        autoConnectCheck->setChecked(true);
        statusLabel->setText(tr("Settings reset to defaults. Click 'Save Settings' to apply."));
        statusLabel->setStyleSheet("QLabel { color: blue; }");
    }
}
