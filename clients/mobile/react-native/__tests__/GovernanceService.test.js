// ParthenonChain Mobile Wallet - GovernanceService Unit Tests

global.fetch = jest.fn();

// We need NetworkService to use the mocked fetch
const NetworkServiceModule = require('../src/NetworkService');
const NetworkService = NetworkServiceModule.default || NetworkServiceModule;

// GovernanceService imports NetworkService as a singleton – require after
// NetworkService so it picks up the same instance.
const GovernanceServiceModule = require('../src/GovernanceService');
const GovernanceService = GovernanceServiceModule.default || GovernanceServiceModule;

function mockFetch(result, error = null) {
  fetch.mockResolvedValueOnce({
    ok: true,
    json: async () => ({ result, error }),
  });
}

function mockFetchError(message) {
  fetch.mockRejectedValueOnce(new Error(message));
}

describe('GovernanceService', () => {
  beforeEach(() => {
    jest.clearAllMocks();
    NetworkService.connected = false;
    NetworkService.blockHeight = 0;
    NetworkService.rpcUrl = 'http://127.0.0.1:8332';
    NetworkService.requestId = 1;
  });

  // ------------------------------------------------------------------ //
  //  listProposals                                                       //
  // ------------------------------------------------------------------ //
  describe('listProposals', () => {
    it('returns the proposals array from the RPC result', async () => {
      const proposals = [
        { proposal_id: 1, title: 'Upgrade protocol', type: 'PROTOCOL_UPGRADE', status: 'ACTIVE' },
        { proposal_id: 2, title: 'Fund audit', type: 'TREASURY_SPENDING', status: 'PENDING' },
      ];
      mockFetch({ proposals });

      const result = await GovernanceService.listProposals();
      expect(result).toHaveLength(2);
      expect(result[0].proposal_id).toBe(1);
    });

    it('returns an empty array when the result has no proposals key', async () => {
      mockFetch({});
      const result = await GovernanceService.listProposals();
      expect(result).toEqual([]);
    });

    it('returns an empty array on network error', async () => {
      mockFetchError('network failure');
      const result = await GovernanceService.listProposals();
      expect(result).toEqual([]);
    });
  });

  // ------------------------------------------------------------------ //
  //  getProposal                                                         //
  // ------------------------------------------------------------------ //
  describe('getProposal', () => {
    it('returns a proposal object on success', async () => {
      const proposal = {
        proposal_id: 3,
        title: 'General motion',
        type: 'GENERAL',
        status: 'ACTIVE',
        yes_votes: 10,
        no_votes: 2,
        abstain_votes: 1,
        veto_votes: 0,
      };
      mockFetch(proposal);

      const result = await GovernanceService.getProposal(3);
      expect(result.proposal_id).toBe(3);
      expect(result.yes_votes).toBe(10);
    });

    it('returns null on error', async () => {
      mockFetchError('timeout');
      const result = await GovernanceService.getProposal(99);
      expect(result).toBeNull();
    });
  });

  // ------------------------------------------------------------------ //
  //  submitProposal                                                      //
  // ------------------------------------------------------------------ //
  describe('submitProposal', () => {
    it('returns the result object with proposal_id on success', async () => {
      mockFetch({ proposal_id: 5 });

      const result = await GovernanceService.submitProposal({
        proposer: 'aabbcc',
        type: 'GENERAL',
        title: 'My proposal',
        description: 'Some text',
      });
      expect(result.proposal_id).toBe(5);
    });

    it('passes the correct method to rpcRequest', async () => {
      mockFetch({ proposal_id: 6 });
      await GovernanceService.submitProposal({
        proposer: 'aabbcc',
        type: 'GENERAL',
        title: 'T',
        description: 'D',
      });
      const body = JSON.parse(fetch.mock.calls[0][1].body);
      expect(body.method).toBe('governance/submit_proposal');
    });

    it('throws on RPC error', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: null, error: { message: 'Forbidden' } }),
      });
      await expect(
        GovernanceService.submitProposal({
          proposer: 'aabbcc',
          type: 'GENERAL',
          title: 'T',
          description: 'D',
        })
      ).rejects.toThrow('Forbidden');
    });
  });

  // ------------------------------------------------------------------ //
  //  castVote                                                            //
  // ------------------------------------------------------------------ //
  describe('castVote', () => {
    it('returns true when the RPC result indicates success', async () => {
      mockFetch({ success: true });
      const ok = await GovernanceService.castVote({
        proposalId: 1,
        voter: 'aabb',
        choice: 'YES',
        votingPower: 100,
        signature: 'ccdd',
      });
      expect(ok).toBe(true);
    });

    it('returns false when success is missing from result', async () => {
      mockFetch({});
      const ok = await GovernanceService.castVote({
        proposalId: 1,
        voter: 'aabb',
        choice: 'NO',
        votingPower: 50,
        signature: 'eeff',
      });
      expect(ok).toBe(false);
    });

    it('passes the correct method to rpcRequest', async () => {
      mockFetch({ success: true });
      await GovernanceService.castVote({
        proposalId: 2,
        voter: 'aabb',
        choice: 'VETO',
        votingPower: 10,
        signature: 'sig',
      });
      const body = JSON.parse(fetch.mock.calls[0][1].body);
      expect(body.method).toBe('governance/vote');
    });

    it('throws on network error', async () => {
      mockFetchError('connection refused');
      await expect(
        GovernanceService.castVote({
          proposalId: 1,
          voter: 'aabb',
          choice: 'ABSTAIN',
          votingPower: 1,
          signature: 'sig',
        })
      ).rejects.toThrow('connection refused');
    });
  });

  // ------------------------------------------------------------------ //
  //  tallyVotes                                                          //
  // ------------------------------------------------------------------ //
  describe('tallyVotes', () => {
    it('returns tally object on success', async () => {
      const tally = { yes_votes: 20, no_votes: 5, abstain_votes: 2, veto_votes: 1 };
      mockFetch(tally);
      const result = await GovernanceService.tallyVotes(1);
      expect(result.yes_votes).toBe(20);
    });

    it('returns null on error', async () => {
      mockFetchError('timeout');
      const result = await GovernanceService.tallyVotes(1);
      expect(result).toBeNull();
    });
  });

  // ------------------------------------------------------------------ //
  //  getTreasuryBalance                                                  //
  // ------------------------------------------------------------------ //
  describe('getTreasuryBalance', () => {
    it('returns treasury balances on success', async () => {
      const bal = { total: 1000, core_development: 400, grants: 300, operations: 200, emergency: 100 };
      mockFetch(bal);
      const result = await GovernanceService.getTreasuryBalance();
      expect(result.total).toBe(1000);
      expect(result.grants).toBe(300);
    });

    it('returns zero balances on error', async () => {
      mockFetchError('network error');
      const result = await GovernanceService.getTreasuryBalance();
      expect(result.total).toBe(0);
      expect(result.emergency).toBe(0);
    });
  });

  // ------------------------------------------------------------------ //
  //  Helper methods                                                      //
  // ------------------------------------------------------------------ //
  describe('proposalTypeLabel', () => {
    it('returns a human-readable label for known types', () => {
      expect(GovernanceService.proposalTypeLabel('GENERAL')).toBe('General');
      expect(GovernanceService.proposalTypeLabel('PARAMETER_CHANGE')).toBe('Parameter Change');
      expect(GovernanceService.proposalTypeLabel('TREASURY_SPENDING')).toBe('Treasury Spending');
      expect(GovernanceService.proposalTypeLabel('PROTOCOL_UPGRADE')).toBe('Protocol Upgrade');
      expect(GovernanceService.proposalTypeLabel('CONSTITUTIONAL')).toBe('Constitutional');
      expect(GovernanceService.proposalTypeLabel('EMERGENCY')).toBe('Emergency');
    });

    it('returns the raw type for an unknown type', () => {
      expect(GovernanceService.proposalTypeLabel('UNKNOWN')).toBe('UNKNOWN');
    });
  });

  describe('proposalStatusLabel', () => {
    it('returns a human-readable label for known statuses', () => {
      expect(GovernanceService.proposalStatusLabel('ACTIVE')).toBe('Active');
      expect(GovernanceService.proposalStatusLabel('PASSED')).toBe('Passed');
      expect(GovernanceService.proposalStatusLabel('REJECTED')).toBe('Rejected');
      expect(GovernanceService.proposalStatusLabel('EXECUTED')).toBe('Executed');
      expect(GovernanceService.proposalStatusLabel('EXPIRED')).toBe('Expired');
      expect(GovernanceService.proposalStatusLabel('PENDING')).toBe('Pending');
    });
  });

  describe('proposalStatusColor', () => {
    it('returns a colour string for each known status', () => {
      ['PENDING', 'ACTIVE', 'PASSED', 'REJECTED', 'EXECUTED', 'EXPIRED'].forEach((s) => {
        const color = GovernanceService.proposalStatusColor(s);
        expect(typeof color).toBe('string');
        expect(color.startsWith('#')).toBe(true);
      });
    });

    it('returns #333 for unknown status', () => {
      expect(GovernanceService.proposalStatusColor('FOOBAR')).toBe('#333');
    });
  });

  describe('totalVotes', () => {
    it('sums all vote types correctly', () => {
      const proposal = { yes_votes: 10, no_votes: 3, abstain_votes: 2, veto_votes: 1 };
      expect(GovernanceService.totalVotes(proposal)).toBe(16);
    });

    it('treats missing vote fields as 0', () => {
      expect(GovernanceService.totalVotes({})).toBe(0);
      expect(GovernanceService.totalVotes({ yes_votes: 5 })).toBe(5);
    });
  });

  // ------------------------------------------------------------------ //
  //  listActiveBans                                                       //
  // ------------------------------------------------------------------ //
  describe('listActiveBans', () => {
    it('returns the bans array from the RPC result', async () => {
      const bans = [
        { address: 'aabb', ban_end: 9000, reason: 'manipulation' },
        { address: 'ccdd', ban_end: 10000, reason: 'fraud' },
      ];
      mockFetch({ bans, count: 2 });
      const result = await GovernanceService.listActiveBans();
      expect(result).toHaveLength(2);
      expect(result[0].address).toBe('aabb');
    });

    it('returns empty array when result has no bans key', async () => {
      mockFetch({});
      const result = await GovernanceService.listActiveBans();
      expect(result).toEqual([]);
    });

    it('returns empty array on network error', async () => {
      mockFetchError('timeout');
      const result = await GovernanceService.listActiveBans();
      expect(result).toEqual([]);
    });

    it('sends ostracism/list_bans to RPC', async () => {
      mockFetch({ bans: [] });
      await GovernanceService.listActiveBans(42);
      const body = JSON.parse(fetch.mock.calls[0][1].body);
      expect(body.method).toBe('ostracism/list_bans');
      expect(body.params[0].block_height).toBe(42);
    });
  });

  // ------------------------------------------------------------------ //
  //  nominateOstracism                                                    //
  // ------------------------------------------------------------------ //
  describe('nominateOstracism', () => {
    it('returns true when nomination succeeds', async () => {
      mockFetch({ success: true });
      const ok = await GovernanceService.nominateOstracism('target', 'nominator', 'reason');
      expect(ok).toBe(true);
    });

    it('returns false when success is false', async () => {
      mockFetch({ success: false });
      const ok = await GovernanceService.nominateOstracism('target', 'nominator', 'reason');
      expect(ok).toBe(false);
    });

    it('sends ostracism/nominate with correct parameters', async () => {
      mockFetch({ success: true });
      await GovernanceService.nominateOstracism('addr_t', 'addr_n', 'my reason', 100);
      const body = JSON.parse(fetch.mock.calls[0][1].body);
      expect(body.method).toBe('ostracism/nominate');
      expect(body.params[0].target).toBe('addr_t');
      expect(body.params[0].reason).toBe('my reason');
      expect(body.params[0].block_height).toBe(100);
    });

    it('throws on network error', async () => {
      mockFetchError('refused');
      await expect(
        GovernanceService.nominateOstracism('t', 'n', 'r')
      ).rejects.toThrow('refused');
    });
  });
});
