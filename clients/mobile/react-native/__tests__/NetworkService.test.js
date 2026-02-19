// ParthenonChain Mobile Wallet - NetworkService Unit Tests

global.fetch = jest.fn();

const NetworkServiceModule = require('../src/NetworkService');
const NetworkService = NetworkServiceModule.default || NetworkServiceModule;

describe('NetworkService', () => {
  beforeEach(() => {
    jest.clearAllMocks();
    NetworkService.connected = false;
    NetworkService.blockHeight = 0;
    NetworkService.rpcUrl = 'http://127.0.0.1:8332';
    NetworkService.requestId = 1;
  });

  describe('connect', () => {
    it('sets connected to true and stores block height on success', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: 42, error: null }),
      });

      const result = await NetworkService.connect();
      expect(result).toBe(true);
      expect(NetworkService.isConnected()).toBe(true);
      expect(NetworkService.getBlockHeight()).toBe(42);
    });

    it('returns false and sets connected to false on fetch failure', async () => {
      fetch.mockRejectedValueOnce(new Error('connection refused'));

      const result = await NetworkService.connect();
      expect(result).toBe(false);
      expect(NetworkService.isConnected()).toBe(false);
    });

    it('returns false when response.ok is false', async () => {
      fetch.mockResolvedValueOnce({
        ok: false,
        status: 401,
        json: async () => ({}),
      });

      const result = await NetworkService.connect();
      expect(result).toBe(false);
    });

    it('accepts a custom URL override', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: 10, error: null }),
      });

      await NetworkService.connect('http://192.168.1.1:8332');
      expect(NetworkService.rpcUrl).toBe('http://192.168.1.1:8332');
    });
  });

  describe('getBalance', () => {
    it('returns balance object from RPC result', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: { TALN: 10.5, DRM: 200, OBL: 50 }, error: null }),
      });

      const balance = await NetworkService.getBalance();
      expect(balance.TALN).toBe(10.5);
      expect(balance.DRM).toBe(200);
      expect(balance.OBL).toBe(50);
    });

    it('returns zero balances on error', async () => {
      fetch.mockRejectedValueOnce(new Error('network error'));

      const balance = await NetworkService.getBalance();
      expect(balance).toEqual({ TALN: 0, DRM: 0, OBL: 0 });
    });
  });

  describe('sendTransaction', () => {
    it('returns a transaction ID on success', async () => {
      const txid = 'abc123def456';
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: txid, error: null }),
      });

      const result = await NetworkService.sendTransaction('TALN', 'parthenon1qabc', 5.0);
      expect(result).toBe(txid);
    });

    it('includes memo in params when provided', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: 'txid-with-memo', error: null }),
      });

      const result = await NetworkService.sendTransaction('DRM', 'parthenon1qdef', 1.0, 'payment');
      expect(result).toBe('txid-with-memo');

      const body = JSON.parse(fetch.mock.calls[0][1].body);
      expect(body.params).toEqual(['DRM', 'parthenon1qdef', 1.0, 'payment']);
    });

    it('omits memo from params when not provided', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: 'txid-no-memo', error: null }),
      });

      await NetworkService.sendTransaction('OBL', 'parthenon1qghi', 2.5);

      const body = JSON.parse(fetch.mock.calls[0][1].body);
      expect(body.params).toEqual(['OBL', 'parthenon1qghi', 2.5]);
    });

    it('throws on RPC error', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: null, error: { message: 'Insufficient funds' } }),
      });

      await expect(
        NetworkService.sendTransaction('TALN', 'parthenon1qabc', 9999999)
      ).rejects.toThrow('Insufficient funds');
    });
  });

  describe('getTransactions', () => {
    it('returns transaction array from RPC result', async () => {
      const txs = [
        { asset: 'TALN', amount: 1.0, time: 1700000000 },  // 2023-11-14T22:13:20Z
        { asset: 'DRM', amount: -0.5, time: 1700001000 },  // 2023-11-14T22:29:40Z
      ];
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: txs, error: null }),
      });

      const result = await NetworkService.getTransactions(10);
      expect(result).toHaveLength(2);
      expect(result[0].asset).toBe('TALN');
    });

    it('returns empty array on error', async () => {
      fetch.mockRejectedValueOnce(new Error('timeout'));

      const result = await NetworkService.getTransactions();
      expect(result).toEqual([]);
    });
  });

  describe('updateBlockHeight', () => {
    it('updates and returns the new block height', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: 500, error: null }),
      });

      const height = await NetworkService.updateBlockHeight();
      expect(height).toBe(500);
      expect(NetworkService.getBlockHeight()).toBe(500);
    });

    it('returns the previous block height on error', async () => {
      NetworkService.blockHeight = 100;
      fetch.mockRejectedValueOnce(new Error('network error'));

      const height = await NetworkService.updateBlockHeight();
      expect(height).toBe(100);
    });
  });

  describe('rpcRequest', () => {
    it('increments request id with each call', async () => {
      fetch
        .mockResolvedValueOnce({ ok: true, json: async () => ({ result: 1, error: null }) })
        .mockResolvedValueOnce({ ok: true, json: async () => ({ result: 2, error: null }) });

      await NetworkService.rpcRequest('getblockcount');
      await NetworkService.rpcRequest('getblockcount');

      const firstId = JSON.parse(fetch.mock.calls[0][1].body).id;
      const secondId = JSON.parse(fetch.mock.calls[1][1].body).id;
      expect(secondId).toBe(firstId + 1);
    });

    it('sends correct JSON-RPC structure', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: 'ok', error: null }),
      });

      await NetworkService.rpcRequest('getbalance', ['TALN']);

      const body = JSON.parse(fetch.mock.calls[0][1].body);
      expect(body.jsonrpc).toBe('2.0');
      expect(body.method).toBe('getbalance');
      expect(body.params).toEqual(['TALN']);
    });
  });
});
