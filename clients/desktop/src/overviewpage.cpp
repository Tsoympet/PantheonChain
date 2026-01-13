// ParthenonChain Desktop Wallet - Overview Page Implementation

#include "overviewpage.h"
#include "rpc_client.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QFont>

OverviewPage::OverviewPage(RPCClient *rpc, QWidget *parent)
    : QWidget(parent),
      rpcClient(rpc),
      currentAsset("TALN")
{
    setupUI();
    updateBalances();
}

void OverviewPage::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    
    // Title
    QLabel *titleLabel = new QLabel(tr("Wallet Overview"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);
    
    // Asset selector
    QHBoxLayout *assetLayout = new QHBoxLayout();
    assetLayout->addWidget(new QLabel(tr("Select Asset:"), this));
    assetSelector = new QComboBox(this);
    assetSelector->addItems({"TALN", "DRM", "OBL"});
    connect(assetSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OverviewPage::onAssetChanged);
    assetLayout->addWidget(assetSelector);
    assetLayout->addStretch();
    mainLayout->addLayout(assetLayout);
    
    // Main balance display
    QGroupBox *balanceBox = new QGroupBox(tr("Current Balance"), this);
    QVBoxLayout *balanceLayout = new QVBoxLayout(balanceBox);
    
    balanceLabel = new QLabel(tr("TALANTON (TALN)"), this);
    QFont balanceLabelFont = balanceLabel->font();
    balanceLabelFont.setPointSize(12);
    balanceLabel->setFont(balanceLabelFont);
    balanceLayout->addWidget(balanceLabel);
    
    balanceValueLabel = new QLabel("0.00000000", this);
    QFont balanceFont = balanceValueLabel->font();
    balanceFont.setPointSize(24);
    balanceFont.setBold(true);
    balanceValueLabel->setFont(balanceFont);
    balanceLayout->addWidget(balanceValueLabel);
    
    mainLayout->addWidget(balanceBox);
    
    // All balances
    QGroupBox *allBalancesBox = new QGroupBox(tr("All Assets"), this);
    QVBoxLayout *allBalancesLayout = new QVBoxLayout(allBalancesBox);
    
    talnBalanceLabel = new QLabel(tr("TALANTON (TALN): 0.00000000"), this);
    drmBalanceLabel = new QLabel(tr("DRACHMA (DRM): 0.00000000"), this);
    oblBalanceLabel = new QLabel(tr("OBOLOS (OBL): 0.00000000"), this);
    
    allBalancesLayout->addWidget(talnBalanceLabel);
    allBalancesLayout->addWidget(drmBalanceLabel);
    allBalancesLayout->addWidget(oblBalanceLabel);
    
    mainLayout->addWidget(allBalancesBox);
    
    // Quick actions
    QHBoxLayout *actionLayout = new QHBoxLayout();
    sendButton = new QPushButton(tr("Send"), this);
    receiveButton = new QPushButton(tr("Receive"), this);
    
    sendButton->setMinimumHeight(40);
    receiveButton->setMinimumHeight(40);
    
    connect(sendButton, &QPushButton::clicked, this, &OverviewPage::onQuickSend);
    connect(receiveButton, &QPushButton::clicked, this, &OverviewPage::onQuickReceive);
    
    actionLayout->addWidget(sendButton);
    actionLayout->addWidget(receiveButton);
    mainLayout->addLayout(actionLayout);
    
    mainLayout->addStretch();
}

void OverviewPage::updateBalances() {
    if (!rpcClient) return;
    
    double talnBalance = rpcClient->getBalance("TALN");
    double drmBalance = rpcClient->getBalance("DRM");
    double oblBalance = rpcClient->getBalance("OBL");
    
    talnBalanceLabel->setText(QString("TALANTON (TALN): %1").arg(talnBalance, 0, 'f', 8));
    drmBalanceLabel->setText(QString("DRACHMA (DRM): %1").arg(drmBalance, 0, 'f', 8));
    oblBalanceLabel->setText(QString("OBOLOS (OBL): %1").arg(oblBalance, 0, 'f', 8));
    
    // Update current asset display
    double currentBalance = 0.0;
    if (currentAsset == "TALN") {
        balanceLabel->setText(tr("TALANTON (TALN)"));
        currentBalance = talnBalance;
    } else if (currentAsset == "DRM") {
        balanceLabel->setText(tr("DRACHMA (DRM)"));
        currentBalance = drmBalance;
    } else if (currentAsset == "OBL") {
        balanceLabel->setText(tr("OBOLOS (OBL)"));
        currentBalance = oblBalance;
    }
    
    balanceValueLabel->setText(QString::number(currentBalance, 'f', 8));
}

void OverviewPage::onAssetChanged(int index) {
    currentAsset = assetSelector->itemText(index);
    updateBalances();
}

void OverviewPage::onQuickSend() {
    // Emit signal to switch to send page (handled by parent)
    emit parentWidget()->findChild<QAction*>("sendAction")->trigger();
}

void OverviewPage::onQuickReceive() {
    // Emit signal to switch to receive page (handled by parent)
    emit parentWidget()->findChild<QAction*>("receiveAction")->trigger();
}
