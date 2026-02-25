// ParthenonChain Mobile Wallet - Governance Screen
// Proposal list, proposal detail/voting, submit proposal, treasury summary

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  TextInput,
  ScrollView,
  Alert,
  ActivityIndicator,
} from 'react-native';
import GovernanceService from '../GovernanceService';
import { formatAmount, truncateHash } from '../utils/format';

// ------------------------------------------------------------------ //
//  Treasury panel                                                      //
// ------------------------------------------------------------------ //
const TreasuryPanel = ({ treasury }) => (
  <View style={styles.card}>
    <Text style={styles.cardTitle}>Treasury</Text>
    <View style={styles.treasuryRow}>
      <Text style={styles.treasuryLabel}>Total</Text>
      <Text style={styles.treasuryValue}>{formatAmount(treasury.total, 'TALN')} TALN</Text>
    </View>
    <View style={styles.treasuryRow}>
      <Text style={styles.treasuryLabel}>Core Dev</Text>
      <Text style={styles.treasuryValue}>{formatAmount(treasury.core_development, 'TALN')} TALN</Text>
    </View>
    <View style={styles.treasuryRow}>
      <Text style={styles.treasuryLabel}>Grants</Text>
      <Text style={styles.treasuryValue}>{formatAmount(treasury.grants, 'TALN')} TALN</Text>
    </View>
    <View style={styles.treasuryRow}>
      <Text style={styles.treasuryLabel}>Operations</Text>
      <Text style={styles.treasuryValue}>{formatAmount(treasury.operations, 'TALN')} TALN</Text>
    </View>
    <View style={styles.treasuryRow}>
      <Text style={styles.treasuryLabel}>Emergency</Text>
      <Text style={styles.treasuryValue}>{formatAmount(treasury.emergency, 'TALN')} TALN</Text>
    </View>
  </View>
);

// ------------------------------------------------------------------ //
//  Vote bar                                                             //
// ------------------------------------------------------------------ //
const VoteBar = ({ yes, no, abstain, veto }) => {
  const total = yes + no + abstain + veto || 1;
  const yesPct = (yes / total) * 100;
  const noPct = (no / total) * 100;
  const abstainPct = (abstain / total) * 100;
  const vetoPct = (veto / total) * 100;
  return (
    <View>
      <View style={styles.voteBar}>
        <View style={[styles.voteSegment, { flex: yesPct, backgroundColor: '#28a745' }]} />
        <View style={[styles.voteSegment, { flex: noPct, backgroundColor: '#dc3545' }]} />
        <View style={[styles.voteSegment, { flex: abstainPct, backgroundColor: '#aaa' }]} />
        <View style={[styles.voteSegment, { flex: vetoPct, backgroundColor: '#fd7e14' }]} />
      </View>
      <View style={styles.voteLegend}>
        <Text style={[styles.legendItem, { color: '#28a745' }]}>YES {yes}</Text>
        <Text style={[styles.legendItem, { color: '#dc3545' }]}>NO {no}</Text>
        <Text style={[styles.legendItem, { color: '#aaa' }]}>ABSTAIN {abstain}</Text>
        <Text style={[styles.legendItem, { color: '#fd7e14' }]}>VETO {veto}</Text>
      </View>
    </View>
  );
};

// ------------------------------------------------------------------ //
//  Proposal card (list item)                                           //
// ------------------------------------------------------------------ //
const ProposalCard = ({ proposal, onPress }) => {
  const statusColor = GovernanceService.proposalStatusColor(proposal.status);
  return (
    <TouchableOpacity style={styles.proposalCard} onPress={() => onPress(proposal)}>
      <View style={styles.proposalCardHeader}>
        <Text style={styles.proposalId}>#{proposal.proposal_id}</Text>
        <Text style={[styles.proposalStatus, { color: statusColor }]}>
          {GovernanceService.proposalStatusLabel(proposal.status)}
        </Text>
      </View>
      <Text style={styles.proposalTitle} numberOfLines={2}>
        {proposal.title}
      </Text>
      <Text style={styles.proposalType}>
        {GovernanceService.proposalTypeLabel(proposal.type)}
      </Text>
      <VoteBar
        yes={proposal.yes_votes || 0}
        no={proposal.no_votes || 0}
        abstain={proposal.abstain_votes || 0}
        veto={proposal.veto_votes || 0}
      />
    </TouchableOpacity>
  );
};

