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
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

static QString proposalTypeLabel(const QString &type) {
    if (type == "PARAMETER_CHANGE")
        return "Parameter Change";
    if (type == "TREASURY_SPENDING")
        return "Treasury Spending";
    if (type == "PROTOCOL_UPGRADE")
        return "Protocol Upgrade";
    if (type == "CONSTITUTIONAL")
        return "Constitutional";
    if (type == "EMERGENCY")
        return "Emergency";
    return "General";
}

static QString proposalStatusLabel(const QString &status) {
    if (status == "ACTIVE")
        return "Active";
    if (status == "PASSED")
        return "Passed";
    if (status == "REJECTED")
        return "Rejected";
    if (status == "EXECUTED")
        return "Executed";
    if (status == "EXPIRED")
        return "Expired";
    return "Pending";
}

// Helper: create a two-column table filled from a QStringList of "key|value" pairs.
static QTableWidget *makeInfoTable(const QStringList &rows, QWidget *parent) {
    QTableWidget *tbl = new QTableWidget(rows.size(), 2, parent);
    tbl->setHorizontalHeaderLabels({"Parameter", "Value"});
    tbl->horizontalHeader()->setStretchLastSection(true);
    tbl->verticalHeader()->setVisible(false);
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->setSelectionMode(QAbstractItemView::NoSelection);
    tbl->setAlternatingRowColors(true);
    for (int i = 0; i < rows.size(); ++i) {
        const QStringList parts = rows[i].split("|");
        tbl->setItem(i, 0, new QTableWidgetItem(parts.value(0).trimmed()));
        tbl->setItem(i, 1, new QTableWidgetItem(parts.value(1).trimmed()));
    }
    tbl->resizeColumnsToContents();
    return tbl;
}

// ---------------------------------------------------------------------------

GovernancePage::GovernancePage(RPCClient *rpc, QWidget *parent)
    : QWidget(parent), rpcClient(rpc), tabWidget(nullptr), selectedProposalId(0) {
    setupUI();

    if (rpcClient) {
        connect(rpcClient, &RPCClient::proposalsUpdated, this, &GovernancePage::onProposalsUpdated);
        connect(rpcClient, &RPCClient::treasuryBalanceUpdated, this,
                &GovernancePage::onTreasuryUpdated);
        connect(rpcClient, &RPCClient::voteCast, this, &GovernancePage::onVoteCast);
        connect(rpcClient, &RPCClient::proposalSubmitted, this,
                &GovernancePage::onProposalSubmitted);
        connect(rpcClient, &RPCClient::activeBansUpdated, this,
                &GovernancePage::onActiveBansUpdated);
        connect(rpcClient, &RPCClient::ostracismNominated, this,
                &GovernancePage::onOstracismNominated);
        connect(rpcClient, &RPCClient::errorOccurred, this, &GovernancePage::onError);
    }
}

void GovernancePage::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(6, 6, 6, 6);

    QLabel *titleLabel = new QLabel(tr("Governance"), this);
    QFont f = titleLabel->font();
    f.setPointSize(18);
    f.setBold(true);
    titleLabel->setFont(f);
    mainLayout->addWidget(titleLabel);

    tabWidget = new QTabWidget(this);

    // ---- Tab 1: Proposals ----
    QWidget *proposalsTab = new QWidget(tabWidget);
    setupProposalsTab(proposalsTab);
    tabWidget->addTab(proposalsTab, tr("Proposals"));

    // ---- Tab 2: Boule & Roles ----
    QWidget *rolesTab = new QWidget(tabWidget);
    setupRolesTab(rolesTab);
    tabWidget->addTab(rolesTab, tr("Boule & Roles"));

    // ---- Tab 3: Ostracism ----
    QWidget *ostracismTab = new QWidget(tabWidget);
    setupOstracismTab(ostracismTab);
    tabWidget->addTab(ostracismTab, tr("Ostracism"));

    // ---- Tab 4: Constitution ----
    QWidget *constitutionTab = new QWidget(tabWidget);
    setupConstitutionTab(constitutionTab);
    tabWidget->addTab(constitutionTab, tr("Constitution"));

    mainLayout->addWidget(tabWidget, 1);
}

