// ParthenonChain Desktop Wallet - Transaction Page Implementation

#include "transactionpage.h"

#include "rpc_client.h"

#include <QComboBox>
#include <QDateTime>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

TransactionPage::TransactionPage(RPCClient* rpc, QWidget* parent)
    : QWidget(parent), rpcClient(rpc) {
    setupUI();

    if (rpcClient) {
        connect(rpcClient, &RPCClient::transactionHistoryUpdated, this,
                &TransactionPage::onTransactionHistoryUpdated);
    }
}

void TransactionPage::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);

    // Title
    QLabel* titleLabel = new QLabel(tr("Transaction History"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // Filter and refresh controls
    QHBoxLayout* controlLayout = new QHBoxLayout();

    controlLayout->addWidget(new QLabel(tr("Filter:"), this));
    filterComboBox = new QComboBox(this);
    filterComboBox->addItems(
        {tr("All"), tr("Sent"), tr("Received"), tr("TALN Only"), tr("DRM Only"), tr("OBL Only")});
    connect(filterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &TransactionPage::onFilterChanged);
    controlLayout->addWidget(filterComboBox);

    controlLayout->addStretch();

    refreshButton = new QPushButton(tr("Refresh"), this);
    connect(refreshButton, &QPushButton::clicked, this, &TransactionPage::onRefresh);
    controlLayout->addWidget(refreshButton);

    mainLayout->addLayout(controlLayout);

    // Transaction table
    transactionTable = new QTableWidget(this);
    transactionTable->setColumnCount(6);
    transactionTable->setHorizontalHeaderLabels(
        {tr("Date/Time"), tr("Type"), tr("Asset"), tr("Amount"), tr("Address"), tr("TXID")});

    transactionTable->horizontalHeader()->setStretchLastSection(true);
    transactionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    transactionTable->setSelectionMode(QAbstractItemView::SingleSelection);
    transactionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    transactionTable->setAlternatingRowColors(true);

    // Set column widths
    transactionTable->setColumnWidth(0, 150);  // Date/Time
    transactionTable->setColumnWidth(1, 80);   // Type
    transactionTable->setColumnWidth(2, 60);   // Asset
    transactionTable->setColumnWidth(3, 120);  // Amount
    transactionTable->setColumnWidth(4, 200);  // Address

    mainLayout->addWidget(transactionTable);

    // Load initial data
    loadTransactions();
}

void TransactionPage::onRefresh() {
    if (rpcClient) {
        rpcClient->getTransactionHistory();
    } else {
        loadTransactions();
    }
}

void TransactionPage::onFilterChanged(int index) {
    // Filter transactions based on selection
    Q_UNUSED(index);
    loadTransactions();
}

void TransactionPage::onTransactionHistoryUpdated() {
    loadTransactions();
}

void TransactionPage::loadTransactions() {
    // Clear existing rows
    transactionTable->setRowCount(0);

    // In production, this would load from rpcClient
    // For now, show sample data
    struct Transaction {
        QString dateTime;
        QString type;
        QString asset;
        double amount;
        QString address;
        QString txid;
    };

    QList<Transaction> sampleTxs = {
        {QDateTime::currentDateTime().addDays(-1).toString("yyyy-MM-dd hh:mm:ss"), "Received",
         "TALN", 100.50000000, "parthenon1q123...", "abc123..."},
        {QDateTime::currentDateTime().addDays(-2).toString("yyyy-MM-dd hh:mm:ss"), "Sent", "DRM",
         -50.25000000, "parthenon1q456...", "def456..."},
        {QDateTime::currentDateTime().addDays(-3).toString("yyyy-MM-dd hh:mm:ss"), "Received",
         "OBL", 200.00000000, "parthenon1q789...", "ghi789..."},
    };

    for (const auto& tx : sampleTxs) {
        int row = transactionTable->rowCount();
        transactionTable->insertRow(row);

        transactionTable->setItem(row, 0, new QTableWidgetItem(tx.dateTime));
        transactionTable->setItem(row, 1, new QTableWidgetItem(tx.type));
        transactionTable->setItem(row, 2, new QTableWidgetItem(tx.asset));
        transactionTable->setItem(row, 3, new QTableWidgetItem(QString::number(tx.amount, 'f', 8)));
        transactionTable->setItem(row, 4, new QTableWidgetItem(tx.address));
        transactionTable->setItem(row, 5, new QTableWidgetItem(tx.txid));

        // Color code sent/received
        if (tx.type == "Sent") {
            transactionTable->item(row, 3)->setForeground(Qt::red);
        } else {
            transactionTable->item(row, 3)->setForeground(Qt::darkGreen);
        }
    }
}
