// ParthenonChain Desktop Wallet - Send Page Implementation

#include "sendpage.h"

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

SendPage::SendPage(RPCClient* rpc, QWidget* parent) : QWidget(parent), rpcClient(rpc) {
    setupUI();

    if (rpcClient) {
        connect(rpcClient, &RPCClient::transactionSent, this, &SendPage::onTransactionSent);
        connect(rpcClient, &RPCClient::errorOccurred, this, &SendPage::onError);
    }
}

void SendPage::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);

    // Title
    QLabel* titleLabel = new QLabel(tr("Send Transaction"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // Send form
    QGroupBox* formBox = new QGroupBox(tr("Transaction Details"), this);
    QFormLayout* formLayout = new QFormLayout(formBox);

    // Asset selector
    assetSelector = new QComboBox(this);
    assetSelector->addItems({"TALN", "DRM", "OBL"});
    formLayout->addRow(tr("Asset:"), assetSelector);

    // Recipient address
    addressEdit = new QLineEdit(this);
    addressEdit->setPlaceholderText("parthenon1q...");
    formLayout->addRow(tr("To Address:"), addressEdit);

    // Amount with MAX button
    QHBoxLayout* amountLayout = new QHBoxLayout();
    amountEdit = new QLineEdit(this);
    amountEdit->setPlaceholderText("0.00000000");
    maxButton = new QPushButton(tr("MAX"), this);
    maxButton->setMaximumWidth(60);
    connect(maxButton, &QPushButton::clicked, this, &SendPage::onMaxClicked);
    amountLayout->addWidget(amountEdit);
    amountLayout->addWidget(maxButton);
    formLayout->addRow(tr("Amount:"), amountLayout);

    // Optional memo
    memoEdit = new QLineEdit(this);
    memoEdit->setPlaceholderText(tr("Optional transaction note"));
    formLayout->addRow(tr("Memo:"), memoEdit);

    mainLayout->addWidget(formBox);

    // Status label
    statusLabel = new QLabel(this);
    statusLabel->setWordWrap(true);
    mainLayout->addWidget(statusLabel);

    // Action buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    sendButton = new QPushButton(tr("Send Transaction"), this);
    sendButton->setMinimumHeight(40);
    clearButton = new QPushButton(tr("Clear"), this);
    clearButton->setMinimumHeight(40);

    connect(sendButton, &QPushButton::clicked, this, &SendPage::onSendClicked);
    connect(clearButton, &QPushButton::clicked, this, &SendPage::onClearClicked);

    buttonLayout->addWidget(sendButton);
    buttonLayout->addWidget(clearButton);
    mainLayout->addLayout(buttonLayout);

    mainLayout->addStretch();
}

void SendPage::onSendClicked() {
    if (!validateInputs()) {
        return;
    }

    if (!rpcClient) {
        statusLabel->setText(tr("Error: Not connected to server"));
        statusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    QString asset = assetSelector->currentText();
    QString address = addressEdit->text().trimmed();
    double amount = amountEdit->text().toDouble();
    QString memo = memoEdit->text().trimmed();

    // Confirm transaction
    QString confirmMsg = tr("Send %1 %2 to %3?").arg(amount).arg(asset).arg(address);
    if (!memo.isEmpty()) {
        confirmMsg += tr("\nMemo: %1").arg(memo);
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Confirm Send"), confirmMsg,
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        statusLabel->setText(tr("Sending transaction..."));
        statusLabel->setStyleSheet("QLabel { color: blue; }");
        rpcClient->sendTransaction(asset, address, amount, memo);
    }
}

void SendPage::onMaxClicked() {
    if (!rpcClient)
        return;

    QString asset = assetSelector->currentText();
    double balance = rpcClient->getBalance(asset);

    // Leave a small amount for fees (0.0001)
    double maxAmount = balance - 0.0001;
    if (maxAmount > 0) {
        amountEdit->setText(QString::number(maxAmount, 'f', 8));
    }
}

void SendPage::onClearClicked() {
    addressEdit->clear();
    amountEdit->clear();
    memoEdit->clear();
    statusLabel->clear();
}

void SendPage::onTransactionSent(const QString& txid) {
    statusLabel->setText(tr("Transaction sent successfully!\nTXID: %1").arg(txid));
    statusLabel->setStyleSheet("QLabel { color: green; }");

    // Clear form
    addressEdit->clear();
    amountEdit->clear();
    memoEdit->clear();
}

void SendPage::onError(const QString& error) {
    statusLabel->setText(tr("Error: %1").arg(error));
    statusLabel->setStyleSheet("QLabel { color: red; }");
}

bool SendPage::validateInputs() {
    QString address = addressEdit->text().trimmed();
    QString amountStr = amountEdit->text().trimmed();

    if (address.isEmpty()) {
        statusLabel->setText(tr("Error: Please enter a recipient address"));
        statusLabel->setStyleSheet("QLabel { color: red; }");
        return false;
    }

    if (!address.startsWith("parthenon1")) {
        statusLabel->setText(tr("Error: Invalid address format"));
        statusLabel->setStyleSheet("QLabel { color: red; }");
        return false;
    }

    if (amountStr.isEmpty()) {
        statusLabel->setText(tr("Error: Please enter an amount"));
        statusLabel->setStyleSheet("QLabel { color: red; }");
        return false;
    }

    bool ok;
    double amount = amountStr.toDouble(&ok);
    if (!ok || amount <= 0) {
        statusLabel->setText(tr("Error: Invalid amount"));
        statusLabel->setStyleSheet("QLabel { color: red; }");
        return false;
    }

    // Check if sufficient balance
    if (rpcClient) {
        QString asset = assetSelector->currentText();
        double balance = rpcClient->getBalance(asset);
        if (amount > balance) {
            statusLabel->setText(tr("Error: Insufficient balance"));
            statusLabel->setStyleSheet("QLabel { color: red; }");
            return false;
        }
    }

    return true;
}
