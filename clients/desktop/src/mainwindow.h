// ParthenonChain Desktop Wallet - Main Window Header

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "rpc_client.h"

#include <QMainWindow>
#include <QStackedWidget>

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
class SettingsPage;
class GovernancePage;
class StakingPage;
class MiningPage;

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
    void showGovernance();
    void showStaking();
    void showMining();
    void showSettings();
    void showAbout();
    void updateStatus();
    void onConnectionStatusChanged(bool connected);
    void onNetworkTypeChanged(NetworkType type);
    void onBalanceChanged();
    void onBlockHeightChanged(int height);

  private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void connectToRPC();

    // Pages
    QStackedWidget *centralStack;
    OverviewPage    *overviewPage;
    SendPage        *sendPage;
    ReceivePage     *receivePage;
    TransactionPage *transactionPage;
    GovernancePage  *governancePage;
    StakingPage     *stakingPage;
    MiningPage      *miningPage;
    SettingsPage    *settingsPage;

    // Actions
    QAction *overviewAction;
    QAction *sendAction;
    QAction *receiveAction;
    QAction *transactionsAction;
    QAction *governanceAction;
    QAction *stakingAction;
    QAction *miningAction;
    QAction *settingsAction;
    QAction *exitAction;
    QAction *aboutAction;
    QAction *aboutQtAction;

    // Menus
    QMenu *fileMenu;
    QMenu *viewMenu;
    QMenu *toolsMenu;
    QMenu *helpMenu;

    // Toolbar
    QToolBar *toolBar;

    // Status bar widgets
    QLabel *connectionLabel;
    QLabel *networkLabel;
    QLabel *blockHeightLabel;
    QLabel *syncProgressLabel;

    // RPC client
    RPCClient *rpcClient;
};

#endif // MAINWINDOW_H
