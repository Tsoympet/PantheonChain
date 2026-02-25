// ParthenonChain Desktop Wallet - Staking Page Implementation

#include "stakingpage.h"

#include "rpc_client.h"

#include <QComboBox>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

StakingPage::StakingPage(RPCClient *rpc, QWidget *parent)
    : QWidget(parent), rpcClient(rpc), layerCombo(nullptr), amountEdit(nullptr),
      addressEdit(nullptr), stakingPowerLabel(nullptr), statusLabel(nullptr),
      stakeButton(nullptr), unstakeButton(nullptr) {
    setupUI();

    if (rpcClient) {
        connect(rpcClient, &RPCClient::stakingPowerUpdated, this, &StakingPage::onStakingPowerUpdated);
        connect(rpcClient, &RPCClient::stakeConfirmed,      this, &StakingPage::onStakeConfirmed);
        connect(rpcClient, &RPCClient::unstakeConfirmed,    this, &StakingPage::onUnstakeConfirmed);
        connect(rpcClient, &RPCClient::errorOccurred,       this, &StakingPage::onError);
    }
}

void StakingPage::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);

    // Title
    QLabel *titleLabel = new QLabel(tr("Staking"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // Info banner
    QLabel *infoLabel = new QLabel(
        tr("Stake DRACHMA (DRM) on L2 or OBOLOS (OBL) on L3 to participate in Proof-of-Stake "
           "consensus and earn governance voting power."),
        this);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { background-color: #e8f4fd; border-left: 4px solid #007AFF; "
                              "padding: 8px; border-radius: 4px; }");
    mainLayout->addWidget(infoLabel);

    // Staking power display
    QGroupBox *powerBox = new QGroupBox(tr("Your Staking Power"), this);
    QVBoxLayout *powerLayout = new QVBoxLayout(powerBox);

    stakingPowerLabel = new QLabel(tr("Staking power: —"), powerBox);
    QFont powerFont = stakingPowerLabel->font();
    powerFont.setPointSize(14);
    powerFont.setBold(true);
    stakingPowerLabel->setFont(powerFont);
    powerLayout->addWidget(stakingPowerLabel);

    mainLayout->addWidget(powerBox);

    // Stake / Unstake form
    QGroupBox *formBox = new QGroupBox(tr("Manage Stake"), this);
    QFormLayout *formLayout = new QFormLayout(formBox);

    layerCombo = new QComboBox(formBox);
    layerCombo->addItem(tr("L2 – DRACHMA (DRM)"), "l2");
    layerCombo->addItem(tr("L3 – OBOLOS (OBL)"),  "l3");
    connect(layerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &StakingPage::onLayerChanged);
    formLayout->addRow(tr("Layer:"), layerCombo);

    addressEdit = new QLineEdit(formBox);
    addressEdit->setPlaceholderText(tr("Your staking address"));
    formLayout->addRow(tr("Address:"), addressEdit);

    amountEdit = new QLineEdit(formBox);
    amountEdit->setPlaceholderText("0.00000000");
    formLayout->addRow(tr("Amount:"), amountEdit);

    mainLayout->addWidget(formBox);

    // Status label
    statusLabel = new QLabel(this);
    statusLabel->setWordWrap(true);
    mainLayout->addWidget(statusLabel);

    // Action buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    stakeButton   = new QPushButton(tr("Stake"),   this);
    unstakeButton = new QPushButton(tr("Unstake"), this);
    stakeButton->setMinimumHeight(40);
    unstakeButton->setMinimumHeight(40);
    stakeButton->setStyleSheet("QPushButton { background-color: #28a745; color: white; "
                                "font-weight: bold; }");
    unstakeButton->setStyleSheet("QPushButton { background-color: #dc3545; color: white; "
                                  "font-weight: bold; }");

    connect(stakeButton,   &QPushButton::clicked, this, &StakingPage::onStakeClicked);
    connect(unstakeButton, &QPushButton::clicked, this, &StakingPage::onUnstakeClicked);

    buttonLayout->addWidget(stakeButton);
    buttonLayout->addWidget(unstakeButton);
    mainLayout->addLayout(buttonLayout);

    // Notes
    QGroupBox *notesBox = new QGroupBox(tr("Notes"), this);
    QVBoxLayout *notesLayout = new QVBoxLayout(notesBox);
    const QStringList notes = {
        tr("• Staked tokens are locked during the unbonding period."),
        tr("• Staking increases governance voting power proportionally."),
        tr("• Slashing may apply for validator misbehaviour."),
        tr("• Rewards are distributed at the end of each epoch."),
    };
    for (const QString &note : notes) {
        notesLayout->addWidget(new QLabel(note, notesBox));
    }
    mainLayout->addWidget(notesBox);

    mainLayout->addStretch();
}

void StakingPage::refreshStakingPower() {
    if (!rpcClient) return;
    const QString address = addressEdit->text().trimmed();
    if (!address.isEmpty()) {
        rpcClient->getStakingPower(address);
    }
}

void StakingPage::onLayerChanged(int /*index*/) {
    refreshStakingPower();
}

void StakingPage::onStakeClicked() {
    if (!rpcClient) {
        statusLabel->setText(tr("Error: Not connected to server"));
        statusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    const QString address = addressEdit->text().trimmed();
    if (address.isEmpty()) {
        statusLabel->setText(tr("Error: Please enter a staking address"));
        statusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    bool ok = false;
    const double amount = amountEdit->text().trimmed().toDouble(&ok);
    if (!ok || amount <= 0) {
        statusLabel->setText(tr("Error: Invalid amount"));
        statusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    const QString layer  = layerCombo->currentData().toString();
    const QString token  = (layer == "l2") ? "DRM" : "OBL";

    QMessageBox::StandardButton reply =
        QMessageBox::question(this, tr("Confirm Stake"),
                              tr("Stake %1 %2 on %3?").arg(amount).arg(token).arg(layerCombo->currentText()),
                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        statusLabel->setText(tr("Sending stake request…"));
        statusLabel->setStyleSheet("QLabel { color: blue; }");
        rpcClient->stakeTokens(address, amount, layer);
    }
}

void StakingPage::onUnstakeClicked() {
    if (!rpcClient) {
        statusLabel->setText(tr("Error: Not connected to server"));
        statusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    const QString address = addressEdit->text().trimmed();
    if (address.isEmpty()) {
        statusLabel->setText(tr("Error: Please enter a staking address"));
        statusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    bool ok = false;
    const double amount = amountEdit->text().trimmed().toDouble(&ok);
    if (!ok || amount <= 0) {
        statusLabel->setText(tr("Error: Invalid amount"));
        statusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    const QString layer = layerCombo->currentData().toString();
    const QString token = (layer == "l2") ? "DRM" : "OBL";

    QMessageBox::StandardButton reply =
        QMessageBox::question(this, tr("Confirm Unstake"),
                              tr("Unstake %1 %2 from %3?").arg(amount).arg(token).arg(layerCombo->currentText()),
                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        statusLabel->setText(tr("Sending unstake request…"));
        statusLabel->setStyleSheet("QLabel { color: blue; }");
        rpcClient->unstakeTokens(address, amount, layer);
    }
}

void StakingPage::onStakingPowerUpdated(double power) {
    stakingPowerLabel->setText(tr("Staking power: %1").arg(power, 0, 'f', 8));
}

void StakingPage::onStakeConfirmed(const QString &layer, double amount) {
    const QString token = (layer == "l2") ? "DRM" : "OBL";
    statusLabel->setText(
        tr("Stake confirmed: %1 %2 on %3.").arg(amount, 0, 'f', 8).arg(token).arg(layer.toUpper()));
    statusLabel->setStyleSheet("QLabel { color: green; }");
    amountEdit->clear();
    refreshStakingPower();
}

void StakingPage::onUnstakeConfirmed(const QString &layer, double amount) {
    const QString token = (layer == "l2") ? "DRM" : "OBL";
    statusLabel->setText(
        tr("Unstake confirmed: %1 %2 from %3.").arg(amount, 0, 'f', 8).arg(token).arg(layer.toUpper()));
    statusLabel->setStyleSheet("QLabel { color: green; }");
    amountEdit->clear();
    refreshStakingPower();
}

void StakingPage::onError(const QString &error) {
    statusLabel->setText(tr("Error: %1").arg(error));
    statusLabel->setStyleSheet("QLabel { color: red; }");
}
