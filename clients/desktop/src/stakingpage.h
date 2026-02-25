// ParthenonChain Desktop Wallet - Staking Page Header

#ifndef STAKINGPAGE_H
#define STAKINGPAGE_H

#include <QWidget>

class RPCClient;
class QLabel;
class QLineEdit;
class QPushButton;
class QComboBox;

class StakingPage : public QWidget {
    Q_OBJECT

  public:
    explicit StakingPage(RPCClient *rpc, QWidget *parent = nullptr);

  private slots:
    void onStakeClicked();
    void onUnstakeClicked();
    void onLayerChanged(int index);
    void onStakingPowerUpdated(double power);
    void onStakeConfirmed(const QString &layer, double amount);
    void onUnstakeConfirmed(const QString &layer, double amount);
    void onError(const QString &error);

  private:
    void setupUI();
    void refreshStakingPower();

    RPCClient *rpcClient;
    QComboBox *layerCombo;
    QLineEdit *amountEdit;
    QLineEdit *addressEdit;
    QLabel    *stakingPowerLabel;
    QLabel    *statusLabel;
    QPushButton *stakeButton;
    QPushButton *unstakeButton;
};

#endif // STAKINGPAGE_H
