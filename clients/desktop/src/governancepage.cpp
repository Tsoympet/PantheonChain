// ParthenonChain Desktop Wallet - Governance Page Implementation

#include "governancepage.h"

#include "rpc_client.h"

#include <QComboBox>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

static QString proposalTypeLabel(const QString &type) {
    if (type == "PARAMETER_CHANGE")  return "Parameter Change";
    if (type == "TREASURY_SPENDING") return "Treasury Spending";
    if (type == "PROTOCOL_UPGRADE")  return "Protocol Upgrade";
    if (type == "CONSTITUTIONAL")    return "Constitutional";
    if (type == "EMERGENCY")         return "Emergency";
    return "General";
}

static QString proposalStatusLabel(const QString &status) {
    if (status == "ACTIVE")   return "Active";
    if (status == "PASSED")   return "Passed";
    if (status == "REJECTED") return "Rejected";
    if (status == "EXECUTED") return "Executed";
    if (status == "EXPIRED")  return "Expired";
    return "Pending";
}

GovernancePage::GovernancePage(RPCClient *rpc, QWidget *parent)
    : QWidget(parent), rpcClient(rpc), selectedProposalId(0) {
    setupUI();

    if (rpcClient) {
        connect(rpcClient, &RPCClient::proposalsUpdated,    this, &GovernancePage::onProposalsUpdated);
        connect(rpcClient, &RPCClient::treasuryBalanceUpdated, this, &GovernancePage::onTreasuryUpdated);
        connect(rpcClient, &RPCClient::voteCast,            this, &GovernancePage::onVoteCast);
        connect(rpcClient, &RPCClient::proposalSubmitted,   this, &GovernancePage::onProposalSubmitted);
        connect(rpcClient, &RPCClient::errorOccurred,       this, &GovernancePage::onError);
    }
}