// ---------------------------------------------------------------------------
//  Tab 1 — Proposals
// ---------------------------------------------------------------------------
void GovernancePage::setupProposalsTab(QWidget *tab) {
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setSpacing(6);

    // Treasury strip
    QGroupBox *treasuryBox = new QGroupBox(tr("Treasury Balances"), tab);
    QHBoxLayout *tl = new QHBoxLayout(treasuryBox);
    treasuryTotalLabel = new QLabel(tr("Total: 0"), tab);
    treasuryCorDevLabel = new QLabel(tr("Core Dev: 0"), tab);
    treasuryGrantsLabel = new QLabel(tr("Grants: 0"), tab);
    treasuryOpsLabel = new QLabel(tr("Ops: 0"), tab);
    treasuryEmergencyLabel = new QLabel(tr("Emergency: 0"), tab);
    tl->addWidget(treasuryTotalLabel);
    tl->addWidget(treasuryCorDevLabel);
    tl->addWidget(treasuryGrantsLabel);
    tl->addWidget(treasuryOpsLabel);
    tl->addWidget(treasuryEmergencyLabel);
    tl->addStretch();
    layout->addWidget(treasuryBox);

    // Splitter: list | detail+submit
    QSplitter *splitter = new QSplitter(Qt::Horizontal, tab);

    // Left: proposal list
    QWidget *listW = new QWidget(splitter);
    QVBoxLayout *ll = new QVBoxLayout(listW);
    ll->setContentsMargins(0, 0, 4, 0);

    QHBoxLayout *filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel(tr("Status:"), listW));
    statusFilter = new QComboBox(listW);
    statusFilter->addItems({tr("All"), tr("Active"), tr("Passed"), tr("Rejected"), tr("Pending"),
                            tr("Expired"), tr("Executed")});
    connect(statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { loadProposals(); });
    filterRow->addWidget(statusFilter);
    filterRow->addStretch();
    refreshButton = new QPushButton(tr("Refresh"), listW);
    connect(refreshButton, &QPushButton::clicked, this, &GovernancePage::onRefresh);
    filterRow->addWidget(refreshButton);
    ll->addLayout(filterRow);

    proposalTable = new QTableWidget(0, 4, listW);
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
    ll->addWidget(proposalTable);

    // Right: detail + submit
    QWidget *rightW = new QWidget(splitter);
    QVBoxLayout *rl = new QVBoxLayout(rightW);
    rl->setContentsMargins(4, 0, 0, 0);

    QGroupBox *detailBox = new QGroupBox(tr("Proposal Detail"), rightW);
    QVBoxLayout *dl = new QVBoxLayout(detailBox);
    detailIdLabel = new QLabel("—", detailBox);
    dl->addWidget(detailIdLabel);
    detailTypeLabel = new QLabel("—", detailBox);
    dl->addWidget(detailTypeLabel);
    detailStatusLabel = new QLabel("—", detailBox);
    dl->addWidget(detailStatusLabel);
    detailTitleLabel = new QLabel("—", detailBox);
    detailTitleLabel->setWordWrap(true);
    dl->addWidget(detailTitleLabel);
    detailDescLabel = new QLabel("—", detailBox);
    detailDescLabel->setWordWrap(true);
    dl->addWidget(detailDescLabel);
    detailVotesLabel = new QLabel("—", detailBox);
    dl->addWidget(detailVotesLabel);
    detailQuorumLabel = new QLabel("—", detailBox);
    dl->addWidget(detailQuorumLabel);
    voteStatusLabel = new QLabel(detailBox);
    voteStatusLabel->setWordWrap(true);
    dl->addWidget(voteStatusLabel);

    QHBoxLayout *vl = new QHBoxLayout();
    voteYesButton = new QPushButton(tr("YES"), detailBox);
    voteNoButton = new QPushButton(tr("NO"), detailBox);
    voteAbstainButton = new QPushButton(tr("ABSTAIN"), detailBox);
    voteVetoButton = new QPushButton(tr("VETO"), detailBox);
    voteYesButton->setStyleSheet("QPushButton{background:#28a745;color:white;}");
    voteNoButton->setStyleSheet("QPushButton{background:#dc3545;color:white;}");
    voteAbstainButton->setStyleSheet("QPushButton{background:#6c757d;color:white;}");
    voteVetoButton->setStyleSheet("QPushButton{background:#fd7e14;color:white;}");
    voteYesButton->setEnabled(false);
    voteNoButton->setEnabled(false);
    voteAbstainButton->setEnabled(false);
    voteVetoButton->setEnabled(false);
    connect(voteYesButton, &QPushButton::clicked, this, &GovernancePage::onVoteYes);
    connect(voteNoButton, &QPushButton::clicked, this, &GovernancePage::onVoteNo);
    connect(voteAbstainButton, &QPushButton::clicked, this, &GovernancePage::onVoteAbstain);
    connect(voteVetoButton, &QPushButton::clicked, this, &GovernancePage::onVoteVeto);
    vl->addWidget(voteYesButton);
    vl->addWidget(voteNoButton);
    vl->addWidget(voteAbstainButton);
    vl->addWidget(voteVetoButton);
    dl->addLayout(vl);
    rl->addWidget(detailBox);

    QGroupBox *submitBox = new QGroupBox(tr("Submit Proposal"), rightW);
    QVBoxLayout *sl = new QVBoxLayout(submitBox);
    QHBoxLayout *tr2 = new QHBoxLayout();
    tr2->addWidget(new QLabel(tr("Type:"), submitBox));
    proposalTypeCombo = new QComboBox(submitBox);
    proposalTypeCombo->addItems({"GENERAL", "PARAMETER_CHANGE", "TREASURY_SPENDING",
                                 "PROTOCOL_UPGRADE", "CONSTITUTIONAL", "EMERGENCY"});
    tr2->addWidget(proposalTypeCombo);
    tr2->addStretch();
    sl->addLayout(tr2);
    proposalTitleEdit = new QLineEdit(submitBox);
    proposalTitleEdit->setPlaceholderText(tr("Proposal title"));
    sl->addWidget(proposalTitleEdit);
    proposalDescEdit = new QTextEdit(submitBox);
    proposalDescEdit->setPlaceholderText(tr("Proposal description…"));
    proposalDescEdit->setMaximumHeight(70);
    sl->addWidget(proposalDescEdit);
    submitButton = new QPushButton(tr("Submit Proposal"), submitBox);
    connect(submitButton, &QPushButton::clicked, this, &GovernancePage::onSubmitProposal);
    sl->addWidget(submitButton);
    submitStatusLabel = new QLabel(submitBox);
    submitStatusLabel->setWordWrap(true);
    sl->addWidget(submitStatusLabel);
    rl->addWidget(submitBox);
    rl->addStretch();

    splitter->addWidget(listW);
    splitter->addWidget(rightW);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    layout->addWidget(splitter, 1);
}