// ------------------------------------------------------------------ //
//  Proposal detail + voting                                            //
// ------------------------------------------------------------------ //
const ProposalDetail = ({ proposal: initialProposal, onBack }) => {
  const [proposal, setProposal] = useState(initialProposal);
  const [voting, setVoting] = useState(false);

  const handleVote = async (choice) => {
    Alert.alert(
      'Confirm Vote',
      `Cast vote "${choice}" on proposal #${proposal.proposal_id}?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Vote',
          onPress: async () => {
            try {
              setVoting(true);
              await GovernanceService.castVote({
                proposalId: proposal.proposal_id,
                voter: 'demo_voter',
                choice,
                votingPower: 1,
                signature: 'demo_sig',
              });
              // Refresh tally
              const tally = await GovernanceService.tallyVotes(proposal.proposal_id);
              if (tally) {
                setProposal((prev) => ({ ...prev, ...tally }));
              }
              Alert.alert('Voted', `Your ${choice} vote was recorded.`);
            } catch (err) {
              Alert.alert('Error', err.message || 'Vote failed');
            } finally {
              setVoting(false);
            }
          },
        },
      ]
    );
  };

  const statusColor = GovernanceService.proposalStatusColor(proposal.status);
  const isActive = proposal.status === 'ACTIVE';

  return (
    <ScrollView style={styles.container}>
      <TouchableOpacity style={styles.backButton} onPress={onBack}>
        <Text style={styles.backButtonText}>← Back</Text>
      </TouchableOpacity>

      <Text style={styles.screenTitle}>Proposal #{proposal.proposal_id}</Text>

      <View style={styles.card}>
        <View style={styles.proposalCardHeader}>
          <Text style={styles.proposalType}>
            {GovernanceService.proposalTypeLabel(proposal.type)}
          </Text>
          <Text style={[styles.proposalStatus, { color: statusColor }]}>
            {GovernanceService.proposalStatusLabel(proposal.status)}
          </Text>
        </View>
        <Text style={styles.detailTitle}>{proposal.title}</Text>
        <Text style={styles.detailDescription}>{proposal.description}</Text>
        {proposal.proposer ? (
          <Text style={styles.detailMeta}>
            Proposer: {truncateHash(proposal.proposer)}
          </Text>
        ) : null}
        {proposal.deposit_amount > 0 ? (
          <Text style={styles.detailMeta}>
            Deposit: {formatAmount(proposal.deposit_amount, 'TALN')} TALN
          </Text>
        ) : null}
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Vote Tally</Text>
        <VoteBar
          yes={proposal.yes_votes || 0}
          no={proposal.no_votes || 0}
          abstain={proposal.abstain_votes || 0}
          veto={proposal.veto_votes || 0}
        />
        <Text style={styles.detailMeta}>
          Total votes: {GovernanceService.totalVotes(proposal)}
        </Text>
        {proposal.quorum_requirement > 0 ? (
          <Text style={styles.detailMeta}>
            Quorum required: {proposal.quorum_requirement}
          </Text>
        ) : null}
        {proposal.approval_threshold > 0 ? (
          <Text style={styles.detailMeta}>
            Approval threshold: {proposal.approval_threshold}%
          </Text>
        ) : null}
      </View>

      {isActive && (
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Cast Your Vote</Text>
          {voting ? (
            <ActivityIndicator size="small" color="#007AFF" style={{ marginVertical: 10 }} />
          ) : (
            <View style={styles.voteButtonRow}>
              <TouchableOpacity
                style={[styles.voteButton, { backgroundColor: '#28a745' }]}
                onPress={() => handleVote('YES')}>
                <Text style={styles.voteButtonText}>YES</Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.voteButton, { backgroundColor: '#dc3545' }]}
                onPress={() => handleVote('NO')}>
                <Text style={styles.voteButtonText}>NO</Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.voteButton, { backgroundColor: '#aaa' }]}
                onPress={() => handleVote('ABSTAIN')}>
                <Text style={styles.voteButtonText}>ABSTAIN</Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.voteButton, { backgroundColor: '#fd7e14' }]}
                onPress={() => handleVote('VETO')}>
                <Text style={styles.voteButtonText}>VETO</Text>
              </TouchableOpacity>
            </View>
          )}
        </View>
      )}
    </ScrollView>
  );
};

// ------------------------------------------------------------------ //
//  Submit proposal form                                                //
// ------------------------------------------------------------------ //
const SubmitProposalForm = ({ onBack, onSubmitted }) => {
  const [type, setType] = useState('GENERAL');
  const [title, setTitle] = useState('');
  const [description, setDescription] = useState('');
  const [submitting, setSubmitting] = useState(false);

  const proposalTypes = [
    'GENERAL',
    'PARAMETER_CHANGE',
    'TREASURY_SPENDING',
    'PROTOCOL_UPGRADE',
    'CONSTITUTIONAL',
    'EMERGENCY',
  ];

  const handleSubmit = async () => {
    if (!title.trim()) {
      Alert.alert('Error', 'Title is required');
      return;
    }
    if (!description.trim()) {
      Alert.alert('Error', 'Description is required');
      return;
    }
    try {
      setSubmitting(true);
      const result = await GovernanceService.submitProposal({
        proposer: 'demo_proposer',
        type,
        title: title.trim(),
        description: description.trim(),
      });
      if (result?.proposal_id !== undefined) {
        Alert.alert('Submitted', `Proposal #${result.proposal_id} created.`);
        onSubmitted && onSubmitted(result.proposal_id);
      } else {
        Alert.alert('Error', 'Proposal submission failed.');
      }
    } catch (err) {
      Alert.alert('Error', err.message || 'Submission failed');
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <ScrollView style={styles.container}>
      <TouchableOpacity style={styles.backButton} onPress={onBack}>
        <Text style={styles.backButtonText}>← Back</Text>
      </TouchableOpacity>
      <Text style={styles.screenTitle}>Submit Proposal</Text>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Proposal Type</Text>
        <ScrollView horizontal showsHorizontalScrollIndicator={false} style={styles.typeSelector}>
          {proposalTypes.map((t) => (
            <TouchableOpacity
              key={t}
              style={[styles.typeChip, type === t && styles.typeChipActive]}
              onPress={() => setType(t)}>
              <Text style={[styles.typeChipText, type === t && styles.typeChipTextActive]}>
                {GovernanceService.proposalTypeLabel(t)}
              </Text>
            </TouchableOpacity>
          ))}
        </ScrollView>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Details</Text>
        <TextInput
          style={styles.input}
          placeholder="Title"
          placeholderTextColor="#999"
          value={title}
          onChangeText={setTitle}
          maxLength={120}
        />
        <TextInput
          style={[styles.input, styles.inputMultiline]}
          placeholder="Description"
          placeholderTextColor="#999"
          value={description}
          onChangeText={setDescription}
          multiline
          numberOfLines={5}
          textAlignVertical="top"
        />
      </View>

      <TouchableOpacity
        style={[styles.submitButton, submitting && styles.submitButtonDisabled]}
        onPress={handleSubmit}
        disabled={submitting}>
        <Text style={styles.submitButtonText}>
          {submitting ? 'Submitting…' : 'Submit Proposal'}
        </Text>
      </TouchableOpacity>
    </ScrollView>
  );
};

