// ParthenonChain Desktop Wallet - Transaction Page Header

#ifndef TRANSACTIONPAGE_H
#define TRANSACTIONPAGE_H

#include "rpc_client.h"

#include <QWidget>
class QTableWidget;
class QPushButton;
class QComboBox;

class TransactionPage : public QWidget {
    Q_OBJECT

  public:
    explicit TransactionPage(RPCClient* rpc, QWidget* parent = nullptr);

  private slots:
    void onRefresh();
    void onFilterChanged(int index);
    void onTransactionHistoryUpdated();

  private:
    void setupUI();
    void loadTransactions();

    RPCClient* rpcClient;
    QTableWidget* transactionTable;
    QPushButton* refreshButton;
    QComboBox* filterComboBox;
};

#endif  // TRANSACTIONPAGE_H
