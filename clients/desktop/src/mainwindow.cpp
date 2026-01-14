// ParthenonChain Desktop Wallet - Main Window Implementation

#include "mainwindow.h"

#include "overviewpage.h"
#include "receivepage.h"
#include "sendpage.h"
#include "transactionpage.h"

#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      centralStack(nullptr),
      overviewPage(nullptr),
      sendPage(nullptr),
      receivePage(nullptr),
      transactionPage(nullptr),
      rpcClient(nullptr) {
    setWindowTitle("ParthenonChain Wallet");
    resize(1000, 700);

    // Create RPC client
    rpcClient = new RPCClient(this);
    connect(rpcClient, &RPCClient::connectionStatusChanged, this,
            &MainWindow::onConnectionStatusChanged);
    connect(rpcClient, &RPCClient::balanceChanged, this, &MainWindow::onBalanceChanged);

    // Create central widget with stacked pages
    centralStack = new QStackedWidget(this);
    setCentralWidget(centralStack);

    // Create pages
    overviewPage = new OverviewPage(rpcClient, this);
    sendPage = new SendPage(rpcClient, this);
    receivePage = new ReceivePage(rpcClient, this);
    transactionPage = new TransactionPage(rpcClient, this);

    // Connect overview page signals
    connect(overviewPage, &OverviewPage::sendRequested, this, &MainWindow::showSend);
    connect(overviewPage, &OverviewPage::receiveRequested, this, &MainWindow::showReceive);

    centralStack->addWidget(overviewPage);
    centralStack->addWidget(sendPage);
    centralStack->addWidget(receivePage);
    centralStack->addWidget(transactionPage);

    // Create UI elements
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    // Show overview by default
    showOverview();

    // Connect to RPC server
    connectToRPC();

    // Setup update timer
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateStatus);
    timer->start(5000);  // Update every 5 seconds
}

MainWindow::~MainWindow() {}

void MainWindow::showOverview() {
    centralStack->setCurrentWidget(overviewPage);
    overviewAction->setChecked(true);
}

void MainWindow::showSend() {
    centralStack->setCurrentWidget(sendPage);
    sendAction->setChecked(true);
}

void MainWindow::showReceive() {
    centralStack->setCurrentWidget(receivePage);
    receiveAction->setChecked(true);
}

void MainWindow::showTransactions() {
    centralStack->setCurrentWidget(transactionPage);
    transactionsAction->setChecked(true);
}

void MainWindow::showAbout() {
    QMessageBox::about(this, tr("About ParthenonChain Wallet"),
                       tr("<h2>ParthenonChain Wallet v1.0.0</h2>"
                          "<p>Multi-asset blockchain wallet supporting:</p>"
                          "<ul>"
                          "<li>TALANTON (TALN) - 21M max supply</li>"
                          "<li>DRACHMA (DRM) - 41M max supply</li>"
                          "<li>OBOLOS (OBL) - 61M max supply</li>"
                          "</ul>"
                          "<p>Built with Qt %1</p>"
                          "<p>Copyright © 2024 ParthenonChain Developers</p>")
                           .arg(QT_VERSION_STR));
}

void MainWindow::updateStatus() {
    if (rpcClient && rpcClient->isConnected()) {
        rpcClient->updateBalances();
        rpcClient->updateBlockHeight();
    }
}

void MainWindow::onConnectionStatusChanged(bool connected) {
    if (connected) {
        connectionLabel->setText("Connected");
        connectionLabel->setStyleSheet("QLabel { color: green; }");
        updateStatus();
    } else {
        connectionLabel->setText("Disconnected");
        connectionLabel->setStyleSheet("QLabel { color: red; }");
    }
}

void MainWindow::onBalanceChanged() {
    if (overviewPage) {
        overviewPage->updateBalances();
    }
}

void MainWindow::createActions() {
    overviewAction = new QAction(tr("&Overview"), this);
    overviewAction->setStatusTip(tr("Show wallet overview"));
    overviewAction->setCheckable(true);
    connect(overviewAction, &QAction::triggered, this, &MainWindow::showOverview);

    sendAction = new QAction(tr("&Send"), this);
    sendAction->setStatusTip(tr("Send coins"));
    sendAction->setCheckable(true);
    connect(sendAction, &QAction::triggered, this, &MainWindow::showSend);

    receiveAction = new QAction(tr("&Receive"), this);
    receiveAction->setStatusTip(tr("Receive coins"));
    receiveAction->setCheckable(true);
    connect(receiveAction, &QAction::triggered, this, &MainWindow::showReceive);

    transactionsAction = new QAction(tr("&Transactions"), this);
    transactionsAction->setStatusTip(tr("View transaction history"));
    transactionsAction->setCheckable(true);
    connect(transactionsAction, &QAction::triggered, this, &MainWindow::showTransactions);

    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setStatusTip(tr("Exit application"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    aboutAction = new QAction(tr("&About ParthenonChain"), this);
    aboutAction->setStatusTip(tr("Show information about ParthenonChain"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);

    aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("Show information about Qt"));
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::createMenus() {
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(exitAction);

    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(overviewAction);
    viewMenu->addAction(sendAction);
    viewMenu->addAction(receiveAction);
    viewMenu->addAction(transactionsAction);

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
    helpMenu->addAction(aboutQtAction);
}

void MainWindow::createToolBars() {
    toolBar = addToolBar(tr("Navigation"));
    toolBar->addAction(overviewAction);
    toolBar->addAction(sendAction);
    toolBar->addAction(receiveAction);
    toolBar->addAction(transactionsAction);
    toolBar->setMovable(false);
}

void MainWindow::createStatusBar() {
    connectionLabel = new QLabel(tr("Connecting..."));
    blockHeightLabel = new QLabel(tr("Block: 0"));
    syncProgressLabel = new QLabel(tr("Synced"));

    statusBar()->addWidget(connectionLabel);
    statusBar()->addPermanentWidget(blockHeightLabel);
    statusBar()->addPermanentWidget(syncProgressLabel);
}

void MainWindow::connectToRPC() {
    if (rpcClient) {
        rpcClient->connectToServer("127.0.0.1", 8332);
    }
}