// ------------------------------------------------------------------ //
//  Roles reference screen                                             //
// ------------------------------------------------------------------ //
const RolesScreen = ({ onBack }) => {
  const sections = [
    {
      title: 'Boule (βουλή) — The Council  [Art. I]',
      body: '500 seats on L3 (OBOLOS). Members selected by VRF sortition (Kleroteria) from the epoch boundary block hash. Term: one epoch (default 14 days), max 4 consecutive terms.\n\nDokimasia screening: min stake ≥ MIN_COUNCIL_STAKE, no slashing in past 4 epochs, uptime ≥ 90%, no active Ostracism.',
    },
    {
      title: 'Prytany (πρυτανεία) — Executive Committee  [Art. I §1.4]',
      body: '50 Boule members randomly selected per epoch. Holds keys to fast-track EMERGENCY proposals. Epistates (presiding officer) chosen daily; may not serve twice in same Prytany term. Cannot simultaneously serve on EmergencyCouncil.',
    },
    {
      title: 'Ekklesia (ἐκκλησία) — The Assembly  [Art. II]',
      body: 'All addresses with positive staked balance on L3 at the proposal snapshot block. Proposal submission requires: stake ≥ MIN_PROPOSAL_STAKE, no active Ostracism, no pending proposal from same address.\n\nQuorum: STANDARD 10% · CONSTITUTIONAL 20% · EMERGENCY 5% · PARAMETER_CHANGE 10% · TREASURY_SPENDING 15%\n\nVoting power = floor(√(staked balance at snapshot)).',
    },
    {
      title: 'EmergencyCouncil  [Art. IX]',
      body: 'M-of-N multi-sig body — default 5-of-9 guardians. Established at genesis; changes require CONSTITUTIONAL proposal.\n\nCan (without prior assembly vote): pause a contract up to 48 h, upgrade contract within 72 h timelock, freeze an address pending Apophasis review.\n\nCannot: confiscate assets, modify supply policy, or override a completed vote.',
    },
    {
      title: 'Apophasis (ἀπόφασις) — Investigative Board  [Art. IX §9.3]',
      body: '5 members selected by VRF from non-Prytany Boule members each epoch. Reviews all EmergencyCouncil actions within 7 days. Publishes on-chain findings report. Recommendations binding when adopted by STANDARD vote within 14 days.',
    },
    {
      title: 'Voting Options  [Art. IV]',
      body: 'YES · NO · ABSTAIN · VETO\n\nVETO: if veto votes exceed 33.34% of all votes cast → unconditional defeat + 14-day re-submission blackout.\n\nDelegation (§4.3): revocable at any time, one level only, no transitive delegation, does not transfer token custody.\n\nVotes are final — changeVote is not available.',
    },
    {
      title: 'Staking Lock Periods  [Art. VII §7.2]',
      body: 'No lock: 1×\n30 days: 1.25×\n90 days: 1.5×\n180 days: 1.75×\n365 days: 2×\n\nLock periods do NOT affect voting power (raw quadratic staked balance) to prevent plutocratic lock-up strategies.',
    },
    {
      title: 'Fee Distribution  [Art. X]',
      body: 'L1 (TALANTON): 60% → Block Producer · 20% → Treasury (OPS) · 20% → Burn\n\nL2 (DRACHMA): 50% → Validator Pool · 20% → Treasury (CORE_DEV) · 20% → L1 Anchor · 10% → Burn\n\nL3 (OBOLOS): 40% → Validator Pool · 20% → Treasury (GRANTS) · 15% → Treasury (CORE_DEV) · 15% → L2 Anchor · 10% → Burn',
    },
  ];

  return (
    <ScrollView style={styles.container}>
      <TouchableOpacity style={styles.backButton} onPress={onBack}>
        <Text style={styles.backButtonText}>← Back</Text>
      </TouchableOpacity>
      <Text style={styles.screenTitle}>Boule & Roles</Text>
      {sections.map((s) => (
        <View key={s.title} style={styles.card}>
          <Text style={styles.cardTitle}>{s.title}</Text>
          <Text style={styles.bodyText}>{s.body}</Text>
        </View>
      ))}
    </ScrollView>
  );
};

