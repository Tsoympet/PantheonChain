// ParthenonChain Desktop Wallet - RPC Client Implementation

#include "rpc_client.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

RPCClient::RPCClient(QObject *parent)
    : QObject(parent), networkManager(nullptr), rpcHost("127.0.0.1"), rpcPort(8332),
      connected(false), blockHeight(0), requestId(1), lastStakingPower(0.0),
      currentNetwork(NetworkType::Mainnet) {

    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &RPCClient::handleNetworkReply);

    // Initialize balances
    balances["TALN"] = 0.0;
    balances["DRM"] = 0.0;
    balances["OBL"] = 0.0;
}

RPCClient::~RPCClient() {}

void RPCClient::setCredentials(const QString &user, const QString &password) {
    rpcUser = user;
    rpcPassword = password;
}

void RPCClient::connectToServer(const QString &host, int port) {
    rpcHost = host;
    rpcPort = port;

    // Test connection by getting block height
    sendRPCRequest("getblockcount");
}

void RPCClient::disconnect() {
    connected = false;
    emit connectionStatusChanged(false);
}

double RPCClient::getBalance(const QString &asset) const { return balances.value(asset, 0.0); }

void RPCClient::updateBalances() { sendRPCRequest("getbalance"); }

void RPCClient::updateBlockHeight() { sendRPCRequest("getblockcount"); }

void RPCClient::sendTransaction(const QString &asset, const QString &address, double amount,
                                const QString &memo) {
    QVariantList params;
    params << asset << address << amount;
    if (!memo.isEmpty()) {
        params << memo;
    }
    sendRPCRequest("sendtoaddress", params);
}

void RPCClient::getNewAddress() { sendRPCRequest("getnewaddress"); }

void RPCClient::getTransactionHistory() { sendRPCRequest("listtransactions"); }