// ---------------------------------------------------------------------------
//  Tab 2 — Boule & Roles (Constitution-derived, static reference)
// ---------------------------------------------------------------------------
void GovernancePage::setupRolesTab(QWidget *tab) {
    QScrollArea *scroll = new QScrollArea(tab);
    scroll->setWidgetResizable(true);
    QWidget *inner = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(inner);
    layout->setSpacing(10);

    auto addSection = [&](const QString &title, const QString &body) {
        QGroupBox *box = new QGroupBox(title, inner);
        QVBoxLayout *bl = new QVBoxLayout(box);
        QLabel *lbl = new QLabel(body, box);
        lbl->setWordWrap(true);
        lbl->setTextFormat(Qt::RichText);
        bl->addWidget(lbl);
        layout->addWidget(box);
    };

    // Boule (Article I)
    addSection(
        tr("Boule (βουλή) — The Council  [Article I]"),
        tr("<b>Size:</b> 500 seats on Layer 3 (OBOLOS).<br>"
           "<b>Selection:</b> VRF sortition (Kleroteria) using the OBOLOS block hash of the last "
           "block of the preceding epoch. No validator can predict or manipulate selection.<br>"
           "<b>Term:</b> One epoch per term (default 14 days). Maximum 4 consecutive terms before "
           "a mandatory one-term rest.<br>"
           "<b>Screening (Dokimasia):</b> Minimum stake ≥ MIN_COUNCIL_STAKE; no slashing event "
           "in past 4 epochs; uptime ≥ 90%; no active Ostracism.<br>"
           "<b>Removal:</b> Supermajority (66%) assembly vote for fraudulent VRF proof, Ostracism "
           "conviction, or falling below minimum stake."));

    // Prytany (Article I §1.4)
    addSection(
        tr("Prytany (πρυτανεία) — Executive Committee  [Article I §1.4]"),
        tr("<b>Size:</b> 50 Boule members randomly selected at the start of each epoch.<br>"
           "<b>Powers:</b> Holds keys to fast-track EMERGENCY proposals.<br>"
           "<b>Epistates:</b> One presiding officer chosen daily from the Prytany; may not serve "
           "twice in the same Prytany term.<br>"
           "<b>Restriction:</b> Prytany members may not simultaneously serve on the "
           "EmergencyCouncil."));

    // Ekklesia (Article II)
    addSection(
        tr("Ekklesia (ἐκκλησία) — The Assembly  [Article II]"),
        tr("<b>Membership:</b> All addresses with a positive staked balance on L3 at the "
           "proposal snapshot block.<br>"
           "<b>Proposal submission:</b> Requires staked balance ≥ MIN_PROPOSAL_STAKE, no active "
           "Ostracism, and no pending unexecuted proposal from the same address.<br>"
           "<b>Quorum by type:</b><br>"
           "• STANDARD: 10% of total staked supply<br>"
           "• CONSTITUTIONAL: 20%<br>"
           "• EMERGENCY: 5% (Prytany initial vote)<br>"
           "• PARAMETER_CHANGE: 10%<br>"
           "• TREASURY_SPENDING: 15%<br>"
           "<b>Voting power:</b> Quadratic — floor(√(stakedBalance at snapshot)). "
           "Anti-flash-stake cooldown prevents last-minute stake manipulation."));

    // EmergencyCouncil (Article IX)
    addSection(
        tr("EmergencyCouncil  [Article IX]"),
        tr("<b>Structure:</b> M-of-N multi-signature body — default 5-of-9 guardian signers.<br>"
           "<b>Composition:</b> Established at genesis; changes require a CONSTITUTIONAL "
           "proposal.<br>"
           "<b>Powers (without prior assembly vote):</b><br>"
           "• Pause a contract/method for up to EMERGENCY_PAUSE_TTL (default 48 h).<br>"
           "• Upgrade a contract implementation within EMERGENCY_UPGRADE_TTL (default 72 h) "
           "timelock after critical vulnerability disclosure.<br>"
           "• Freeze an address's governance participation pending Apophasis review.<br>"
           "<b>Prohibited:</b> Cannot confiscate staked assets, modify supply policy, or override "
           "a completed assembly vote.<br>"
           "<b>Guardians:</b> Publicly disclosed on-chain; may not simultaneously serve on the "
           "Prytany."));

    // Apophasis (Article IX §9.3)
    addSection(
        tr("Apophasis (ἀπόφασις) — Investigative Board  [Article IX §9.3]"),
        tr("<b>Size:</b> 5 members selected by VRF from non-Prytany Boule members each epoch.<br>"
           "<b>Role:</b><br>"
           "• Review all EmergencyCouncil actions within 7 days of execution.<br>"
           "• Publish a public on-chain findings report.<br>"
           "• Recommend ratification, revocation, or sanctions against guardians.<br>"
           "<b>Binding:</b> Recommendations become binding when adopted by a STANDARD assembly "
           "vote within 14 days of publication."));

    // Voting (Article IV)
    addSection(
        tr("Voting  [Article IV]"),
        tr("<b>Vote options:</b> YES · NO · ABSTAIN · VETO<br>"
           "<b>VETO rule:</b> If veto votes exceed 33.34% of all votes cast, the proposal is "
           "unconditionally defeated and enters a 14-day re-submission blackout.<br>"
           "<b>Delegation (§4.3):</b> A staker may delegate via VotingSystem::delegate(delegatee). "
           "Revocable at any time; limited to one level (no transitive delegation); "
           "does not transfer token custody.<br>"
           "<b>Finality (§4.4):</b> Votes are final once cast; changeVote is not available."));

    // Staking lock periods (Article VII §7.2)
    addSection(
        tr("Staking Lock Periods  [Article VII §7.2]"),
        tr("<table border='1' cellpadding='3'>"
           "<tr><th>Lock Period</th><th>Yield Multiplier</th></tr>"
           "<tr><td>No lock (liquid)</td><td>1×</td></tr>"
           "<tr><td>30 days</td><td>1.25×</td></tr>"
           "<tr><td>90 days</td><td>1.5×</td></tr>"
           "<tr><td>180 days</td><td>1.75×</td></tr>"
           "<tr><td>365 days</td><td>2×</td></tr>"
           "</table>"
           "<br>Lock periods do <b>not</b> affect voting power (which uses raw quadratic staked "
           "balance) to prevent lock-up strategies from amplifying governance influence."));

    // Fee routing (Article X)
    addSection(
        tr("Fee Distribution  [Article X]"),
        tr("<b>Layer 1 (TALANTON):</b><br>"
           "• 60% → L1 Block Producer &nbsp;• 20% → L1 Treasury (OPERATIONS) &nbsp;• 20% → Burn<br>"
           "<b>Layer 2 (DRACHMA):</b><br>"
           "• 50% → L2 Validator Pool &nbsp;• 20% → L2 Treasury (CORE_DEV) "
           "&nbsp;• 20% → L1 Anchor Subsidy &nbsp;• 10% → Burn<br>"
           "<b>Layer 3 (OBOLOS):</b><br>"
           "• 40% → L3 Validator Pool &nbsp;• 20% → L3 Treasury (GRANTS) "
           "&nbsp;• 15% → L3 Treasury (CORE_DEV) &nbsp;• 15% → L2 Anchor Subsidy "
           "&nbsp;• 10% → Burn"));

    layout->addStretch();
    scroll->setWidget(inner);
    QVBoxLayout *tabLayout = new QVBoxLayout(tab);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->addWidget(scroll);
}

