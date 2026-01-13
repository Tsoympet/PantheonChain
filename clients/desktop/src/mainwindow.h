// ParthenonChain Desktop Wallet - Main Window Header

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "rpc_client.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QToolBar;
class QStatusBar;
class QLabel;
QT_END_NAMESPACE

class OverviewPage;
class SendPage;
class ReceivePage;
class TransactionPage;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void showOverview();
    void showSend();
    void showReceive();
    void showTransactions();
    void showAbout();
    void updateStatus();
    void onConnectionStatusChanged(bool connected);
    void onBalanceChanged();

private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void connectToRPC();

    // Pages
    QStackedWidget *centralStack;
    OverviewPage *overviewPage;
    SendPage *sendPage;
    ReceivePage *receivePage;
    TransactionPage *transactionPage;

    // Actions
    QAction *overviewAction;
    QAction *sendAction;
    QAction *receiveAction;
    QAction *transactionsAction;
    QAction *exitAction;
    QAction *aboutAction;
    QAction *aboutQtAction;

    // Menus
    QMenu *fileMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;

    // Toolbar
    QToolBar *toolBar;

    // Status bar widgets
    QLabel *connectionLabel;
    QLabel *blockHeightLabel;
    QLabel *syncProgressLabel;

    // RPC client
    RPCClient *rpcClient;
};

#endif // MAINWINDOW_H
