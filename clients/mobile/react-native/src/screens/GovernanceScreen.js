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
//  Main governance screen                                              //
// ------------------------------------------------------------------ //
const GovernanceScreen = ({ onBack }) => {
  const [view, setView] = useState('list'); // 'list' | 'detail' | 'submit'
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
        onBack={() => {
          setView('list');
          loadData();
        }}
      />
    );
  }

  if (view === 'submit') {
    return (
      <SubmitProposalForm
        onBack={() => setView('list')}
        onSubmitted={() => {
          setView('list');
          loadData();
        }}
      />
    );
  }

  return (
    <ScrollView style={styles.container}>
      <TouchableOpacity style={styles.backButton} onPress={onBack}>
        <Text style={styles.backButtonText}>← Back</Text>
      </TouchableOpacity>
      <Text style={styles.screenTitle}>Governance</Text>

      <TreasuryPanel treasury={treasury} />

      <View style={styles.sectionHeader}>
        <Text style={styles.sectionTitle}>Proposals</Text>
        <TouchableOpacity
          style={styles.submitButton}
          onPress={() => setView('submit')}>
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
});

export default GovernanceScreen;
