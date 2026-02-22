// ParthenonChain Desktop Wallet - Receive Page Implementation

#include "receivepage.h"

#include "rpc_client.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QApplication>
#include <QClipboard>
#include <QFont>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

    ReceivePage::ReceivePage(RPCClient * rpc, QWidget* parent)
    : QWidget(parent),
            rpcClient(rpc) {
    setupUI();
}

void ReceivePage::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);

    // Title
    QLabel* titleLabel = new QLabel(tr("Receive Coins"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // Instructions
    QLabel* instructionsLabel =
        new QLabel(tr("Share your address with others to receive payments.\n"
                      "Each address can be used multiple times."),
                   this);
    instructionsLabel->setWordWrap(true);
    mainLayout->addWidget(instructionsLabel);

    // Address display
    QGroupBox* addressBox = new QGroupBox(tr("Your Address"), this);
    QVBoxLayout* addressLayout = new QVBoxLayout(addressBox);

    addressEdit = new QLineEdit(this);
    addressEdit->setReadOnly(true);
    addressEdit->setPlaceholderText(tr("Click 'Generate New Address' to create an address"));
    QFont addressFont("Monospace", 10);
    addressEdit->setFont(addressFont);
    addressLayout->addWidget(addressEdit);

    // Address action buttons
    QHBoxLayout* addressButtonLayout = new QHBoxLayout();
    generateButton = new QPushButton(tr("Generate New Address"), this);
    copyButton = new QPushButton(tr("Copy to Clipboard"), this);
    copyButton->setEnabled(false);

    connect(generateButton, &QPushButton::clicked, this, &ReceivePage::onGenerateAddress);
    connect(copyButton, &QPushButton::clicked, this, &ReceivePage::onCopyAddress);

    addressButtonLayout->addWidget(generateButton);
    addressButtonLayout->addWidget(copyButton);
    addressLayout->addLayout(addressButtonLayout);

    mainLayout->addWidget(addressBox);

    // QR Code placeholder
    QGroupBox* qrBox = new QGroupBox(tr("QR Code"), this);
    QVBoxLayout* qrLayout = new QVBoxLayout(qrBox);

    qrCodeLabel = new QLabel(this);
    qrCodeLabel->setMinimumSize(300, 300);
    qrCodeLabel->setAlignment(Qt::AlignCenter);
    qrCodeLabel->setStyleSheet("QLabel { border: 1px solid #ccc; background: white; }");
    qrCodeLabel->setText(tr("QR code will appear here"));
    qrLayout->addWidget(qrCodeLabel, 0, Qt::AlignCenter);

    mainLayout->addWidget(qrBox);

    mainLayout->addStretch();
}

void ReceivePage::onGenerateAddress() {
    if (!rpcClient) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Not connected to server. Please check your connection."));
        return;
    }

    QString newAddress = rpcClient->getNewAddress();
    if (newAddress.isEmpty()) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to generate address. Please check your connection."));
        return;
    }

    addressEdit->setText(newAddress);
    copyButton->setEnabled(true);

    qrCodeLabel->setText(tr("QR Code for:\n%1").arg(newAddress));
}

void ReceivePage::onCopyAddress() {
    QString address = addressEdit->text();
    if (address.isEmpty()) {
        return;
    }

    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(address);

    QMessageBox::information(this, tr("Copied"), tr("Address copied to clipboard!"));
}