// ---------------------------------------------------------------------------
//  Tab 3 — Ostracism (Article VIII, wired to RPC)
// ---------------------------------------------------------------------------
void GovernancePage::setupOstracismTab(QWidget *tab) {
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setSpacing(8);

    // Info banner
    QLabel *info = new QLabel(
        tr("<b>Ostracism (ὀστρακισμός) — Article VIII</b><br>"
           "Community-driven temporary governance exclusion. A successfully ostracized address "
           "may not submit proposals, serve on the Boule/Prytany, or receive treasury grants, "
           "but <i>may</i> continue to vote, stake, transact, and withdraw funds.<br>"
           "Requires CONSTITUTIONAL supermajority (≥66%) with ≥20% quorum."),
        tab);
    info->setWordWrap(true);
    info->setStyleSheet("QLabel{background:#fff8e1;border-left:4px solid #ffc107;"
                        "padding:8px;border-radius:4px;}");
    layout->addWidget(info);

    // Active bans table
    QGroupBox *bansBox = new QGroupBox(tr("Active Bans"), tab);
    QVBoxLayout *bl = new QVBoxLayout(bansBox);
    bansTable = new QTableWidget(0, 3, bansBox);
    bansTable->setHorizontalHeaderLabels({tr("Address"), tr("Ban Ends (Block)"), tr("Reason")});
    bansTable->horizontalHeader()->setStretchLastSection(true);
    bansTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    bansTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    bansTable->setAlternatingRowColors(true);
    bansTable->setColumnWidth(0, 220);
    bansTable->setColumnWidth(1, 130);
    bl->addWidget(bansTable);
    layout->addWidget(bansBox, 1);

    // Nominate form
    QGroupBox *nomBox = new QGroupBox(tr("Nominate for Ostracism"), tab);
    QVBoxLayout *nl = new QVBoxLayout(nomBox);
    QHBoxLayout *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Target address:"), nomBox));
    ostracismTargetEdit = new QLineEdit(nomBox);
    ostracismTargetEdit->setPlaceholderText(tr("Hex address"));
    row1->addWidget(ostracismTargetEdit);
    nl->addLayout(row1);
    QHBoxLayout *row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("Reason:"), nomBox));
    ostracismReasonEdit = new QLineEdit(nomBox);
    ostracismReasonEdit->setPlaceholderText(tr("Describe the alleged harm to the protocol"));
    row2->addWidget(ostracismReasonEdit);
    nl->addLayout(row2);
    ostracismNominateButton = new QPushButton(tr("Submit Nomination"), nomBox);
    ostracismNominateButton->setStyleSheet(
        "QPushButton{background:#fd7e14;color:white;font-weight:bold;}");
    connect(ostracismNominateButton, &QPushButton::clicked, this,
            &GovernancePage::onNominateOstracism);
    nl->addWidget(ostracismNominateButton);
    ostracismStatusLabel = new QLabel(nomBox);
    ostracismStatusLabel->setWordWrap(true);
    nl->addWidget(ostracismStatusLabel);
    layout->addWidget(nomBox);
}

