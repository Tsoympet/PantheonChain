// ParthenonChain Mobile Wallet - Governance Service
// Handles all governance-related RPC communication

import NetworkService from './NetworkService';

class GovernanceService {
  // ------------------------------------------------------------------ //
  //  Proposals                                                           //
  // ------------------------------------------------------------------ //

  /**
   * Fetch the list of active proposals from the node.
   * @returns {Promise<Array>} Array of proposal objects
   */
  async listProposals() {
    try {
      const response = await NetworkService.rpcRequest('governance/list_proposals');
      return response.result?.proposals || [];
    } catch (error) {
      console.error('GovernanceService.listProposals error:', error);
      return [];
    }
  }

  /**
   * Fetch a single proposal by ID.
   * @param {number} proposalId
   * @returns {Promise<Object|null>}
   */
  async getProposal(proposalId) {
    try {
      const response = await NetworkService.rpcRequest('governance/get_proposal', [
        { proposal_id: proposalId },
      ]);
      return response.result || null;
    } catch (error) {
      console.error('GovernanceService.getProposal error:', error);
      return null;
    }
  }

  /**
   * Submit a new governance proposal.
   * @param {Object} params
   * @param {string} params.proposer   - hex-encoded proposer address
   * @param {string} params.type       - ProposalType string (e.g. 'GENERAL')
   * @param {string} params.title
   * @param {string} params.description
   * @param {number} [params.deposit]  - deposit amount (anti-spam collateral)
   * @returns {Promise<Object|null>}  { proposal_id } on success
   */
  async submitProposal({ proposer, type, title, description, deposit = 0 }) {
    try {
      const response = await NetworkService.rpcRequest('governance/submit_proposal', [
        { proposer, type, title, description, deposit_amount: deposit },
      ]);
      return response.result || null;
    } catch (error) {
      console.error('GovernanceService.submitProposal error:', error);
      throw error;
    }
  }

  // ------------------------------------------------------------------ //
  //  Voting                                                              //
  // ------------------------------------------------------------------ //

  /**
   * Cast a vote on a proposal.
   * @param {Object} params
   * @param {number} params.proposalId
   * @param {string} params.voter        - hex-encoded voter address
   * @param {string} params.choice       - 'YES' | 'NO' | 'ABSTAIN' | 'VETO'
   * @param {number} params.votingPower
   * @param {string} params.signature    - hex-encoded signature
   * @returns {Promise<boolean>}
   */
  async castVote({ proposalId, voter, choice, votingPower, signature }) {
    try {
      const response = await NetworkService.rpcRequest('governance/vote', [
        {
          proposal_id: proposalId,
          voter,
          choice,
          voting_power: votingPower,
          signature,
        },
      ]);
      return response.result?.success === true;
    } catch (error) {
      console.error('GovernanceService.castVote error:', error);
      throw error;
    }
  }

  /**
   * Request a tally update for a proposal and return the refreshed vote counts.
   * @param {number} proposalId
   * @returns {Promise<Object|null>}  { yes_votes, no_votes, abstain_votes, veto_votes }
   */
  async tallyVotes(proposalId) {
    try {
      const response = await NetworkService.rpcRequest('governance/tally', [
        { proposal_id: proposalId },
      ]);
      return response.result || null;
    } catch (error) {
      console.error('GovernanceService.tallyVotes error:', error);
      return null;
    }
  }

  // ------------------------------------------------------------------ //
  //  Treasury                                                            //
  // ------------------------------------------------------------------ //

  /**
   * Fetch treasury balances broken down by track.
   * @returns {Promise<Object>}  { total, core_development, grants, operations, emergency }
   */
  async getTreasuryBalance() {
    try {
      const response = await NetworkService.rpcRequest('treasury/balance');
      return (
        response.result || {
          total: 0,
          core_development: 0,
          grants: 0,
          operations: 0,
          emergency: 0,
        }
      );
    } catch (error) {
      console.error('GovernanceService.getTreasuryBalance error:', error);
      return { total: 0, core_development: 0, grants: 0, operations: 0, emergency: 0 };
    }
  }

  // ------------------------------------------------------------------ //
  //  Helpers                                                             //
  // ------------------------------------------------------------------ //

  /**
   * Human-readable label for a ProposalType string.
   */
  proposalTypeLabel(type) {
    const labels = {
      PARAMETER_CHANGE: 'Parameter Change',
      TREASURY_SPENDING: 'Treasury Spending',
      PROTOCOL_UPGRADE: 'Protocol Upgrade',
      GENERAL: 'General',
      CONSTITUTIONAL: 'Constitutional',
      EMERGENCY: 'Emergency',
    };
    return labels[type] || type;
  }

  /**
   * Human-readable label for a ProposalStatus string.
   */
  proposalStatusLabel(status) {
    const labels = {
      PENDING: 'Pending',
      ACTIVE: 'Active',
      PASSED: 'Passed',
      REJECTED: 'Rejected',
      EXECUTED: 'Executed',
      EXPIRED: 'Expired',
    };
    return labels[status] || status;
  }

  /**
   * Returns a colour hint string for a status (for styling).
   */
  proposalStatusColor(status) {
    const colors = {
      PENDING: '#888',
      ACTIVE: '#007AFF',
      PASSED: '#28a745',
      REJECTED: '#dc3545',
      EXECUTED: '#6f42c1',
      EXPIRED: '#aaa',
    };
    return colors[status] || '#333';
  }

  /**
   * Compute the total votes cast for a proposal object.
   */
  totalVotes(proposal) {
    return (
      (proposal.yes_votes || 0) +
      (proposal.no_votes || 0) +
      (proposal.abstain_votes || 0) +
      (proposal.veto_votes || 0)
    );
  }
}

export default new GovernanceService();
