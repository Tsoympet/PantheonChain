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

// Maps combo index → NetworkType
static NetworkType indexToNetwork(int i) {
    switch (i) {
    case 1:
        return NetworkType::Testnet;
    case 2:
        return NetworkType::Devnet;
    default:
        return NetworkType::Mainnet;
    }
}
static int networkToIndex(NetworkType t) {
    switch (t) {
    case NetworkType::Testnet:
        return 1;
    case NetworkType::Devnet:
        return 2;
    default:
        return 0;
    }
}

SettingsPage::SettingsPage(RPCClient *rpc, QWidget *parent)
    : QWidget(parent), rpcClient(rpc), rpcHostEdit(nullptr), rpcPortEdit(nullptr),
      rpcUserEdit(nullptr), rpcPasswordEdit(nullptr), networkCombo(nullptr),
      networkStatusBadge(nullptr), peerCountLabel(nullptr), latencyLabel(nullptr),
      nodeVersionLabel(nullptr), devNetGateWidget(nullptr), devNetAddressEdit(nullptr),
      verifyDevNetButton(nullptr), devNetStatusLabel(nullptr), autoConnectCheck(nullptr),
      statusLabel(nullptr), saveButton(nullptr), resetButton(nullptr), devNetVerified(false),
      pendingNetworkIndex(0) {
    setupUI();
    loadSettings();

    if (rpcClient) {
        connect(rpcClient, &RPCClient::devNetAccessResult, this,
                &SettingsPage::onDevNetAccessResult);
        connect(rpcClient, &RPCClient::networkStatusUpdated, this,
                &SettingsPage::onNetworkStatusUpdated);
        connect(rpcClient, &RPCClient::connectionStatusChanged, this,
                &SettingsPage::onConnectionStatusChanged);
    }
}