// ---------------------------------------------------------------------------
//  Tab 4 — Constitution (Article V Isonomia limits, static)
// ---------------------------------------------------------------------------
void GovernancePage::setupConstitutionTab(QWidget *tab) {
    QScrollArea *scroll = new QScrollArea(tab);
    scroll->setWidgetResizable(true);
    QWidget *inner = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(inner);
    layout->setSpacing(10);

    // Preamble
    QLabel *preamble = new QLabel(
        tr("<b>PantheonChain Governance Constitution</b><br><br>"
           "Governing principles:<br>"
           "• <b>Isonomia</b> — Equality before the law (all parameters subject to constitutional "
           "floors/ceilings)<br>"
           "• <b>Isegoria</b> — Equal right of speech (any address meeting minimum stake may "
           "submit proposals)<br>"
           "• <b>Demokratia</b> — Power of the people (Ekklesia is the sovereign decision-making "
           "body)<br>"
           "• <b>Sophrosyne</b> — Prudence (veto threshold and supermajority protect the "
           "minority)<br>"
           "• <b>Eunomia</b> — Good order (proposal pipeline enforces mandatory review periods)"),
        inner);
    preamble->setWordWrap(true);
    preamble->setStyleSheet("QLabel{background:#e8f4fd;border-left:4px solid #007AFF;"
                            "padding:10px;border-radius:4px;}");
    layout->addWidget(preamble);

    // Isonomia limits (Article V)
    QGroupBox *isonomiaBox =
        new QGroupBox(tr("Article V: Constitutional Limits (Isonomia)"), inner);
    QVBoxLayout *il = new QVBoxLayout(isonomiaBox);
    QLabel *isonomiaNote = new QLabel(
        tr("Hard-coded in GovernanceConstants.sol. No proposal — including a CONSTITUTIONAL "
           "proposal — may move a parameter outside these absolute limits without a code-level "
           "upgrade (itself requiring a CONSTITUTIONAL proposal + 30-day timelock)."),
        isonomiaBox);
    isonomiaNote->setWordWrap(true);
    il->addWidget(isonomiaNote);

    const QStringList isonomiaRows = {
        "Boule size | 100 seats (floor) – 1 000 seats (ceiling)",
        "Council term length | 3 days – 90 days",
        "Prytany size | 10 members – 100 members",
        "Standard voting window | 3 days – 30 days",
        "Constitutional voting window | 7 days – 60 days",
        "Emergency execution TTL | 12 hours – 7 days",
        "Standard quorum | 5% – 30% of total staked supply",
        "Constitutional quorum | 10% – 40%",
        "Supermajority threshold | 60% – 80% (CONSTITUTIONAL proposals)",
        "Veto threshold | 20% – 45% of total votes",
        "Min proposal stake | 0.001% – 1% of total staked supply",
        "Min council stake | 0.01% – 5% of total staked supply",
        "Max concurrent proposals | 5 – 100",
        "Execution delay (standard) | 1 day – 14 days",
        "Execution delay (constitutional) | 3 days – 30 days",
        "Large grant threshold | 0.1% – 10% of treasury balance",
        "Slashing — double sign | 1% – 30% of validator stake",
        "Slashing — downtime | 0.001% – 5% of validator stake",
        "Anti-flash-stake cooldown | 1 block – 14 days",
        "Ostracism duration | 30 days – 365 days",
    };
    il->addWidget(makeInfoTable(isonomiaRows, isonomiaBox));
    layout->addWidget(isonomiaBox);

    // Supply (Article XI)
    QGroupBox *supplyBox = new QGroupBox(tr("Article XI: Supply Policy"), inner);
    QVBoxLayout *sv = new QVBoxLayout(supplyBox);
    sv->addWidget(new QLabel(
        tr("Maximum supplies are hard-coded at the consensus layer — cannot be changed by any "
           "governance action. Requires a hard fork with community consensus."),
        supplyBox));
    sv->addWidget(makeInfoTable(
        {
            "TALANTON (TALN) | 21 000 000 (Layer 1)",
            "DRACHMA (DRM) | 41 000 000 (Layer 2)",
            "OBOLOS (OBL) | 61 000 000 (Layer 3)",
        },
        supplyBox));
    layout->addWidget(supplyBox);

    // Proposal types quick ref (Article III)
    QGroupBox *typesBox = new QGroupBox(tr("Article III: Proposal Types"), inner);
    QVBoxLayout *tv = new QVBoxLayout(typesBox);
    tv->addWidget(makeInfoTable(
        {
            "STANDARD | >50% non-abstaining votes · 7-day window · 2-day execution delay",
            "PARAMETER_CHANGE | >50% · 7-day window · 3-day execution delay",
            "CONSTITUTIONAL | ≥66% supermajority · 14-day window · 7-day execution delay",
            "EMERGENCY | Prytany ≥34/50 · assembly ratification within 72 h",
            "TREASURY_SPENDING | >50% · 10-day window · 3-day execution delay",
        },
        typesBox));
    layout->addWidget(typesBox);

    // Glossary (Appendix B)
    QGroupBox *glossaryBox = new QGroupBox(tr("Appendix B: Glossary of Greek Terms"), inner);
    QVBoxLayout *gv = new QVBoxLayout(glossaryBox);
    gv->addWidget(makeInfoTable(
        {
            "Apophasis (ἀπόφασις) | Investigative board that reviews emergency actions",
            "Boule (βουλή) | Validator council selected by VRF sortition",
            "Dokimasia (δοκιμασία) | Eligibility screening for council candidates",
            "Ekklesia (ἐκκλησία) | Full staker assembly — sovereign governance body",
            "Epistates (ἐπιστάτης) | Presiding officer of the Prytany, chosen daily",
            "Eunomia (εὐνομία) | Good order — the governance pipeline structure",
            "Isegoria (ἰσηγορία) | Equal right of proposal submission",
            "Isonomia (ἰσονομία) | Constitutional parameter bounds enforceable by code",
            "Kleroteria (κληρωτήρια) | VRF-based sortition mechanism",
            "Ostrakismos (ὀστρακισμός) | Community-voted temporary governance exclusion",
            "Prytany (πρυτανεία) | Executive committee of 50 Boule members",
            "Sophrosyne (σωφροσύνη) | Prudence — veto and supermajority protections",
        },
        glossaryBox));
    layout->addWidget(glossaryBox);

    layout->addStretch();
    scroll->setWidget(inner);
    QVBoxLayout *tabLayout = new QVBoxLayout(tab);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->addWidget(scroll);
}

