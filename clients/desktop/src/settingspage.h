// ParthenonChain Desktop Wallet - Settings Page Header

#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>

class RPCClient;
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

  private:
    void setupUI();
    void loadSettings();

    RPCClient *rpcClient;
    QLineEdit *rpcHostEdit;
    QLineEdit *rpcPortEdit;
    QLineEdit *rpcUserEdit;
    QLineEdit *rpcPasswordEdit;
    QComboBox *networkCombo;
    QCheckBox *autoConnectCheck;
    QLabel *statusLabel;
    QPushButton *saveButton;
    QPushButton *resetButton;
};

#endif // SETTINGSPAGE_H
