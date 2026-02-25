// ParthenonChain Mobile Wallet - NetworkService Staking Method Tests

global.fetch = jest.fn();

const NetworkServiceModule = require('../src/NetworkService');
const NetworkService = NetworkServiceModule.default || NetworkServiceModule;

function mockFetch(result, error = null) {
  fetch.mockResolvedValueOnce({
    ok: true,
    json: async () => ({ result, error }),
  });
}

function mockFetchError(message) {
  fetch.mockRejectedValueOnce(new Error(message));
}

describe('NetworkService staking methods', () => {
  beforeEach(() => {
    jest.clearAllMocks();
    NetworkService.connected = false;
    NetworkService.blockHeight = 0;
    NetworkService.rpcUrl = 'http://127.0.0.1:8332';
    NetworkService.requestId = 1;
  });

  // ------------------------------------------------------------------ //
  //  stake                                                               //
  // ------------------------------------------------------------------ //
  describe('stake', () => {
    it('sends staking/stake with correct parameters and returns result', async () => {
      const stakeResult = { status: 'accepted', layer: 'l2', amount: 100 };
      mockFetch(stakeResult);

      const result = await NetworkService.stake('addr1', 100, 'l2');
      expect(result).toEqual(stakeResult);

      const body = JSON.parse(fetch.mock.calls[0][1].body);
      expect(body.method).toBe('staking/stake');
      expect(body.params[0].address).toBe('addr1');
      expect(body.params[0].amount).toBe(100);
      expect(body.params[0].layer).toBe('l2');
    });

    it('defaults layer to l2 when not provided', async () => {
      mockFetch({ status: 'accepted' });
      await NetworkService.stake('addr1', 50);

      const body = JSON.parse(fetch.mock.calls[0][1].body);
      expect(body.params[0].layer).toBe('l2');
    });

    it('supports l3 layer', async () => {
      mockFetch({ status: 'accepted', layer: 'l3' });
      const result = await NetworkService.stake('addr1', 200, 'l3');
      expect(result.layer).toBe('l3');
    });

    it('throws on network error', async () => {
      mockFetchError('connection refused');
      await expect(NetworkService.stake('addr1', 10, 'l2')).rejects.toThrow('connection refused');
    });
  });

  // ------------------------------------------------------------------ //
  //  unstake                                                             //
  // ------------------------------------------------------------------ //
  describe('unstake', () => {
    it('sends staking/unstake with correct parameters and returns result', async () => {
      const unstakeResult = { status: 'queued', layer: 'l2', amount: 50 };
      mockFetch(unstakeResult);

      const result = await NetworkService.unstake('addr1', 50, 'l2');
      expect(result).toEqual(unstakeResult);

      const body = JSON.parse(fetch.mock.calls[0][1].body);
      expect(body.method).toBe('staking/unstake');
      expect(body.params[0].amount).toBe(50);
    });

    it('defaults layer to l2 when not provided', async () => {
      mockFetch({ status: 'queued' });
      await NetworkService.unstake('addr1', 25);

      const body = JSON.parse(fetch.mock.calls[0][1].body);
      expect(body.params[0].layer).toBe('l2');
    });

    it('throws on RPC error', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: null, error: { message: 'Insufficient stake' } }),
      });
      await expect(NetworkService.unstake('addr1', 9999, 'l2')).rejects.toThrow(
        'Insufficient stake'
      );
    });
  });

  // ------------------------------------------------------------------ //
  //  getStakingPower                                                     //
  // ------------------------------------------------------------------ //
  describe('getStakingPower', () => {
    it('returns voting power object on success', async () => {
      const powerResult = { address: 'addr1', voting_power: 500, layer: 'l2' };
      mockFetch(powerResult);

      const result = await NetworkService.getStakingPower('addr1');
      expect(result.voting_power).toBe(500);

      const body = JSON.parse(fetch.mock.calls[0][1].body);
      expect(body.method).toBe('staking/get_power');
      expect(body.params[0].address).toBe('addr1');
    });

    it('returns zero voting_power when result is null', async () => {
      mockFetch(null);
      const result = await NetworkService.getStakingPower('addr1');
      expect(result.voting_power).toBe(0);
    });

    it('returns zero voting_power on network error (does not throw)', async () => {
      mockFetchError('timeout');
      const result = await NetworkService.getStakingPower('addr1');
      expect(result.voting_power).toBe(0);
    });
  });
});
