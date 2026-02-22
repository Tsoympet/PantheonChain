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
      connected(false), blockHeight(0), requestId(1) {

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
