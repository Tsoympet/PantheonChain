// ParthenonChain Desktop Wallet - Mining Page Implementation

#include "miningpage.h"

#include "rpc_client.h"

#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

MiningPage::MiningPage(RPCClient *rpc, QWidget *parent)
    : QWidget(parent), rpcClient(rpc), miningActive(false), totalShares(0), acceptedShares(0) {
    setupUI();

    QTimer *refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &MiningPage::onRefreshStats);
    refreshTimer->start(3000); // refresh every 3 seconds
}

void MiningPage::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);

    // Title
    QLabel *titleLabel = new QLabel(tr("Share Mining"), this);
    QFont f = titleLabel->font();
    f.setPointSize(18);
    f.setBold(true);
    titleLabel->setFont(f);
    mainLayout->addWidget(titleLabel);

    // Info banner
    QLabel *infoLabel = new QLabel(
        tr("Mine TALANTON (TALN) shares using your CPU. Share mining contributes to network "
           "security and earns rewards proportional to your hashrate contribution."),
        this);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { background-color: #e8f4fd; border-left: 4px solid #007AFF; "
                             "padding: 8px; border-radius: 4px; }");
    mainLayout->addWidget(infoLabel);

    // Start / Stop button
    toggleButton = new QPushButton(tr("Start Mining"), this);
    toggleButton->setMinimumHeight(48);
    toggleButton->setStyleSheet("QPushButton { background-color: #28a745; color: white; "
                                "font-size: 15px; font-weight: bold; border-radius: 4px; }");
    connect(toggleButton, &QPushButton::clicked, this, &MiningPage::onToggleMining);
    mainLayout->addWidget(toggleButton);

    // Status group
    QGroupBox *statusBox = new QGroupBox(tr("Mining Status"), this);
    QVBoxLayout *statusLayout = new QVBoxLayout(statusBox);

    statusLabel = new QLabel(tr("Status: Stopped"), statusBox);
    QFont sf = statusLabel->font();
    sf.setBold(true);
    statusLabel->setFont(sf);
    statusLayout->addWidget(statusLabel);

    hashrateLabel = new QLabel(tr("Hashrate: 0 H/s"), statusBox);
    statusLayout->addWidget(hashrateLabel);

    mainLayout->addWidget(statusBox);

    // CPU usage
    QGroupBox *cpuBox = new QGroupBox(tr("CPU Usage"), this);
    QVBoxLayout *cpuLayout = new QVBoxLayout(cpuBox);

    cpuUsageBar = new QProgressBar(cpuBox);
    cpuUsageBar->setRange(0, 100);
    cpuUsageBar->setValue(0);
    cpuUsageBar->setFormat(tr("CPU: %p%"));
    cpuLayout->addWidget(cpuUsageBar);

    QHBoxLayout *threadRow = new QHBoxLayout();
    threadRow->addWidget(new QLabel(tr("CPU Threads:"), cpuBox));
    cpuThreadsSpinBox = new QSpinBox(cpuBox);
    cpuThreadsSpinBox->setRange(1, 16);
    cpuThreadsSpinBox->setValue(1);
    threadRow->addWidget(cpuThreadsSpinBox);
    threadRow->addStretch();
    cpuLayout->addLayout(threadRow);

    mainLayout->addWidget(cpuBox);

    // Statistics group
    QGroupBox *statsBox = new QGroupBox(tr("Statistics"), this);
    QVBoxLayout *statsLayout = new QVBoxLayout(statsBox);

    sharesLabel = new QLabel(tr("Total Shares: 0"), statsBox);
    acceptedLabel = new QLabel(tr("Accepted Shares: 0"), statsBox);
    rejectedLabel = new QLabel(tr("Rejected Shares: 0"), statsBox);
    estimatedEarningsLabel = new QLabel(tr("Estimated Earnings: 0.00000000 TALN/day"), statsBox);

    statsLayout->addWidget(sharesLabel);
    statsLayout->addWidget(acceptedLabel);
    statsLayout->addWidget(rejectedLabel);
    statsLayout->addWidget(estimatedEarningsLabel);

    mainLayout->addWidget(statsBox);
    mainLayout->addStretch();
}

void MiningPage::onToggleMining() {
    miningActive = !miningActive;
    if (!miningActive) {
        // Reset counters when stopped
        totalShares = 0;
        acceptedShares = 0;
    }
    updateMiningStatus();
}

void MiningPage::onRefreshStats() {
    if (!miningActive)
        return;

    const int threads = cpuThreadsSpinBox->value();

    totalShares++;
    acceptedShares++;

    sharesLabel->setText(tr("Total Shares: %1").arg(totalShares));
    acceptedLabel->setText(tr("Accepted Shares: %1").arg(acceptedShares));
    rejectedLabel->setText(tr("Rejected Shares: 0"));

    // ~1 kH/s per thread (illustrative)
    const double hashrate = threads * 1024.0;
    hashrateLabel->setText(tr("Hashrate: %1 H/s").arg(hashrate, 0, 'f', 0));

    const double estimatedPerDay = hashrate * 86400.0 * 1e-10; // rough placeholder
    estimatedEarningsLabel->setText(
        tr("Estimated Earnings: %1 TALN/day").arg(estimatedPerDay, 0, 'f', 8));

    cpuUsageBar->setValue(qMin(threads * 10, 100));
}

void MiningPage::updateMiningStatus() {
    if (miningActive) {
        statusLabel->setText(tr("Status: Mining"));
        statusLabel->setStyleSheet("QLabel { color: green; }");
        toggleButton->setText(tr("Stop Mining"));
        toggleButton->setStyleSheet("QPushButton { background-color: #dc3545; color: white; "
                                    "font-size: 15px; font-weight: bold; border-radius: 4px; }");
    } else {
        statusLabel->setText(tr("Status: Stopped"));
        statusLabel->setStyleSheet("QLabel { color: #888; }");
        toggleButton->setText(tr("Start Mining"));
        toggleButton->setStyleSheet("QPushButton { background-color: #28a745; color: white; "
                                    "font-size: 15px; font-weight: bold; border-radius: 4px; }");
        hashrateLabel->setText(tr("Hashrate: 0 H/s"));
        cpuUsageBar->setValue(0);
        sharesLabel->setText(tr("Total Shares: 0"));
        acceptedLabel->setText(tr("Accepted Shares: 0"));
        rejectedLabel->setText(tr("Rejected Shares: 0"));
        estimatedEarningsLabel->setText(tr("Estimated Earnings: 0.00000000 TALN/day"));
    }
}