// ------------------------------------------------------------------ //
//  Ostracism screen                                                   //
// ------------------------------------------------------------------ //
const OstracismScreen = ({ onBack }) => {
  const [bans, setBans] = useState([]);
  const [loading, setLoading] = useState(true);
  const [target, setTarget] = useState('');
  const [reason, setReason] = useState('');
  const [nominating, setNominating] = useState(false);

  useEffect(() => {
    (async () => {
      setLoading(true);
      const result = await GovernanceService.listActiveBans();
      setBans(result);
      setLoading(false);
    })();
  }, []);

  const handleNominate = async () => {
    if (!target.trim()) { Alert.alert('Error', 'Target address is required'); return; }
    if (!reason.trim()) { Alert.alert('Error', 'Reason is required'); return; }
    Alert.alert('Confirm', `Nominate address for ostracism?\n\n${target.trim()}\n\nReason: ${reason.trim()}`, [
      { text: 'Cancel', style: 'cancel' },
      {
        text: 'Nominate',
        onPress: async () => {
          try {
            setNominating(true);
            const ok = await GovernanceService.nominateOstracism(target.trim(), 'demo_nominator', reason.trim());
            if (ok) {
              Alert.alert('Submitted', 'Ostracism nomination submitted.');
              setTarget(''); setReason('');
              const updated = await GovernanceService.listActiveBans();
              setBans(updated);
            } else {
              Alert.alert('Failed', 'Nomination failed (already nominated or banned).');
            }
          } catch (err) {
            Alert.alert('Error', err.message || 'Nomination failed');
          } finally {
            setNominating(false);
          }
        },
      },
    ]);
  };

  return (
    <ScrollView style={styles.container}>
      <TouchableOpacity style={styles.backButton} onPress={onBack}>
        <Text style={styles.backButtonText}>← Back</Text>
      </TouchableOpacity>
      <Text style={styles.screenTitle}>Ostracism</Text>

      <View style={[styles.card, { borderLeftWidth: 4, borderLeftColor: '#ffc107' }]}>
        <Text style={styles.bodyText}>
          Temporary governance exclusion. An ostracized address may not submit proposals, serve
          on Boule/Prytany, or receive treasury grants — but may still vote, stake, and transact.{'\n\n'}
          Requires CONSTITUTIONAL supermajority (≥66%) with ≥20% quorum.  [Art. VIII]
        </Text>
      </View>

      <View style={styles.sectionHeader}>
        <Text style={styles.sectionTitle}>Active Bans</Text>
      </View>

      {loading ? (
        <ActivityIndicator size="small" color="#007AFF" style={{ marginVertical: 16 }} />
      ) : bans.length === 0 ? (
        <Text style={styles.emptyText}>No active bans.</Text>
      ) : (
        bans.map((ban, i) => (
          <View key={i} style={[styles.card, { borderLeftWidth: 4, borderLeftColor: '#dc3545' }]}>
            <Text style={[styles.bodyText, { fontWeight: '700', color: '#dc3545' }]}>
              {ban.address}
            </Text>
            <Text style={styles.bodyText}>Ban ends at block: {ban.ban_end}</Text>
            <Text style={styles.bodyText}>Reason: {ban.reason || '—'}</Text>
          </View>
        ))
      )}

      <View style={styles.sectionHeader}>
        <Text style={styles.sectionTitle}>Nominate for Ostracism</Text>
      </View>
      <View style={styles.card}>
        <TextInput
          style={styles.input}
          placeholder="Target address (hex)"
          placeholderTextColor="#999"
          value={target}
          onChangeText={setTarget}
        />
        <TextInput
          style={[styles.input, styles.inputMultiline]}
          placeholder="Reason (describe the alleged harm)"
          placeholderTextColor="#999"
          value={reason}
          onChangeText={setReason}
          multiline
          numberOfLines={3}
          textAlignVertical="top"
        />
        <TouchableOpacity
          style={[styles.submitButton, { backgroundColor: '#fd7e14' }, nominating && styles.submitButtonDisabled]}
          onPress={handleNominate}
          disabled={nominating}>
          <Text style={styles.submitButtonText}>
            {nominating ? 'Submitting…' : 'Submit Nomination'}
          </Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
};

// ------------------------------------------------------------------ //
//  Constitutional limits screen                                       //
// ------------------------------------------------------------------ //
const ConstitutionScreen = ({ onBack }) => {
  const isonomia = [
    ['Boule size', '100 – 1 000 seats'],
    ['Council term', '3 – 90 days'],
    ['Prytany size', '10 – 100 members'],
    ['Standard voting window', '3 – 30 days'],
    ['Constitutional voting window', '7 – 60 days'],
    ['Emergency execution TTL', '12 h – 7 days'],
    ['Standard quorum', '5% – 30% staked supply'],
    ['Constitutional quorum', '10% – 40%'],
    ['Supermajority threshold', '60% – 80%'],
    ['Veto threshold', '20% – 45% of votes'],
    ['Min proposal stake', '0.001% – 1% staked supply'],
    ['Min council stake', '0.01% – 5% staked supply'],
    ['Max concurrent proposals', '5 – 100'],
    ['Execution delay (standard)', '1 – 14 days'],
    ['Execution delay (constitutional)', '3 – 30 days'],
    ['Large grant threshold', '0.1% – 10% treasury'],
    ['Slashing — double sign', '1% – 30% stake'],
    ['Slashing — downtime', '0.001% – 5% stake'],
    ['Anti-flash-stake cooldown', '1 block – 14 days'],
    ['Ostracism duration', '30 – 365 days'],
  ];

  const supply = [
    ['TALANTON (TALN)', '21 000 000 (L1)'],
    ['DRACHMA (DRM)', '41 000 000 (L2)'],
    ['OBOLOS (OBL)', '61 000 000 (L3)'],
  ];

  const proposalTypes = [
    ['STANDARD', '>50% · 7-day window · 2-day delay'],
    ['PARAMETER_CHANGE', '>50% · 7-day window · 3-day delay'],
    ['CONSTITUTIONAL', '≥66% supermajority · 14-day window · 7-day delay'],
    ['EMERGENCY', 'Prytany ≥34/50 · assembly ratification within 72 h'],
    ['TREASURY_SPENDING', '>50% · 10-day window · 3-day delay'],
  ];

  const glossary = [
    ['Apophasis (ἀπόφασις)', 'Investigative board reviewing emergency actions'],
    ['Boule (βουλή)', 'Validator council selected by VRF sortition'],
    ['Dokimasia (δοκιμασία)', 'Eligibility screening for council candidates'],
    ['Ekklesia (ἐκκλησία)', 'Full staker assembly — sovereign governance body'],
    ['Epistates (ἐπιστάτης)', 'Presiding officer of Prytany, chosen daily'],
    ['Eunomia (εὐνομία)', 'Good order — governance pipeline structure'],
    ['Isegoria (ἰσηγορία)', 'Equal right of proposal submission'],
    ['Isonomia (ἰσονομία)', 'Constitutional parameter bounds enforceable by code'],
    ['Kleroteria (κληρωτήρια)', 'VRF-based sortition mechanism'],
    ['Ostrakismos (ὀστρακισμός)', 'Community-voted temporary governance exclusion'],
    ['Prytany (πρυτανεία)', 'Executive committee of 50 Boule members'],
    ['Sophrosyne (σωφροσύνη)', 'Prudence — veto and supermajority protections'],
  ];

  const Table = ({ title, rows }) => (
    <View style={styles.card}>
      <Text style={styles.cardTitle}>{title}</Text>
      {rows.map(([param, value]) => (
        <View key={param} style={styles.constitutionRow}>
          <Text style={styles.constitutionParam}>{param}</Text>
          <Text style={styles.constitutionValue}>{value}</Text>
        </View>
      ))}
    </View>
  );

  return (
    <ScrollView style={styles.container}>
      <TouchableOpacity style={styles.backButton} onPress={onBack}>
        <Text style={styles.backButtonText}>← Back</Text>
      </TouchableOpacity>
      <Text style={styles.screenTitle}>Constitution</Text>

      <View style={[styles.card, { borderLeftWidth: 4, borderLeftColor: '#007AFF' }]}>
        <Text style={styles.bodyText}>
          Governing principles: Isonomia · Isegoria · Demokratia · Sophrosyne · Eunomia{'\n'}
          All parameters enforced in code. No proposal may exceed these limits without a hard fork.
        </Text>
      </View>

      <Table title="Art. V — Isonomia Limits (Floor – Ceiling)" rows={isonomia} />
      <Table title="Art. XI — Maximum Token Supplies" rows={supply} />
      <Table title="Art. III — Proposal Types" rows={proposalTypes} />
      <Table title="Appendix B — Glossary" rows={glossary} />
    </ScrollView>
  );
};

// ------------------------------------------------------------------ //
//  Main governance screen                                              //
// ------------------------------------------------------------------ //
const GovernanceScreen = ({ onBack }) => {
  const [view, setView] = useState('list'); // 'list' | 'detail' | 'submit' | 'roles' | 'ostracism' | 'constitution'
  const [proposals, setProposals] = useState([]);
  const [treasury, setTreasury] = useState({
    total: 0,
    core_development: 0,
    grants: 0,
    operations: 0,
    emergency: 0,
  });
  const [selectedProposal, setSelectedProposal] = useState(null);
  const [loading, setLoading] = useState(true);

  const loadData = useCallback(async () => {
    setLoading(true);
    const [props, treas] = await Promise.all([
      GovernanceService.listProposals(),
      GovernanceService.getTreasuryBalance(),
    ]);
    setProposals(props);
    setTreasury(treas);
    setLoading(false);
  }, []);

  useEffect(() => {
    loadData();
  }, [loadData]);

  if (view === 'detail' && selectedProposal) {
    return (
      <ProposalDetail
        proposal={selectedProposal}
        onBack={() => { setView('list'); loadData(); }}
      />
    );
  }

  if (view === 'submit') {
    return (
      <SubmitProposalForm
        onBack={() => setView('list')}
        onSubmitted={() => { setView('list'); loadData(); }}
      />
    );
  }

  if (view === 'roles') {
    return <RolesScreen onBack={() => setView('list')} />;
  }

  if (view === 'ostracism') {
    return <OstracismScreen onBack={() => setView('list')} />;
  }

  if (view === 'constitution') {
    return <ConstitutionScreen onBack={() => setView('list')} />;
  }

  return (
    <ScrollView style={styles.container}>
      <TouchableOpacity style={styles.backButton} onPress={onBack}>
        <Text style={styles.backButtonText}>← Back</Text>
      </TouchableOpacity>
      <Text style={styles.screenTitle}>Governance</Text>

      {/* Quick-nav tabs */}
      <ScrollView horizontal showsHorizontalScrollIndicator={false} style={styles.navTabRow}>
        {[
          { id: 'list', label: '📋 Proposals' },
          { id: 'roles', label: '🏛 Roles' },
          { id: 'ostracism', label: '⚖ Ostracism' },
          { id: 'constitution', label: '📜 Constitution' },
        ].map((tab) => (
          <TouchableOpacity
            key={tab.id}
            style={[styles.navTab, view === tab.id && styles.navTabActive]}
            onPress={() => setView(tab.id)}>
            <Text style={[styles.navTabText, view === tab.id && styles.navTabTextActive]}>
              {tab.label}
            </Text>
          </TouchableOpacity>
        ))}
      </ScrollView>

      <TreasuryPanel treasury={treasury} />

      <View style={styles.sectionHeader}>
        <Text style={styles.sectionTitle}>Proposals</Text>
        <TouchableOpacity style={styles.submitButton} onPress={() => setView('submit')}>
          <Text style={styles.submitButtonText}>+ Submit</Text>
        </TouchableOpacity>
      </View>

      {loading ? (
        <ActivityIndicator size="large" color="#007AFF" style={{ marginTop: 40 }} />
      ) : proposals.length === 0 ? (
        <Text style={styles.emptyText}>No proposals found.</Text>
      ) : (
        proposals.map((p) => (
          <ProposalCard
            key={p.proposal_id}
            proposal={p}
            onPress={(prop) => {
              setSelectedProposal(prop);
              setView('detail');
            }}
          />
        ))
      )}
    </ScrollView>
  );
};

// ------------------------------------------------------------------ //
//  Styles                                                              //
// ------------------------------------------------------------------ //
const styles = StyleSheet.create({
  container: {
    flex: 1,
    padding: 16,
    backgroundColor: '#f5f5f5',
  },
  backButton: {
    marginBottom: 12,
  },
  backButtonText: {
    color: '#007AFF',
    fontSize: 16,
  },
  screenTitle: {
    fontSize: 26,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 16,
  },
  card: {
    backgroundColor: '#fff',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.08,
    shadowRadius: 3,
    elevation: 2,
  },
  cardTitle: {
    fontSize: 14,
    fontWeight: '600',
    color: '#666',
    textTransform: 'uppercase',
    letterSpacing: 0.8,
    marginBottom: 10,
  },
  // Treasury
  treasuryRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 4,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: '#eee',
  },
  treasuryLabel: {
    fontSize: 14,
    color: '#555',
  },
  treasuryValue: {
    fontSize: 14,
    fontWeight: '600',
    color: '#333',
  },
  // Section header
  sectionHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: '700',
    color: '#333',
  },
  // Proposal card
  proposalCard: {
    backgroundColor: '#fff',
    borderRadius: 12,
    padding: 14,
    marginBottom: 10,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.08,
    shadowRadius: 3,
    elevation: 2,
  },
  proposalCardHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 6,
  },
  proposalId: {
    fontSize: 12,
    color: '#888',
    fontWeight: '600',
  },
  proposalStatus: {
    fontSize: 12,
    fontWeight: '700',
  },
  proposalTitle: {
    fontSize: 15,
    fontWeight: '600',
    color: '#222',
    marginBottom: 4,
  },
  proposalType: {
    fontSize: 12,
    color: '#888',
    marginBottom: 8,
  },
  // Vote bar
  voteBar: {
    flexDirection: 'row',
    height: 8,
    borderRadius: 4,
    overflow: 'hidden',
    backgroundColor: '#eee',
    marginBottom: 4,
  },
  voteSegment: {
    height: 8,
  },
  voteLegend: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  legendItem: {
    fontSize: 11,
    fontWeight: '600',
  },
  // Vote buttons
  voteButtonRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: 6,
  },
  voteButton: {
    flex: 1,
    marginHorizontal: 3,
    paddingVertical: 10,
    borderRadius: 8,
    alignItems: 'center',
  },
  voteButtonText: {
    color: '#fff',
    fontWeight: '700',
    fontSize: 12,
  },
  // Detail
  detailTitle: {
    fontSize: 18,
    fontWeight: '700',
    color: '#222',
    marginBottom: 8,
  },
  detailDescription: {
    fontSize: 14,
    color: '#444',
    lineHeight: 20,
    marginBottom: 8,
  },
  detailMeta: {
    fontSize: 12,
    color: '#888',
    marginTop: 4,
  },
  // Submit form
  input: {
    backgroundColor: '#f5f5f5',
    borderRadius: 8,
    padding: 12,
    fontSize: 14,
    borderWidth: 1,
    borderColor: '#ddd',
    marginBottom: 12,
  },
  inputMultiline: {
    height: 100,
  },
  typeSelector: {
    flexDirection: 'row',
  },
  typeChip: {
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 16,
    backgroundColor: '#e0e0e0',
    marginRight: 8,
  },
  typeChipActive: {
    backgroundColor: '#007AFF',
  },
  typeChipText: {
    fontSize: 12,
    color: '#555',
    fontWeight: '600',
  },
  typeChipTextActive: {
    color: '#fff',
  },
  // Submit button
  submitButton: {
    backgroundColor: '#007AFF',
    paddingHorizontal: 16,
    paddingVertical: 10,
    borderRadius: 8,
    alignItems: 'center',
  },
  submitButtonDisabled: {
    backgroundColor: '#aaa',
  },
  submitButtonText: {
    color: '#fff',
    fontWeight: '700',
    fontSize: 14,
  },
  // Empty state
  emptyText: {
    textAlign: 'center',
    color: '#888',
    fontSize: 15,
    marginTop: 40,
  },
  // Body text (roles/constitution)
  bodyText: {
    fontSize: 13,
    color: '#444',
    lineHeight: 19,
  },
  // Nav tab bar (governance main screen)
  navTabRow: {
    flexDirection: 'row',
    marginBottom: 12,
  },
  navTab: {
    paddingHorizontal: 14,
    paddingVertical: 7,
    borderRadius: 16,
    backgroundColor: '#e0e0e0',
    marginRight: 8,
  },
  navTabActive: {
    backgroundColor: '#007AFF',
  },
  navTabText: {
    fontSize: 13,
    fontWeight: '600',
    color: '#555',
  },
  navTabTextActive: {
    color: '#fff',
  },
  // Constitution table rows
  constitutionRow: {
    flexDirection: 'row',
    paddingVertical: 5,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: '#eee',
  },
  constitutionParam: {
    flex: 1,
    fontSize: 12,
    color: '#555',
    fontWeight: '600',
    paddingRight: 8,
  },
  constitutionValue: {
    flex: 1,
    fontSize: 12,
    color: '#333',
  },
});

export default GovernanceScreen;
