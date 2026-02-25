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

struct ProposalRecord {
    quint64 proposalId;
    QString type;
    QString status;
    QString title;
    QString description;
    QString proposer;
    quint64 yesVotes;
    quint64 noVotes;
    quint64 abstainVotes;
    quint64 vetoVotes;
    quint64 quorumRequirement;
    quint64 approvalThreshold;
    quint64 depositAmount;
    bool bouleApproved;

    ProposalRecord()
        : proposalId(0), yesVotes(0), noVotes(0), abstainVotes(0), vetoVotes(0),
          quorumRequirement(0), approvalThreshold(50), depositAmount(0), bouleApproved(false) {}
};

struct TreasuryBalance {
    quint64 total;
    quint64 coreDevelopment;
    quint64 grants;
    quint64 operations;
    quint64 emergency;

    TreasuryBalance() : total(0), coreDevelopment(0), grants(0), operations(0), emergency(0) {}
};

class RPCClient : public QObject {
    Q_OBJECT

  public:
    explicit RPCClient(QObject *parent = nullptr);
    ~RPCClient();

    void connectToServer(const QString &host, int port);
    void setCredentials(const QString &user, const QString &password);
    void disconnect();
    bool isConnected() const { return connected; }

    // Balance queries
    double getBalance(const QString &asset) const;
    void updateBalances();

    // Block info
    int getBlockHeight() const { return blockHeight; }
    void updateBlockHeight();

    // Transaction operations
    void sendTransaction(const QString &asset, const QString &address, double amount,
                         const QString &memo = QString());
    void getNewAddress();
    void getTransactionHistory();

    // Access last-fetched transaction list
    QList<TransactionRecord> transactions() const { return transactionList; }

    // ---------------------------------------------------------------- //
    //  Governance                                                        //
    // ---------------------------------------------------------------- //
    void listProposals();
    void getProposal(quint64 proposalId);
    void submitProposal(const QString &type, const QString &title, const QString &description,
                        quint64 depositAmount = 0);
    void castVote(quint64 proposalId, const QString &choice);
    void tallyVotes(quint64 proposalId);
    void getTreasuryBalance();

    QList<ProposalRecord> proposals() const { return proposalList; }
    TreasuryBalance treasuryBalance() const { return lastTreasuryBalance; }

    // ---------------------------------------------------------------- //
    //  Staking                                                           //
    // ---------------------------------------------------------------- //
    void stakeTokens(const QString &address, double amount, const QString &layer);
    void unstakeTokens(const QString &address, double amount, const QString &layer);
    void getStakingPower(const QString &address);
    double getLastStakingPower() const { return lastStakingPower; }

  signals:
    void connectionStatusChanged(bool connected);
    void balanceChanged();
    void blockHeightChanged(int height);
    void transactionSent(const QString &txid);
    void transactionHistoryUpdated();
    void newAddressReceived(const QString &address);
    void errorOccurred(const QString &error);
    // Governance signals
    void proposalsUpdated();
    void proposalUpdated(quint64 proposalId);
    void voteCast(quint64 proposalId, bool success);
    void treasuryBalanceUpdated();
    void proposalSubmitted(quint64 proposalId);
    // Staking signals
    void stakingPowerUpdated(double power);
    void stakeConfirmed(const QString &layer, double amount);
    void unstakeConfirmed(const QString &layer, double amount);

  private slots:
    void handleNetworkReply(QNetworkReply *reply);

  private:
    void sendRPCRequest(const QString &method, const QVariantList &params = QVariantList());

    QNetworkAccessManager *networkManager;
    QString rpcHost;
    int rpcPort;
    bool connected;
    QString rpcUser;
    QString rpcPassword;

    QMap<QString, double> balances;
    int blockHeight;
    int requestId;
    QList<TransactionRecord> transactionList;

    // Governance state
    QList<ProposalRecord> proposalList;
    TreasuryBalance lastTreasuryBalance;

    // Staking state
    double lastStakingPower;
};

#endif // RPC_CLIENT_H
