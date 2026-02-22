// ParthenonChain Desktop Wallet - Receive Page Header

#ifndef RECEIVEPAGE_H
#define RECEIVEPAGE_H

#include <QWidget>

class RPCClient;
class QLabel;
class QPushButton;
class QLineEdit;

class ReceivePage : public QWidget {
    Q_OBJECT

  public:
    explicit ReceivePage(RPCClient* rpc, QWidget* parent = nullptr);

  private slots:
    void onGenerateAddress();
    void onCopyAddress();
    void onNewAddressReceived(const QString& address);

  private:
    void setupUI();

    RPCClient* rpcClient;
    QLineEdit* addressEdit;
    QPushButton* generateButton;
    QPushButton* copyButton;
    QLabel* qrCodeLabel;
};

#endif  // RECEIVEPAGE_H