void SettingsPage::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);

    // ---- Title ----
    QLabel *titleLabel = new QLabel(tr("Settings"), this);
    QFont f = titleLabel->font();
    f.setPointSize(18);
    f.setBold(true);
    titleLabel->setFont(f);
    mainLayout->addWidget(titleLabel);

    // ---- Live Network Status ----
    QGroupBox *statusBox = new QGroupBox(tr("Network Status"), this);
    QVBoxLayout *sl = new QVBoxLayout(statusBox);

    networkStatusBadge = new QLabel(tr("● Checking…"), statusBox);
    QFont bf = networkStatusBadge->font();
    bf.setBold(true);
    bf.setPointSize(13);
    networkStatusBadge->setFont(bf);
    sl->addWidget(networkStatusBadge);

    QHBoxLayout *statsRow = new QHBoxLayout();
    peerCountLabel = new QLabel(tr("Peers: —"), statusBox);
    latencyLabel = new QLabel(tr("Latency: —"), statusBox);
    nodeVersionLabel = new QLabel(tr("Node: —"), statusBox);
    statsRow->addWidget(peerCountLabel);
    statsRow->addWidget(latencyLabel);
    statsRow->addWidget(nodeVersionLabel);
    statsRow->addStretch();
    sl->addLayout(statsRow);

    QPushButton *refreshBtn = new QPushButton(tr("Refresh Status"), statusBox);
    connect(refreshBtn, &QPushButton::clicked, this, [this] {
        if (rpcClient)
            rpcClient->refreshNetworkStatus();
    });
    sl->addWidget(refreshBtn);
    mainLayout->addWidget(statusBox);

    // ---- Network Selector ----
    QGroupBox *netBox = new QGroupBox(tr("Network Selection"), this);
    QFormLayout *netForm = new QFormLayout(netBox);

    networkCombo = new QComboBox(netBox);
    networkCombo->addItem(tr("Mainnet  (port 8332)"), 0);
    networkCombo->addItem(tr("Testnet  (port 18332)"), 1);
    networkCombo->addItem(tr("Devnet   (port 18443) — governance role required"), 2);
    connect(networkCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &SettingsPage::onNetworkComboChanged);
    netForm->addRow(tr("Network:"), networkCombo);

    autoConnectCheck = new QCheckBox(tr("Auto-connect on startup"), netBox);
    netForm->addRow(autoConnectCheck);
    mainLayout->addWidget(netBox);

    // ---- DevNet gate (hidden unless Devnet is selected) ----
    devNetGateWidget = new QWidget(this);
    devNetGateWidget->setStyleSheet("QWidget { background:#fff8e1; border:1px solid #ffc107; "
                                    "border-radius:4px; padding:4px; }");
    QVBoxLayout *dv = new QVBoxLayout(devNetGateWidget);
    dv->setContentsMargins(10, 10, 10, 10);
    QLabel *devInfo = new QLabel(
        tr("⚠  Devnet access is restricted to governance roles: "
           "Boule member, Prytany member, EmergencyCouncil guardian, or Apophasis board member.\n"
           "Enter your address and click Verify to confirm eligibility."),
        devNetGateWidget);
    devInfo->setWordWrap(true);
    dv->addWidget(devInfo);
    QHBoxLayout *dvRow = new QHBoxLayout();
    devNetAddressEdit = new QLineEdit(devNetGateWidget);
    devNetAddressEdit->setPlaceholderText(tr("Your governance address (hex)"));
    dvRow->addWidget(devNetAddressEdit);
    verifyDevNetButton = new QPushButton(tr("Verify Role"), devNetGateWidget);
    connect(verifyDevNetButton, &QPushButton::clicked, this, [this] {
        const QString addr = devNetAddressEdit->text().trimmed();
        if (addr.isEmpty()) {
            devNetStatusLabel->setText(tr("Enter your address first."));
            devNetStatusLabel->setStyleSheet("QLabel{color:red;}");
            return;
        }
        verifyDevNetButton->setEnabled(false);
        devNetStatusLabel->setText(tr("Verifying…"));
        devNetStatusLabel->setStyleSheet("QLabel{color:blue;}");
        if (rpcClient)
            rpcClient->checkDevNetAccess(addr);
    });
    dvRow->addWidget(verifyDevNetButton);
    dv->addLayout(dvRow);
    devNetStatusLabel = new QLabel(devNetGateWidget);
    devNetStatusLabel->setWordWrap(true);
    dv->addWidget(devNetStatusLabel);
    devNetGateWidget->setVisible(false);
    mainLayout->addWidget(devNetGateWidget);

    // ---- RPC Connection ----
    QGroupBox *rpcBox = new QGroupBox(tr("RPC Connection"), this);
    QFormLayout *rpcLayout = new QFormLayout(rpcBox);

    rpcHostEdit = new QLineEdit(rpcBox);
    rpcHostEdit->setPlaceholderText("127.0.0.1");
    rpcLayout->addRow(tr("Host:"), rpcHostEdit);

    rpcPortEdit = new QLineEdit(rpcBox);
    rpcPortEdit->setPlaceholderText("8332");
    rpcLayout->addRow(tr("Port:"), rpcPortEdit);

    rpcUserEdit = new QLineEdit(rpcBox);
    rpcUserEdit->setPlaceholderText(tr("RPC username"));
    rpcLayout->addRow(tr("Username:"), rpcUserEdit);

    rpcPasswordEdit = new QLineEdit(rpcBox);
    rpcPasswordEdit->setPlaceholderText(tr("RPC password"));
    rpcPasswordEdit->setEchoMode(QLineEdit::Password);
    rpcLayout->addRow(tr("Password:"), rpcPasswordEdit);

    mainLayout->addWidget(rpcBox);

    // ---- Status + Buttons ----
    statusLabel = new QLabel(this);
    statusLabel->setWordWrap(true);
    mainLayout->addWidget(statusLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    saveButton = new QPushButton(tr("Save & Apply"), this);
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
    QSettings s("ParthenonChain", "Wallet");
    rpcHostEdit->setText(s.value("rpc/host", "127.0.0.1").toString());
    rpcPortEdit->setText(s.value("rpc/port", "8332").toString());
    rpcUserEdit->setText(s.value("rpc/user", "").toString());
    autoConnectCheck->setChecked(s.value("autoConnect", true).toBool());

    const QString network = s.value("network", "mainnet").toString();
    if (network == "testnet") {
        networkCombo->setCurrentIndex(1);
    } else if (network == "devnet") {
        networkCombo->setCurrentIndex(2);
    } else {
        networkCombo->setCurrentIndex(0);
    }
    updateNetworkStatusBadge();
}

void SettingsPage::onNetworkComboChanged(int index) {
    if (index == 2 /* Devnet */ && !devNetVerified) {
        devNetGateWidget->setVisible(true);
        // Revert to mainnet visually until verified
        // Don't change the underlying RPCClient yet
    } else {
        devNetGateWidget->setVisible(false);
        if (index != 2)
            devNetVerified = false; // reset if leaving devnet
    }
    // Auto-fill default port
    rpcPortEdit->setText(QString::number(RPCClient::defaultPort(indexToNetwork(index))));
}

void SettingsPage::onDevNetAccessResult(bool granted, const QString &role) {
    verifyDevNetButton->setEnabled(true);
    if (granted) {
        devNetVerified = true;
        devNetStatusLabel->setText(
            tr("✓ Access granted — Role: %1").arg(role.isEmpty() ? tr("Verified") : role));
        devNetStatusLabel->setStyleSheet("QLabel{color:green;font-weight:bold;}");
    } else {
        devNetVerified = false;
        devNetStatusLabel->setText(
            tr("✗ Access denied — address does not hold a qualifying governance role. "
               "Devnet requires Boule, Prytany, EmergencyCouncil, or Apophasis membership."));
        devNetStatusLabel->setStyleSheet("QLabel{color:red;}");
        // Revert combo to mainnet
        networkCombo->blockSignals(true);
        networkCombo->setCurrentIndex(0);
        networkCombo->blockSignals(false);
        devNetGateWidget->setVisible(false);
        rpcPortEdit->setText(QString::number(RPCClient::defaultPort(NetworkType::Mainnet)));
    }
}