// ---------------------------------------------------------------------------
//  Slots — Proposals tab
// ---------------------------------------------------------------------------
void GovernancePage::onRefresh() {
    if (rpcClient) {
        rpcClient->listProposals();
        rpcClient->getTreasuryBalance();
        rpcClient->listActiveBans();
    }
}

void GovernancePage::onProposalsUpdated() { loadProposals(); }

void GovernancePage::onTreasuryUpdated() {
    if (!rpcClient)
        return;
    TreasuryBalance bal = rpcClient->treasuryBalance();
    treasuryTotalLabel->setText(tr("Total: %1").arg(bal.total));
    treasuryCorDevLabel->setText(tr("Core Dev: %1").arg(bal.coreDevelopment));
    treasuryGrantsLabel->setText(tr("Grants: %1").arg(bal.grants));
    treasuryOpsLabel->setText(tr("Ops: %1").arg(bal.operations));
    treasuryEmergencyLabel->setText(tr("Emergency: %1").arg(bal.emergency));
}

void GovernancePage::loadProposals() {
    proposalTable->setRowCount(0);
    if (!rpcClient)
        return;
    const QString filter = statusFilter->currentText();
    for (const ProposalRecord &p : rpcClient->proposals()) {
        const QString statusStr = proposalStatusLabel(p.status);
        if (filter != tr("All") && statusStr != filter)
            continue;
        int row = proposalTable->rowCount();
        proposalTable->insertRow(row);
        proposalTable->setItem(row, 0, new QTableWidgetItem(QString::number(p.proposalId)));
        proposalTable->setItem(row, 1, new QTableWidgetItem(proposalTypeLabel(p.type)));
        proposalTable->setItem(row, 2, new QTableWidgetItem(statusStr));
        proposalTable->setItem(row, 3, new QTableWidgetItem(p.title));
        proposalTable->item(row, 0)->setData(Qt::UserRole, static_cast<quint64>(p.proposalId));
    }
}

