// ParthenonChain Desktop Wallet - Overview Page Header

#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QWidget>

class RPCClient;
class QLabel;
class QPushButton;
class QComboBox;

class OverviewPage : public QWidget {
    Q_OBJECT

  public:
    explicit OverviewPage(RPCClient* rpc, QWidget* parent = nullptr);
    void updateBalances();

  signals:
    void sendRequested();
    void receiveRequested();

  private slots:
    void onAssetChanged(int index);
    void onQuickSend();
    void onQuickReceive();

  private:
    void setupUI();

    RPCClient* rpcClient;
    QComboBox* assetSelector;
    QLabel* balanceLabel;
    QLabel* balanceValueLabel;
    QLabel* talnBalanceLabel;
    QLabel* drmBalanceLabel;
    QLabel* oblBalanceLabel;
    QPushButton* sendButton;
    QPushButton* receiveButton;
    QString currentAsset;
};

#endif  // OVERVIEWPAGE_H