void RPCClient::handleNetworkReply(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        connected = false;
        emit connectionStatusChanged(false);
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        reply->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();

    // Check for RPC errors
    if (obj.contains("error") && !obj["error"].isNull()) {
        emit errorOccurred(obj["error"].toObject()["message"].toString());
        reply->deleteLater();
        return;
    }

    // Handle successful response
    if (!connected) {
        connected = true;
        emit connectionStatusChanged(true);
    }

    // Parse result based on method (stored in request property)
    QString method = reply->property("method").toString();
    QJsonValue result = obj["result"];

    if (method == "getblockcount") {
        blockHeight = result.toInt();
        emit blockHeightChanged(blockHeight);
    } else if (method == "getbalance") {
        if (result.isObject()) {
            QJsonObject balObj = result.toObject();
            balances["TALN"] = balObj["TALN"].toDouble();
            balances["DRM"] = balObj["DRM"].toDouble();
            balances["OBL"] = balObj["OBL"].toDouble();
            emit balanceChanged();
        }
    } else if (method == "getnewaddress") {
        if (result.isString()) {
            emit newAddressReceived(result.toString());
        }
    } else if (method == "sendtoaddress") {
        emit transactionSent(result.toString());
    } else if (method == "listtransactions") {
        if (result.isArray()) {
            transactionList.clear();
            for (const auto &item : result.toArray()) {
                QJsonObject txObj = item.toObject();
                TransactionRecord rec;
                QString category = txObj["category"].toString();
                rec.type = (category == "send") ? "Sent" : "Received";
                rec.asset = txObj["asset"].toString("TALN");
                rec.amount = txObj["amount"].toDouble();
                rec.address = txObj["address"].toString();
                rec.txid = txObj["txid"].toString();
                qint64 time = static_cast<qint64>(txObj["time"].toDouble());
                rec.dateTime = QDateTime::fromSecsSinceEpoch(time).toString("yyyy-MM-dd hh:mm:ss");
                transactionList.append(rec);
            }
        }
        emit transactionHistoryUpdated();
    } else if (method == "governance/list_proposals") {
        proposalList.clear();
        QJsonObject resultObj = result.toObject();
        QJsonArray proposals = resultObj["proposals"].toArray();
        for (const auto &item : proposals) {
            QJsonObject p = item.toObject();
            ProposalRecord rec;
            rec.proposalId = static_cast<quint64>(p["proposal_id"].toDouble());
            rec.type = p["type"].toString();
            rec.status = p["status"].toString();
            rec.title = p["title"].toString();
            rec.description = p["description"].toString();
            rec.proposer = p["proposer"].toString();
            rec.yesVotes = static_cast<quint64>(p["yes_votes"].toDouble());
            rec.noVotes = static_cast<quint64>(p["no_votes"].toDouble());
            rec.abstainVotes = static_cast<quint64>(p["abstain_votes"].toDouble());
            rec.vetoVotes = static_cast<quint64>(p["veto_votes"].toDouble());
            rec.quorumRequirement = static_cast<quint64>(p["quorum_requirement"].toDouble());
            rec.approvalThreshold = static_cast<quint64>(p["approval_threshold"].toDouble());
            rec.depositAmount = static_cast<quint64>(p["deposit_amount"].toDouble());
            rec.bouleApproved = p["boule_approved"].toBool();
            proposalList.append(rec);
        }
        emit proposalsUpdated();
    } else if (method == "governance/get_proposal") {
        if (result.isObject()) {
            QJsonObject p = result.toObject();
            quint64 id = static_cast<quint64>(p["proposal_id"].toDouble());
            // Update or insert in proposalList
            bool found = false;
            for (auto &rec : proposalList) {
                if (rec.proposalId == id) {
                    rec.yesVotes = static_cast<quint64>(p["yes_votes"].toDouble());
                    rec.noVotes = static_cast<quint64>(p["no_votes"].toDouble());
                    rec.abstainVotes = static_cast<quint64>(p["abstain_votes"].toDouble());
                    rec.vetoVotes = static_cast<quint64>(p["veto_votes"].toDouble());
                    rec.status = p["status"].toString();
                    found = true;
                    break;
                }
            }
            if (!found) {
                ProposalRecord rec;
                rec.proposalId = id;
                rec.type = p["type"].toString();
                rec.status = p["status"].toString();
                rec.title = p["title"].toString();
                rec.description = p["description"].toString();
                rec.yesVotes = static_cast<quint64>(p["yes_votes"].toDouble());
                rec.noVotes = static_cast<quint64>(p["no_votes"].toDouble());
                rec.abstainVotes = static_cast<quint64>(p["abstain_votes"].toDouble());
                rec.vetoVotes = static_cast<quint64>(p["veto_votes"].toDouble());
                proposalList.append(rec);
            }
            emit proposalUpdated(id);
        }
    } else if (method == "governance/submit_proposal") {
        quint64 id = static_cast<quint64>(result.toObject()["proposal_id"].toDouble());
        emit proposalSubmitted(id);
    } else if (method == "governance/vote") {
        quint64 id = static_cast<quint64>(result.toObject()["proposal_id"].toDouble());
        bool ok = result.toObject()["success"].toBool();
        emit voteCast(id, ok);
    } else if (method == "governance/tally") {
        // Tally triggers a refresh of the proposal
        quint64 id = static_cast<quint64>(result.toObject()["proposal_id"].toDouble());
        emit proposalUpdated(id);
    } else if (method == "treasury/balance") {
        if (result.isObject()) {
            QJsonObject b = result.toObject();
            lastTreasuryBalance.total = static_cast<quint64>(b["total"].toDouble());
            lastTreasuryBalance.coreDevelopment =
                static_cast<quint64>(b["core_development"].toDouble());
            lastTreasuryBalance.grants = static_cast<quint64>(b["grants"].toDouble());
            lastTreasuryBalance.operations = static_cast<quint64>(b["operations"].toDouble());
            lastTreasuryBalance.emergency = static_cast<quint64>(b["emergency"].toDouble());
            emit treasuryBalanceUpdated();
        }
    } else if (method == "staking/stake") {
        double amount = result.toObject()["amount"].toDouble();
        QString layer = result.toObject()["layer"].toString();
        emit stakeConfirmed(layer, amount);
    } else if (method == "staking/unstake") {
        double amount = result.toObject()["amount"].toDouble();
        QString layer = result.toObject()["layer"].toString();
        emit unstakeConfirmed(layer, amount);
    } else if (method == "staking/get_power") {
        lastStakingPower = result.toObject()["voting_power"].toDouble();
        emit stakingPowerUpdated(lastStakingPower);
    } else if (method == "ostracism/list_bans") {
        activeBansList.clear();
        QJsonObject resultObj = result.toObject();
        QJsonArray bans = resultObj["bans"].toArray();
        for (const auto &item : bans) {
            QJsonObject b = item.toObject();
            OstracismRecord rec;
            rec.address = b["address"].toString();
            rec.banEndBlock = static_cast<quint64>(b["ban_end"].toDouble());
            rec.reason = b["reason"].toString();
            activeBansList.append(rec);
        }
        emit activeBansUpdated();
    } else if (method == "ostracism/nominate") {
        bool ok = result.toObject()["success"].toBool();
        emit ostracismNominated(ok);
    } else if (method == "network/status") {
        QJsonObject s = result.toObject();
        netStatus.connected = true;
        netStatus.peerCount = s["peer_count"].toInt();
        netStatus.latencyMs = s["latency_ms"].toInt(-1);
        netStatus.nodeVersion = s["version"].toString();
        netStatus.network = currentNetwork;
        emit networkStatusUpdated();
    } else if (method == "network/check_dev_access") {
        bool granted = result.toObject()["granted"].toBool();
        QString role = result.toObject()["role"].toString();
        emit devNetAccessResult(granted, role);
    }

    reply->deleteLater();
}

