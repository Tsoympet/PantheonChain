// ParthenonChain Desktop Wallet - RPC Client Header

#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

#include <QDateTime>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>

class QNetworkAccessManager;
class QNetworkReply;

struct TransactionRecord {
    QString dateTime;
    QString type;
    QString asset;
    double amount;
    QString address;
    QString txid;
};

class RPCClient : public QObject {
    Q_OBJECT

  public:
    explicit RPCClient(QObject* parent = nullptr);
    ~RPCClient();

    void connectToServer(const QString& host, int port);
    void setCredentials(const QString& user, const QString& password);
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
    void getNewAddress();
    void getTransactionHistory();

    // Access last-fetched transaction list
    QList<TransactionRecord> transactions() const { return transactionList; }

  signals:
    void connectionStatusChanged(bool connected);
    void balanceChanged();
    void blockHeightChanged(int height);
    void transactionSent(const QString& txid);
    void transactionHistoryUpdated();
    void newAddressReceived(const QString& address);
    void errorOccurred(const QString& error);

  private slots:
    void handleNetworkReply(QNetworkReply* reply);

  private:
    void sendRPCRequest(const QString& method, const QVariantList& params = QVariantList());

    QNetworkAccessManager* networkManager;
    QString rpcHost;
    int rpcPort;
    bool connected;
    QString rpcUser;
    QString rpcPassword;

    QMap<QString, double> balances;
    int blockHeight;
    int requestId;
    QList<TransactionRecord> transactionList;
};

#endif  // RPC_CLIENT_H
