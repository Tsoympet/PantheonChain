// ParthenonChain Desktop Wallet - Mining Page Header

#ifndef MININGPAGE_H
#define MININGPAGE_H

#include <QWidget>

class RPCClient;
class QLabel;
class QPushButton;
class QSpinBox;
class QProgressBar;

class MiningPage : public QWidget {
    Q_OBJECT

  public:
    explicit MiningPage(RPCClient *rpc, QWidget *parent = nullptr);

  private slots:
    void onToggleMining();
    void onRefreshStats();

  private:
    void setupUI();
    void updateMiningStatus();

    RPCClient *rpcClient;
    bool miningActive;
    quint64 totalShares;
    quint64 acceptedShares;

    QLabel *statusLabel;
    QLabel *hashrateLabel;
    QLabel *sharesLabel;
    QLabel *acceptedLabel;
    QLabel *rejectedLabel;
    QLabel *estimatedEarningsLabel;
    QSpinBox *cpuThreadsSpinBox;
    QPushButton *toggleButton;
    QProgressBar *cpuUsageBar;
};

#endif // MININGPAGE_H
