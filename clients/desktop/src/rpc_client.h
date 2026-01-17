// ParthenonChain Desktop Wallet - RPC Client Header

#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>

class QNetworkAccessManager;
class QNetworkReply;

class RPCClient : public QObject {
    Q_OBJECT

  public:
    explicit RPCClient(QObject* parent = nullptr);
    ~RPCClient();

    void connectToServer(const QString& host, int port);
    void disconnect();
    bool isConnected() const { return connected; }

    // Balance queries
    double getBalance(const QString& asset) const;
    void updateBalances();

    // Block info
    int getBlockHeight() const { return blockHeight; }
    void updateBlockHeight();

    // Transaction operations
    void sendTransaction(const QString& asset, const QString& address, double amount,
                         const QString& memo = QString());
    QString getNewAddress();
    void getTransactionHistory();

  signals:
    void connectionStatusChanged(bool connected);
    void balanceChanged();
    void blockHeightChanged(int height);
    void transactionSent(const QString& txid);
    void transactionHistoryUpdated();
    void errorOccurred(const QString& error);

  private slots:
    void handleNetworkReply(QNetworkReply* reply);

  private:
    void sendRPCRequest(const QString& method, const QVariantList& params = QVariantList());

    QNetworkAccessManager* networkManager;
    QString rpcHost;
    int rpcPort;
    bool connected;

    QMap<QString, double> balances;
    int blockHeight;
    int requestId;
};

#endif  // RPC_CLIENT_H