void GovernancePage::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Page title
    QLabel *titleLabel = new QLabel(tr("Governance"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // ---------------------------------------------------------------- //
    //  Treasury summary (top bar)                                       //
    // ---------------------------------------------------------------- //
    QGroupBox *treasuryBox = new QGroupBox(tr("Treasury"), this);
    QHBoxLayout *treasuryLayout = new QHBoxLayout(treasuryBox);

    treasuryTotalLabel    = new QLabel(tr("Total: 0 TALN"), this);
    treasuryCorDevLabel   = new QLabel(tr("Core Dev: 0"), this);
    treasuryGrantsLabel   = new QLabel(tr("Grants: 0"), this);
    treasuryOpsLabel      = new QLabel(tr("Ops: 0"), this);
    treasuryEmergencyLabel = new QLabel(tr("Emergency: 0"), this);

    treasuryLayout->addWidget(treasuryTotalLabel);
    treasuryLayout->addWidget(treasuryCorDevLabel);
    treasuryLayout->addWidget(treasuryGrantsLabel);
    treasuryLayout->addWidget(treasuryOpsLabel);
    treasuryLayout->addWidget(treasuryEmergencyLabel);
    treasuryLayout->addStretch();

    mainLayout->addWidget(treasuryBox);

    // ---------------------------------------------------------------- //
    //  Horizontal splitter: proposal list | detail + submit             //
    // ---------------------------------------------------------------- //
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    // --- Left: proposal list ---
    QWidget *listWidget = new QWidget(splitter);
    QVBoxLayout *listLayout = new QVBoxLayout(listWidget);
    listLayout->setContentsMargins(0, 0, 4, 0);

    QHBoxLayout *filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel(tr("Status:"), listWidget));
    statusFilter = new QComboBox(listWidget);
    statusFilter->addItems({tr("All"), tr("Active"), tr("Passed"), tr("Rejected"),
                             tr("Pending"), tr("Expired"), tr("Executed")});
    connect(statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { loadProposals(); });
    filterLayout->addWidget(statusFilter);
    filterLayout->addStretch();
    refreshButton = new QPushButton(tr("Refresh"), listWidget);
    connect(refreshButton, &QPushButton::clicked, this, &GovernancePage::onRefresh);
    filterLayout->addWidget(refreshButton);
    listLayout->addLayout(filterLayout);

    proposalTable = new QTableWidget(0, 4, listWidget);
    proposalTable->setHorizontalHeaderLabels({tr("ID"), tr("Type"), tr("Status"), tr("Title")});
    proposalTable->horizontalHeader()->setStretchLastSection(true);
    proposalTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    proposalTable->setSelectionMode(QAbstractItemView::SingleSelection);
    proposalTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    proposalTable->setAlternatingRowColors(true);
    proposalTable->setColumnWidth(0, 50);
    proposalTable->setColumnWidth(1, 130);
    proposalTable->setColumnWidth(2, 90);
    connect(proposalTable, &QTableWidget::currentCellChanged, this,
            [this](int row, int, int, int) { onProposalSelected(row); });
    listLayout->addWidget(proposalTable);

    // --- Right: detail + submit ---
    QWidget *rightWidget = new QWidget(splitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(4, 0, 0, 0);

    // Detail group
    QGroupBox *detailBox = new QGroupBox(tr("Proposal Detail"), rightWidget);
    QVBoxLayout *detailLayout = new QVBoxLayout(detailBox);

    detailIdLabel     = new QLabel(tr("—"), detailBox);
    detailTypeLabel   = new QLabel(tr("—"), detailBox);
    detailStatusLabel = new QLabel(tr("—"), detailBox);
    detailTitleLabel  = new QLabel(tr("—"), detailBox);
    detailTitleLabel->setWordWrap(true);
    detailDescLabel   = new QLabel(tr("—"), detailBox);
    detailDescLabel->setWordWrap(true);
    detailVotesLabel  = new QLabel(tr("—"), detailBox);
    detailQuorumLabel = new QLabel(tr("—"), detailBox);

    detailLayout->addWidget(detailIdLabel);
    detailLayout->addWidget(detailTypeLabel);
    detailLayout->addWidget(detailStatusLabel);
    detailLayout->addWidget(detailTitleLabel);
    detailLayout->addWidget(detailDescLabel);
    detailLayout->addWidget(detailVotesLabel);
    detailLayout->addWidget(detailQuorumLabel);

    voteStatusLabel = new QLabel(this);
    voteStatusLabel->setWordWrap(true);
    detailLayout->addWidget(voteStatusLabel);

    QHBoxLayout *voteLayout = new QHBoxLayout();
    voteYesButton     = new QPushButton(tr("Vote YES"),     detailBox);
    voteNoButton      = new QPushButton(tr("Vote NO"),      detailBox);
    voteAbstainButton = new QPushButton(tr("Vote ABSTAIN"), detailBox);
    voteVetoButton    = new QPushButton(tr("Vote VETO"),    detailBox);

    voteYesButton->setStyleSheet("QPushButton { background-color: #28a745; color: white; }");
    voteNoButton->setStyleSheet("QPushButton { background-color: #dc3545; color: white; }");
    voteAbstainButton->setStyleSheet("QPushButton { background-color: #6c757d; color: white; }");
    voteVetoButton->setStyleSheet("QPushButton { background-color: #fd7e14; color: white; }");

    voteYesButton->setEnabled(false);
    voteNoButton->setEnabled(false);
    voteAbstainButton->setEnabled(false);
    voteVetoButton->setEnabled(false);

    connect(voteYesButton,     &QPushButton::clicked, this, &GovernancePage::onVoteYes);
    connect(voteNoButton,      &QPushButton::clicked, this, &GovernancePage::onVoteNo);
    connect(voteAbstainButton, &QPushButton::clicked, this, &GovernancePage::onVoteAbstain);
    connect(voteVetoButton,    &QPushButton::clicked, this, &GovernancePage::onVoteVeto);

    voteLayout->addWidget(voteYesButton);
    voteLayout->addWidget(voteNoButton);
    voteLayout->addWidget(voteAbstainButton);
    voteLayout->addWidget(voteVetoButton);
    detailLayout->addLayout(voteLayout);

    rightLayout->addWidget(detailBox);

    // Submit proposal group
    QGroupBox *submitBox = new QGroupBox(tr("Submit Proposal"), rightWidget);
    QVBoxLayout *submitLayout = new QVBoxLayout(submitBox);

    QHBoxLayout *typeRow = new QHBoxLayout();
    typeRow->addWidget(new QLabel(tr("Type:"), submitBox));
    proposalTypeCombo = new QComboBox(submitBox);
    proposalTypeCombo->addItems({"GENERAL", "PARAMETER_CHANGE", "TREASURY_SPENDING",
                                  "PROTOCOL_UPGRADE", "CONSTITUTIONAL", "EMERGENCY"});
    typeRow->addWidget(proposalTypeCombo);
    typeRow->addStretch();
    submitLayout->addLayout(typeRow);

    proposalTitleEdit = new QLineEdit(submitBox);
    proposalTitleEdit->setPlaceholderText(tr("Proposal title"));
    submitLayout->addWidget(proposalTitleEdit);

    proposalDescEdit = new QTextEdit(submitBox);
    proposalDescEdit->setPlaceholderText(tr("Proposal description…"));
    proposalDescEdit->setMaximumHeight(80);
    submitLayout->addWidget(proposalDescEdit);

    submitButton = new QPushButton(tr("Submit Proposal"), submitBox);
    connect(submitButton, &QPushButton::clicked, this, &GovernancePage::onSubmitProposal);
    submitLayout->addWidget(submitButton);

    submitStatusLabel = new QLabel(submitBox);
    submitStatusLabel->setWordWrap(true);
    submitLayout->addWidget(submitStatusLabel);

    rightLayout->addWidget(submitBox);
    rightLayout->addStretch();

    splitter->addWidget(listWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    mainLayout->addWidget(splitter, 1);
}

void GovernancePage::onRefresh() {
    if (rpcClient) {
        rpcClient->listProposals();
        rpcClient->getTreasuryBalance();
    } else {
        loadProposals();
    }
}

void GovernancePage::onProposalsUpdated() { loadProposals(); }

void GovernancePage::onTreasuryUpdated() {
    if (!rpcClient) return;
    TreasuryBalance bal = rpcClient->treasuryBalance();
    treasuryTotalLabel->setText(tr("Total: %1 TALN").arg(bal.total));
    treasuryCorDevLabel->setText(tr("Core Dev: %1").arg(bal.coreDevelopment));
    treasuryGrantsLabel->setText(tr("Grants: %1").arg(bal.grants));
    treasuryOpsLabel->setText(tr("Ops: %1").arg(bal.operations));
    treasuryEmergencyLabel->setText(tr("Emergency: %1").arg(bal.emergency));
}

void GovernancePage::loadProposals() {
    proposalTable->setRowCount(0);
    if (!rpcClient) return;

    const QString filterStatus = statusFilter->currentText();
    for (const ProposalRecord &p : rpcClient->proposals()) {
        const QString statusStr = proposalStatusLabel(p.status);
        if (filterStatus != tr("All") && statusStr != filterStatus) continue;

        int row = proposalTable->rowCount();
        proposalTable->insertRow(row);
        proposalTable->setItem(row, 0, new QTableWidgetItem(QString::number(p.proposalId)));
        proposalTable->setItem(row, 1, new QTableWidgetItem(proposalTypeLabel(p.type)));
        proposalTable->setItem(row, 2, new QTableWidgetItem(statusStr));
        proposalTable->setItem(row, 3, new QTableWidgetItem(p.title));

        // Store proposal ID in row
        proposalTable->item(row, 0)->setData(Qt::UserRole, static_cast<quint64>(p.proposalId));
    }
}

void GovernancePage::onProposalSelected(int row) {
    if (row < 0 || !rpcClient) return;
    auto *idItem = proposalTable->item(row, 0);
    if (!idItem) return;
    quint64 id = idItem->data(Qt::UserRole).toULongLong();

    for (const ProposalRecord &p : rpcClient->proposals()) {
        if (p.proposalId == id) {
            selectedProposalId = id;
            showProposalDetail(p);
            return;
        }
    }
}

void GovernancePage::showProposalDetail(const ProposalRecord &p) {
    detailIdLabel->setText(tr("Proposal #%1").arg(p.proposalId));
    detailTypeLabel->setText(tr("Type: %1").arg(proposalTypeLabel(p.type)));
    detailStatusLabel->setText(tr("Status: %1").arg(proposalStatusLabel(p.status)));
    detailTitleLabel->setText(p.title);
    detailDescLabel->setText(p.description.isEmpty() ? tr("(no description)") : p.description);
    detailVotesLabel->setText(
        tr("YES: %1  NO: %2  ABSTAIN: %3  VETO: %4")
            .arg(p.yesVotes).arg(p.noVotes).arg(p.abstainVotes).arg(p.vetoVotes));
    detailQuorumLabel->setText(
        tr("Quorum: %1  Threshold: %2%").arg(p.quorumRequirement).arg(p.approvalThreshold));
    voteStatusLabel->clear();

    const bool isActive = (p.status == "ACTIVE");
    voteYesButton->setEnabled(isActive);
    voteNoButton->setEnabled(isActive);
    voteAbstainButton->setEnabled(isActive);
    voteVetoButton->setEnabled(isActive);
}

void GovernancePage::clearDetail() {
    detailIdLabel->setText("—");
    detailTypeLabel->setText("—");
    detailStatusLabel->setText("—");
    detailTitleLabel->setText("—");
    detailDescLabel->setText("—");
    detailVotesLabel->setText("—");
    detailQuorumLabel->setText("—");
    voteStatusLabel->clear();
    voteYesButton->setEnabled(false);
    voteNoButton->setEnabled(false);
    voteAbstainButton->setEnabled(false);
    voteVetoButton->setEnabled(false);
    selectedProposalId = 0;
}

void GovernancePage::onVoteYes()     { if (rpcClient && selectedProposalId) rpcClient->castVote(selectedProposalId, "YES");     }
void GovernancePage::onVoteNo()      { if (rpcClient && selectedProposalId) rpcClient->castVote(selectedProposalId, "NO");      }
void GovernancePage::onVoteAbstain() { if (rpcClient && selectedProposalId) rpcClient->castVote(selectedProposalId, "ABSTAIN"); }
void GovernancePage::onVoteVeto()    { if (rpcClient && selectedProposalId) rpcClient->castVote(selectedProposalId, "VETO");    }

void GovernancePage::onVoteCast(quint64 proposalId, bool success) {
    if (success) {
        voteStatusLabel->setText(tr("Vote recorded for proposal #%1.").arg(proposalId));
        voteStatusLabel->setStyleSheet("QLabel { color: green; }");
        if (rpcClient) rpcClient->tallyVotes(proposalId);
    } else {
        voteStatusLabel->setText(tr("Vote failed for proposal #%1.").arg(proposalId));
        voteStatusLabel->setStyleSheet("QLabel { color: red; }");
    }
}

void GovernancePage::onSubmitProposal() {
    if (!rpcClient) {
        submitStatusLabel->setText(tr("Error: Not connected"));
        submitStatusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    const QString title = proposalTitleEdit->text().trimmed();
    const QString desc  = proposalDescEdit->toPlainText().trimmed();

    if (title.isEmpty()) {
        submitStatusLabel->setText(tr("Error: Title is required"));
        submitStatusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }
    if (desc.isEmpty()) {
        submitStatusLabel->setText(tr("Error: Description is required"));
        submitStatusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    const QString type = proposalTypeCombo->currentText();
    rpcClient->submitProposal(type, title, desc);
    submitStatusLabel->setText(tr("Submitting proposal…"));
    submitStatusLabel->setStyleSheet("QLabel { color: blue; }");
}

void GovernancePage::onProposalSubmitted(quint64 proposalId) {
    submitStatusLabel->setText(tr("Proposal #%1 submitted successfully!").arg(proposalId));
    submitStatusLabel->setStyleSheet("QLabel { color: green; }");
    proposalTitleEdit->clear();
    proposalDescEdit->clear();
    if (rpcClient) rpcClient->listProposals();
}

void GovernancePage::onError(const QString &error) {
    if (voteYesButton->isEnabled()) {
        voteStatusLabel->setText(tr("Error: %1").arg(error));
        voteStatusLabel->setStyleSheet("QLabel { color: red; }");
    }
    submitStatusLabel->setText(tr("Error: %1").arg(error));
    submitStatusLabel->setStyleSheet("QLabel { color: red; }");
}
