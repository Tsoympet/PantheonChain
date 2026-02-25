// ParthenonChain Desktop Wallet - Settings Page Header

#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include "rpc_client.h"

#include <QWidget>

class QLineEdit;
class QLabel;
class QPushButton;
class QComboBox;
class QCheckBox;

class SettingsPage : public QWidget {
    Q_OBJECT

  public:
    explicit SettingsPage(RPCClient *rpc, QWidget *parent = nullptr);

  signals:
    void settingsSaved();

  private slots:
    void onSaveClicked();
    void onResetClicked();
    void onNetworkComboChanged(int index);
    void onDevNetAccessResult(bool granted, const QString &role);
    void onNetworkStatusUpdated();
    void onConnectionStatusChanged(bool connected);

  private:
    void setupUI();
    void loadSettings();
    void updateNetworkStatusBadge();

    RPCClient  *rpcClient;

    // RPC connection
    QLineEdit  *rpcHostEdit;
    QLineEdit  *rpcPortEdit;
    QLineEdit  *rpcUserEdit;
    QLineEdit  *rpcPasswordEdit;

    // Network selector
    QComboBox  *networkCombo;
    QLabel     *networkStatusBadge;   //!< Live coloured badge: "● Mainnet · Connected"
    QLabel     *peerCountLabel;
    QLabel     *latencyLabel;
    QLabel     *nodeVersionLabel;

    // DevNet gate
    QWidget    *devNetGateWidget;     //!< Shown only when Devnet is selected
    QLineEdit  *devNetAddressEdit;    //!< Address used for role check
    QPushButton *verifyDevNetButton;
    QLabel     *devNetStatusLabel;

    // Misc
    QCheckBox  *autoConnectCheck;
    QLabel     *statusLabel;
    QPushButton *saveButton;
    QPushButton *resetButton;

    // Tracks whether devnet access has been verified this session
    bool devNetVerified;
    int  pendingNetworkIndex;   //!< Saved so we can revert if devnet check fails
};

#endif // SETTINGSPAGE_H
