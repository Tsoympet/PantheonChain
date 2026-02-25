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

// -------------------------------------------------------------------- //
//  Network type                                                          //
// -------------------------------------------------------------------- //
enum class NetworkType {
    Mainnet, //!< Production network  (default port 8332)
    Testnet, //!< Public test network (default port 18332)
    Devnet   //!< Developer network   (default port 18443) — role-gated
};

struct NetworkStatus {
    NetworkType network;
    bool connected;
    int blockHeight;
    int peerCount;
    int latencyMs;
    QString nodeVersion;

    NetworkStatus()
        : network(NetworkType::Mainnet), connected(false), blockHeight(0), peerCount(0),
          latencyMs(-1) {}
};

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

struct OstracismRecord {
    QString address;
    quint64 banEndBlock;
    QString reason;

    OstracismRecord() : banEndBlock(0) {}
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

    // ---------------------------------------------------------------- //
    //  Network type management                                           //
    // ---------------------------------------------------------------- //
    /** Switch to a different network; updates the default port automatically. */
    void setNetworkType(NetworkType type);
    NetworkType networkType() const { return currentNetwork; }
    /** Returns the default RPC port for a given network. */
    static int defaultPort(NetworkType type);
    /** Returns a human-readable network name ("Mainnet", "Testnet", "Devnet"). */
    static QString networkName(NetworkType type);

    /** Request live status (peers, latency, version) from the node. */
    void refreshNetworkStatus();
    NetworkStatus lastNetworkStatus() const { return netStatus; }

    /** Verify the caller has a governance role that permits Devnet access. */
    void checkDevNetAccess(const QString &address);

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

    // ---------------------------------------------------------------- //
    //  Ostracism (Article VIII)                                         //
    // ---------------------------------------------------------------- //
    void listActiveBans(quint64 blockHeight = 0);
    void nominateOstracism(const QString &target, const QString &nominator, const QString &reason,
                           quint64 blockHeight = 0);

    QList<OstracismRecord> activeBans() const { return activeBansList; }

  signals:
    void connectionStatusChanged(bool connected);
    void balanceChanged();
    void blockHeightChanged(int height);
    void transactionSent(const QString &txid);
    void transactionHistoryUpdated();
    void newAddressReceived(const QString &address);
    void errorOccurred(const QString &error);
    // Network signals
    void networkTypeChanged(NetworkType type);
    void networkStatusUpdated();
    void devNetAccessResult(bool granted, const QString &role);
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
    // Ostracism signals
    void activeBansUpdated();
    void ostracismNominated(bool success);

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

    NetworkType currentNetwork;
    NetworkStatus netStatus;

    QMap<QString, double> balances;
    int blockHeight;
    int requestId;
    QList<TransactionRecord> transactionList;

    // Governance state
    QList<ProposalRecord> proposalList;
    TreasuryBalance lastTreasuryBalance;

    // Staking state
    double lastStakingPower;

    // Ostracism state
    QList<OstracismRecord> activeBansList;
};

#endif // RPC_CLIENT_H