void SettingsPage::onNetworkStatusUpdated() { updateNetworkStatusBadge(); }

void SettingsPage::onConnectionStatusChanged(bool /*connected*/) { updateNetworkStatusBadge(); }

void SettingsPage::updateNetworkStatusBadge() {
    if (!rpcClient) {
        networkStatusBadge->setText(tr("● Not connected"));
        networkStatusBadge->setStyleSheet("QLabel{color:#888;}");
        return;
    }

    const NetworkStatus ns = rpcClient->lastNetworkStatus();
    const bool conn = rpcClient->isConnected();
    const QString netName = RPCClient::networkName(rpcClient->networkType());

    QString badge;
    QString style;
    if (conn) {
        badge = tr("● %1  ·  Connected  ·  Block %2").arg(netName).arg(rpcClient->getBlockHeight());
        style = (rpcClient->networkType() == NetworkType::Mainnet)   ? "QLabel{color:#28a745;}"
                : (rpcClient->networkType() == NetworkType::Testnet) ? "QLabel{color:#fd7e14;}"
                                                                     : "QLabel{color:#6f42c1;}";
    } else {
        badge = tr("● %1  ·  Disconnected").arg(netName);
        style = "QLabel{color:#dc3545;}";
    }
    networkStatusBadge->setText(badge);
    networkStatusBadge->setStyleSheet(style);

    if (ns.peerCount > 0)
        peerCountLabel->setText(tr("Peers: %1").arg(ns.peerCount));
    if (ns.latencyMs >= 0)
        latencyLabel->setText(tr("Latency: %1 ms").arg(ns.latencyMs));
    if (!ns.nodeVersion.isEmpty())
        nodeVersionLabel->setText(tr("Node: %1").arg(ns.nodeVersion));
}

void SettingsPage::onSaveClicked() {
    const QString host = rpcHostEdit->text().trimmed();
    const QString portStr = rpcPortEdit->text().trimmed();
    const int idx = networkCombo->currentIndex();

    if (host.isEmpty()) {
        statusLabel->setText(tr("Error: Host cannot be empty"));
        statusLabel->setStyleSheet("QLabel{color:red;}");
        return;
    }
    bool ok = false;
    const int port = portStr.toInt(&ok);
    if (!ok || port <= 0 || port > 65535) {
        statusLabel->setText(tr("Error: Invalid port number"));
        statusLabel->setStyleSheet("QLabel{color:red;}");
        return;
    }
    if (idx == 2 && !devNetVerified) {
        statusLabel->setText(
            tr("Error: Devnet access not verified. Verify your governance role first."));
        statusLabel->setStyleSheet("QLabel{color:red;}");
        return;
    }

    const QStringList networkKeys = {"mainnet", "testnet", "devnet"};
    QSettings settings("ParthenonChain", "Wallet");
    settings.setValue("rpc/host", host);
    settings.setValue("rpc/port", port);
    settings.setValue("rpc/user", rpcUserEdit->text().trimmed());
    settings.setValue("network", networkKeys.value(idx, "mainnet"));
    settings.setValue("autoConnect", autoConnectCheck->isChecked());

    if (rpcClient) {
        rpcClient->setCredentials(rpcUserEdit->text().trimmed(), rpcPasswordEdit->text());
        rpcClient->setNetworkType(indexToNetwork(idx));
        rpcClient->connectToServer(host, port);
        rpcClient->refreshNetworkStatus();
    }

    statusLabel->setText(tr("Settings saved and applied."));
    statusLabel->setStyleSheet("QLabel{color:green;}");
    emit settingsSaved();
}

void SettingsPage::onResetClicked() {
    QMessageBox::StandardButton reply =
        QMessageBox::question(this, tr("Reset Settings"), tr("Reset all settings to defaults?"),
                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        rpcHostEdit->setText("127.0.0.1");
        rpcPortEdit->setText("8332");
        rpcUserEdit->clear();
        rpcPasswordEdit->clear();
        networkCombo->setCurrentIndex(0);
        autoConnectCheck->setChecked(true);
        devNetVerified = false;
        devNetGateWidget->setVisible(false);
        statusLabel->setText(tr("Settings reset to defaults. Click 'Save & Apply' to apply."));
        statusLabel->setStyleSheet("QLabel{color:blue;}");
    }
}