void RPCClient::sendRPCRequest(const QString &method, const QVariantList &params) {
    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = requestId++;
    request["method"] = method;
    request["params"] = QJsonArray::fromVariantList(params);

    QJsonDocument doc(request);
    QByteArray data = doc.toJson();

    QUrl url(QString("http://%1:%2").arg(rpcHost).arg(rpcPort));
    QNetworkRequest netRequest(url);
    netRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (!rpcUser.isEmpty() || !rpcPassword.isEmpty()) {
        const QString credentials = rpcUser + ":" + rpcPassword;
        const QByteArray credentialsBase64 = credentials.toUtf8().toBase64();
        netRequest.setRawHeader("Authorization", "Basic " + credentialsBase64);
    }

    QNetworkReply *reply = networkManager->post(netRequest, data);
    reply->setProperty("method", method);
}

// -------------------------------------------------------------------- //
//  Governance methods                                                    //
// -------------------------------------------------------------------- //

void RPCClient::listProposals() { sendRPCRequest("governance/list_proposals"); }

void RPCClient::getProposal(quint64 proposalId) {
    QVariantMap params;
    params["proposal_id"] = proposalId;
    sendRPCRequest("governance/get_proposal", {params});
}

void RPCClient::submitProposal(const QString &type, const QString &title,
                               const QString &description, quint64 depositAmount) {
    QVariantMap params;
    params["proposer"] = QString(); // filled in by the node from auth credentials
    params["type"] = type;
    params["title"] = title;
    params["description"] = description;
    params["deposit_amount"] = depositAmount;
    sendRPCRequest("governance/submit_proposal", {params});
}

void RPCClient::castVote(quint64 proposalId, const QString &choice) {
    QVariantMap params;
    params["proposal_id"] = proposalId;
    params["choice"] = choice;
    params["voter"] = QString(); // filled in by the node from auth credentials
    params["voting_power"] = 1;
    params["signature"] = QString();
    sendRPCRequest("governance/vote", {params});
}

void RPCClient::tallyVotes(quint64 proposalId) {
    QVariantMap params;
    params["proposal_id"] = proposalId;
    sendRPCRequest("governance/tally", {params});
}

void RPCClient::getTreasuryBalance() { sendRPCRequest("treasury/balance"); }

// -------------------------------------------------------------------- //
//  Staking methods                                                       //
// -------------------------------------------------------------------- //

void RPCClient::stakeTokens(const QString &address, double amount, const QString &layer) {
    QVariantMap params;
    params["address"] = address;
    params["amount"] = amount;
    params["layer"] = layer;
    sendRPCRequest("staking/stake", {params});
}

void RPCClient::unstakeTokens(const QString &address, double amount, const QString &layer) {
    QVariantMap params;
    params["address"] = address;
    params["amount"] = amount;
    params["layer"] = layer;
    sendRPCRequest("staking/unstake", {params});
}

void RPCClient::getStakingPower(const QString &address) {
    QVariantMap params;
    params["address"] = address;
    sendRPCRequest("staking/get_power", {params});
}

// -------------------------------------------------------------------- //
//  Ostracism methods (Article VIII)                                      //
// -------------------------------------------------------------------- //

void RPCClient::listActiveBans(quint64 blockHeight) {
    QVariantMap params;
    params["block_height"] = blockHeight;
    sendRPCRequest("ostracism/list_bans", {params});
}

void RPCClient::nominateOstracism(const QString &target, const QString &nominator,
                                  const QString &reason, quint64 blockHeight) {
    QVariantMap params;
    params["target"] = target;
    params["nominator"] = nominator;
    params["reason"] = reason;
    params["block_height"] = blockHeight;
    sendRPCRequest("ostracism/nominate", {params});
}

// -------------------------------------------------------------------- //
//  Network type management                                               //
// -------------------------------------------------------------------- //

int RPCClient::defaultPort(NetworkType type) {
    switch (type) {
    case NetworkType::Testnet:
        return 18332;
    case NetworkType::Devnet:
        return 18443;
    default:
        return 8332;
    }
}

QString RPCClient::networkName(NetworkType type) {
    switch (type) {
    case NetworkType::Testnet:
        return QStringLiteral("Testnet");
    case NetworkType::Devnet:
        return QStringLiteral("Devnet");
    default:
        return QStringLiteral("Mainnet");
    }
}

void RPCClient::setNetworkType(NetworkType type) {
    if (currentNetwork == type)
        return;
    currentNetwork = type;
    netStatus.network = type;
    // Update port to network default (caller may override afterward with connectToServer)
    rpcPort = defaultPort(type);
    connected = false;
    emit networkTypeChanged(type);
    emit connectionStatusChanged(false);
    sendRPCRequest("getblockcount");
}

void RPCClient::refreshNetworkStatus() { sendRPCRequest("network/status"); }

void RPCClient::checkDevNetAccess(const QString &address) {
    QVariantMap params;
    params["address"] = address;
    sendRPCRequest("network/check_dev_access", {params});
}
