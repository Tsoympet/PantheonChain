// ParthenonChain Desktop Wallet - Send Page Header

#ifndef SENDPAGE_H
#define SENDPAGE_H

#include <QWidget>

class RPCClient;
class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;

class SendPage : public QWidget {
    Q_OBJECT

public:
    explicit SendPage(RPCClient *rpc, QWidget *parent = nullptr);

private slots:
    void onSendClicked();
    void onMaxClicked();
    void onClearClicked();
    void onTransactionSent(const QString &txid);
    void onError(const QString &error);

private:
    void setupUI();
    bool validateInputs();
    
    RPCClient *rpcClient;
    QComboBox *assetSelector;
    QLineEdit *addressEdit;
    QLineEdit *amountEdit;
    QLineEdit *memoEdit;
    QLabel *statusLabel;
    QPushButton *sendButton;
    QPushButton *maxButton;
    QPushButton *clearButton;
};

#endif // SENDPAGE_H
