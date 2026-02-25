// ParthenonChain Desktop Wallet - Governance Page Header

#ifndef GOVERNANCEPAGE_H
#define GOVERNANCEPAGE_H

#include "rpc_client.h"

#include <QWidget>

class QTableWidget;
class QPushButton;
class QLabel;
class QComboBox;
class QLineEdit;
class QTextEdit;
class QStackedWidget;
class QGroupBox;

class GovernancePage : public QWidget {
    Q_OBJECT

  public:
    explicit GovernancePage(RPCClient *rpc, QWidget *parent = nullptr);

  private slots:
    void onRefresh();
    void onProposalsUpdated();
    void onTreasuryUpdated();
    void onProposalSelected(int row);
    void onVoteYes();
    void onVoteNo();
    void onVoteAbstain();
    void onVoteVeto();
    void onSubmitProposal();
    void onVoteCast(quint64 proposalId, bool success);
    void onProposalSubmitted(quint64 proposalId);
    void onError(const QString &error);

  private:
    void setupUI();
    void loadProposals();
    void showProposalDetail(const ProposalRecord &proposal);
    void clearDetail();

    RPCClient *rpcClient;

    // Proposal list panel
    QTableWidget *proposalTable;
    QPushButton  *refreshButton;
    QComboBox    *statusFilter;

    // Detail panel
    QLabel       *detailIdLabel;
    QLabel       *detailTypeLabel;
    QLabel       *detailStatusLabel;
    QLabel       *detailTitleLabel;
    QLabel       *detailDescLabel;
    QLabel       *detailVotesLabel;
    QLabel       *detailQuorumLabel;
    QPushButton  *voteYesButton;
    QPushButton  *voteNoButton;
    QPushButton  *voteAbstainButton;
    QPushButton  *voteVetoButton;
    QLabel       *voteStatusLabel;

    // Submit proposal panel
    QComboBox    *proposalTypeCombo;
    QLineEdit    *proposalTitleEdit;
    QTextEdit    *proposalDescEdit;
    QPushButton  *submitButton;
    QLabel       *submitStatusLabel;

    // Treasury panel
    QLabel       *treasuryTotalLabel;
    QLabel       *treasuryCorDevLabel;
    QLabel       *treasuryGrantsLabel;
    QLabel       *treasuryOpsLabel;
    QLabel       *treasuryEmergencyLabel;

    quint64 selectedProposalId;
};

#endif // GOVERNANCEPAGE_H
