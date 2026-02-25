// ParthenonChain Desktop Wallet - Governance Page Header

#ifndef GOVERNANCEPAGE_H
#define GOVERNANCEPAGE_H

#include "rpc_client.h"

#include <QWidget>

class QTabWidget;
class QTableWidget;
class QPushButton;
class QLabel;
class QComboBox;
class QLineEdit;
class QTextEdit;
class QGroupBox;

class GovernancePage : public QWidget {
    Q_OBJECT

  public:
    explicit GovernancePage(RPCClient *rpc, QWidget *parent = nullptr);

  private slots:
    // Proposals tab
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
    // Ostracism tab
    void onActiveBansUpdated();
    void onNominateOstracism();
    void onOstracismNominated(bool success);
    // Generic error
    void onError(const QString &error);

  private:
    void setupUI();
    void setupProposalsTab(QWidget *tab);
    void setupRolesTab(QWidget *tab);
    void setupOstracismTab(QWidget *tab);
    void setupConstitutionTab(QWidget *tab);
    void loadProposals();
    void showProposalDetail(const ProposalRecord &proposal);
    void clearDetail();

    RPCClient *rpcClient;
    QTabWidget *tabWidget;

    // ---- Proposals tab ----
    QTableWidget *proposalTable;
    QPushButton  *refreshButton;
    QComboBox    *statusFilter;
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
    QComboBox    *proposalTypeCombo;
    QLineEdit    *proposalTitleEdit;
    QTextEdit    *proposalDescEdit;
    QPushButton  *submitButton;
    QLabel       *submitStatusLabel;
    QLabel       *treasuryTotalLabel;
    QLabel       *treasuryCorDevLabel;
    QLabel       *treasuryGrantsLabel;
    QLabel       *treasuryOpsLabel;
    QLabel       *treasuryEmergencyLabel;

    // ---- Ostracism tab ----
    QTableWidget *bansTable;
    QLineEdit    *ostracismTargetEdit;
    QLineEdit    *ostracismReasonEdit;
    QPushButton  *ostracismNominateButton;
    QLabel       *ostracismStatusLabel;

    quint64 selectedProposalId;
};

#endif // GOVERNANCEPAGE_H
