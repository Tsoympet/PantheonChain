// ParthenonChain Mobile Wallet - NetworkService Unit Tests

global.fetch = jest.fn();

const NetworkServiceModule = require('../src/NetworkService');
const NetworkService = NetworkServiceModule.default || NetworkServiceModule;

describe('NetworkService', () => {
  beforeEach(() => {
    jest.clearAllMocks();
    NetworkService.network      = 'mainnet';
    NetworkService.connected    = false;
    NetworkService.blockHeight  = 0;
    NetworkService.rpcUrl       = 'http://127.0.0.1:8332';
    NetworkService.requestId    = 1;
    NetworkService.peerCount    = 0;
    NetworkService.latencyMs    = -1;
    NetworkService.nodeVersion  = '';
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
        { asset: 'TALN', amount: 1.0, time: 1700000000 },
        { asset: 'DRM', amount: -0.5, time: 1700001000 },
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

  // ------------------------------------------------------------------ //
  //  getNetworkStatus                                                    //
  // ------------------------------------------------------------------ //
  describe('getNetworkStatus', () => {
    it('returns mainnet status when on mainnet', () => {
      const s = NetworkService.getNetworkStatus();
      expect(s.network).toBe('mainnet');
      expect(s.networkName).toBe('Mainnet');
      expect(s.networkColor).toBe('#1f2a44');
      expect(s.connected).toBe(false);
    });

    it('reflects connected state and blockHeight', () => {
      NetworkService.connected   = true;
      NetworkService.blockHeight = 42;
      const s = NetworkService.getNetworkStatus();
      expect(s.connected).toBe(true);
      expect(s.blockHeight).toBe(42);
    });
  });

  // ------------------------------------------------------------------ //
  //  setNetwork                                                          //
  // ------------------------------------------------------------------ //
  describe('setNetwork', () => {
    it('switches to testnet and updates rpcUrl', async () => {
      fetch.mockResolvedValueOnce({ ok: true, json: async () => ({ result: 100 }) });
      await NetworkService.setNetwork('testnet');
      expect(NetworkService.network).toBe('testnet');
      expect(NetworkService.rpcUrl).toBe('http://127.0.0.1:18332');
    });

    it('switches to devnet and updates rpcUrl', async () => {
      fetch.mockResolvedValueOnce({ ok: true, json: async () => ({ result: 5 }) });
      await NetworkService.setNetwork('devnet');
      expect(NetworkService.network).toBe('devnet');
      expect(NetworkService.rpcUrl).toBe('http://127.0.0.1:18443');
    });

    it('returns true when connection succeeds', async () => {
      fetch.mockResolvedValueOnce({ ok: true, json: async () => ({ result: 99 }) });
      const ok = await NetworkService.setNetwork('testnet');
      expect(ok).toBe(true);
      expect(NetworkService.connected).toBe(true);
      expect(NetworkService.blockHeight).toBe(99);
    });

    it('returns false when connection fails', async () => {
      fetch.mockRejectedValueOnce(new Error('connection refused'));
      const ok = await NetworkService.setNetwork('testnet');
      expect(ok).toBe(false);
      expect(NetworkService.connected).toBe(false);
    });

    it('throws for unknown network key', async () => {
      await expect(NetworkService.setNetwork('invalid')).rejects.toThrow('Unknown network');
    });

    it('sends getblockcount to the correct testnet URL', async () => {
      fetch.mockResolvedValueOnce({ ok: true, json: async () => ({ result: 1 }) });
      await NetworkService.setNetwork('testnet');
      expect(fetch.mock.calls[0][0]).toBe('http://127.0.0.1:18332');
    });

    it('getCurrentNetworkConfig returns testnet config after switch', async () => {
      fetch.mockResolvedValueOnce({ ok: true, json: async () => ({ result: 1 }) });
      await NetworkService.setNetwork('testnet');
      const cfg = NetworkService.getCurrentNetworkConfig();
      expect(cfg.name).toBe('Testnet');
      expect(cfg.color).toBe('#fd7e14');
    });
  });

  // ------------------------------------------------------------------ //
  //  checkDevNetAccess                                                   //
  // ------------------------------------------------------------------ //
  describe('checkDevNetAccess', () => {
    it('returns granted=true and role when access is allowed', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: { granted: true, role: 'Boule' } }),
      });
      const result = await NetworkService.checkDevNetAccess('0xabc');
      expect(result.granted).toBe(true);
      expect(result.role).toBe('Boule');
    });

    it('returns granted=false when access is denied', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: { granted: false, role: '' } }),
      });
      const result = await NetworkService.checkDevNetAccess('0xdeadbeef');
      expect(result.granted).toBe(false);
    });

    it('returns granted=false on network error (safe default)', async () => {
      fetch.mockRejectedValueOnce(new Error('timeout'));
      const result = await NetworkService.checkDevNetAccess('0xabc');
      expect(result.granted).toBe(false);
    });

    it('sends network/check_dev_access with correct address', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: { granted: true, role: 'Prytany' } }),
      });
      await NetworkService.checkDevNetAccess('myaddr');
      const body = JSON.parse(fetch.mock.calls[0][1].body);
      expect(body.method).toBe('network/check_dev_access');
      expect(body.params[0].address).toBe('myaddr');
    });

    it('supports all qualifying governance roles', async () => {
      for (const role of ['Boule', 'Prytany', 'EmergencyCouncil', 'Apophasis']) {
        fetch.mockResolvedValueOnce({
          ok: true,
          json: async () => ({ result: { granted: true, role } }),
        });
        const result = await NetworkService.checkDevNetAccess('addr');
        expect(result.granted).toBe(true);
        expect(result.role).toBe(role);
      }
    });
  });

  // ------------------------------------------------------------------ //
  //  refreshNetworkStatus                                                //
  // ------------------------------------------------------------------ //
  describe('refreshNetworkStatus', () => {
    it('updates peerCount, latencyMs, nodeVersion from RPC response', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: { peer_count: 12, latency_ms: 45, version: 'parthenon/1.0.0' } }),
      });
      const s = await NetworkService.refreshNetworkStatus();
      expect(NetworkService.peerCount).toBe(12);
      expect(NetworkService.latencyMs).toBe(45);
      expect(NetworkService.nodeVersion).toBe('parthenon/1.0.0');
      expect(s.peerCount).toBe(12);
    });

    it('returns current status even on RPC error (does not throw)', async () => {
      fetch.mockRejectedValueOnce(new Error('timeout'));
      NetworkService.peerCount = 7;
      const s = await NetworkService.refreshNetworkStatus();
      expect(s).toBeDefined();
      expect(NetworkService.peerCount).toBe(7);
    });

    it('sends network/status to the RPC endpoint', async () => {
      fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ result: {} }),
      });
      await NetworkService.refreshNetworkStatus();
      const body = JSON.parse(fetch.mock.calls[0][1].body);
      expect(body.method).toBe('network/status');
    });
  });
});