void GovernancePage::onProposalSelected(int row) {
    if (row < 0 || !rpcClient)
        return;
    auto *idItem = proposalTable->item(row, 0);
    if (!idItem)
        return;
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
    detailVotesLabel->setText(tr("YES: %1  NO: %2  ABSTAIN: %3  VETO: %4")
                                  .arg(p.yesVotes)
                                  .arg(p.noVotes)
                                  .arg(p.abstainVotes)
                                  .arg(p.vetoVotes));
    detailQuorumLabel->setText(
        tr("Quorum: %1  Threshold: %2%").arg(p.quorumRequirement).arg(p.approvalThreshold));
    voteStatusLabel->clear();
    const bool active = (p.status == "ACTIVE");
    voteYesButton->setEnabled(active);
    voteNoButton->setEnabled(active);
    voteAbstainButton->setEnabled(active);
    voteVetoButton->setEnabled(active);
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

void GovernancePage::onVoteYes() {
    if (rpcClient && selectedProposalId)
        rpcClient->castVote(selectedProposalId, "YES");
}
void GovernancePage::onVoteNo() {
    if (rpcClient && selectedProposalId)
        rpcClient->castVote(selectedProposalId, "NO");
}
void GovernancePage::onVoteAbstain() {
    if (rpcClient && selectedProposalId)
        rpcClient->castVote(selectedProposalId, "ABSTAIN");
}
void GovernancePage::onVoteVeto() {
    if (rpcClient && selectedProposalId)
        rpcClient->castVote(selectedProposalId, "VETO");
}

void GovernancePage::onVoteCast(quint64 proposalId, bool success) {
    if (success) {
        voteStatusLabel->setText(tr("Vote recorded for proposal #%1.").arg(proposalId));
        voteStatusLabel->setStyleSheet("QLabel{color:green;}");
        if (rpcClient)
            rpcClient->tallyVotes(proposalId);
    } else {
        voteStatusLabel->setText(tr("Vote failed for proposal #%1.").arg(proposalId));
        voteStatusLabel->setStyleSheet("QLabel{color:red;}");
    }
}

void GovernancePage::onSubmitProposal() {
    if (!rpcClient) {
        submitStatusLabel->setText(tr("Error: Not connected"));
        return;
    }
    const QString title = proposalTitleEdit->text().trimmed();
    const QString desc = proposalDescEdit->toPlainText().trimmed();
    if (title.isEmpty()) {
        submitStatusLabel->setText(tr("Error: Title required"));
        return;
    }
    if (desc.isEmpty()) {
        submitStatusLabel->setText(tr("Error: Description required"));
        return;
    }
    rpcClient->submitProposal(proposalTypeCombo->currentText(), title, desc);
    submitStatusLabel->setText(tr("Submitting…"));
    submitStatusLabel->setStyleSheet("QLabel{color:blue;}");
}

void GovernancePage::onProposalSubmitted(quint64 proposalId) {
    submitStatusLabel->setText(tr("Proposal #%1 submitted!").arg(proposalId));
    submitStatusLabel->setStyleSheet("QLabel{color:green;}");
    proposalTitleEdit->clear();
    proposalDescEdit->clear();
    if (rpcClient)
        rpcClient->listProposals();
}

// ---------------------------------------------------------------------------
//  Slots — Ostracism tab
// ---------------------------------------------------------------------------
void GovernancePage::onActiveBansUpdated() {
    bansTable->setRowCount(0);
    if (!rpcClient)
        return;
    for (const OstracismRecord &r : rpcClient->activeBans()) {
        int row = bansTable->rowCount();
        bansTable->insertRow(row);
        bansTable->setItem(row, 0, new QTableWidgetItem(r.address));
        bansTable->setItem(row, 1, new QTableWidgetItem(QString::number(r.banEndBlock)));
        bansTable->setItem(row, 2, new QTableWidgetItem(r.reason));
    }
}

void GovernancePage::onNominateOstracism() {
    if (!rpcClient) {
        ostracismStatusLabel->setText(tr("Error: Not connected"));
        ostracismStatusLabel->setStyleSheet("QLabel{color:red;}");
        return;
    }
    const QString target = ostracismTargetEdit->text().trimmed();
    const QString reason = ostracismReasonEdit->text().trimmed();
    if (target.isEmpty()) {
        ostracismStatusLabel->setText(tr("Error: Target address required"));
        ostracismStatusLabel->setStyleSheet("QLabel{color:red;}");
        return;
    }
    if (reason.isEmpty()) {
        ostracismStatusLabel->setText(tr("Error: Reason required"));
        ostracismStatusLabel->setStyleSheet("QLabel{color:red;}");
        return;
    }
    rpcClient->nominateOstracism(target, QString(), reason);
    ostracismStatusLabel->setText(tr("Submitting nomination…"));
    ostracismStatusLabel->setStyleSheet("QLabel{color:blue;}");
}

void GovernancePage::onOstracismNominated(bool success) {
    if (success) {
        ostracismStatusLabel->setText(tr("Nomination submitted successfully."));
        ostracismStatusLabel->setStyleSheet("QLabel{color:green;}");
        ostracismTargetEdit->clear();
        ostracismReasonEdit->clear();
        if (rpcClient)
            rpcClient->listActiveBans();
    } else {
        ostracismStatusLabel->setText(tr("Nomination failed (already nominated or banned)."));
        ostracismStatusLabel->setStyleSheet("QLabel{color:red;}");
    }
}

// ---------------------------------------------------------------------------
//  Generic error
// ---------------------------------------------------------------------------
void GovernancePage::onError(const QString &error) {
    voteStatusLabel->setText(tr("Error: %1").arg(error));
    voteStatusLabel->setStyleSheet("QLabel{color:red;}");
    submitStatusLabel->setText(tr("Error: %1").arg(error));
    submitStatusLabel->setStyleSheet("QLabel{color:red;}");
    ostracismStatusLabel->setText(tr("Error: %1").arg(error));
    ostracismStatusLabel->setStyleSheet("QLabel{color:red;}");
}
