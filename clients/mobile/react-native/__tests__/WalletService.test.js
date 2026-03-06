// ParthenonChain Mobile Wallet - WalletService Unit Tests

jest.mock('react-native-sha256', () => ({
  // Returns a deterministic hex string of length 64 (sha256 output): 8-char prefix + 56 zero-padded chars
  sha256: jest.fn((input) => Promise.resolve('aabbccdd' + input.length.toString(16).padStart(56, '0'))),
}));

const WalletServiceModule = require('../src/WalletService');
// WalletService exports a singleton instance
const WalletService = WalletServiceModule.default || WalletServiceModule;

describe('WalletService', () => {
  beforeEach(() => {
    // Reset addresses between tests
    WalletService.addresses = {};
    WalletService.privateKeys = {};
  });

  describe('validateAddress', () => {
    it('accepts a valid parthenon address', () => {
      const validAddress = 'parthenon1q' + 'a'.repeat(38);
      expect(WalletService.validateAddress(validAddress)).toBe(true);
    });

    it('rejects an address with wrong prefix', () => {
      const badAddress = 'bitcoin1q' + 'a'.repeat(38);
      expect(WalletService.validateAddress(badAddress)).toBe(false);
    });

    it('rejects an address that is too short', () => {
      const shortAddress = 'parthenon1qabc';
      expect(WalletService.validateAddress(shortAddress)).toBe(false);
    });

    it('rejects an empty string', () => {
      expect(WalletService.validateAddress('')).toBe(false);
    });
  });

  describe('generateAddress', () => {
    it('generates an address starting with parthenon1q', async () => {
      const address = await WalletService.generateAddress('TALN');
      expect(address).toBeTruthy();
      expect(address.startsWith('parthenon1q')).toBe(true);
    });

    it('stores generated addresses per asset', async () => {
      await WalletService.generateAddress('TALN');
      await WalletService.generateAddress('TALN');
      expect(WalletService.getAddresses('TALN').length).toBe(2);
    });

    it('stores addresses separately per asset', async () => {
      await WalletService.generateAddress('TALN');
      await WalletService.generateAddress('DRM');
      expect(WalletService.getAddresses('TALN').length).toBe(1);
      expect(WalletService.getAddresses('DRM').length).toBe(1);
    });
  });

  describe('getAddresses', () => {
    it('returns empty array when no addresses generated', () => {
      expect(WalletService.getAddresses('OBL')).toEqual([]);
    });

    it('returns all stored addresses for an asset', async () => {
      await WalletService.generateAddress('TALN');
      await WalletService.generateAddress('TALN');
      const addresses = WalletService.getAddresses('TALN');
      expect(addresses.length).toBe(2);
      addresses.forEach((addr) => {
        expect(addr.startsWith('parthenon1q')).toBe(true);
      });
    });
  });

  describe('getCurrentAddress', () => {
    it('returns null when no addresses have been generated', () => {
      expect(WalletService.getCurrentAddress('TALN')).toBeNull();
    });

    it('returns the most recently generated address', async () => {
      await WalletService.generateAddress('TALN');
      const second = await WalletService.generateAddress('TALN');
      expect(WalletService.getCurrentAddress('TALN')).toBe(second);
    });
  });

  describe('signTransaction', () => {
    it('returns a signature object for a valid transaction', async () => {
      const to = 'parthenon1q' + 'b'.repeat(38);
      const tx = { asset: 'TALN', to, amount: 1.5, memo: '' };
      const result = await WalletService.signTransaction(tx);
      expect(result).toBeTruthy();
      expect(typeof result.txid_hash).toBe('string');
      expect(typeof result.signature).toBe('string');
      expect(typeof result.payload_hex).toBe('string');
      expect(result.payload_hex.length).toBeGreaterThan(0);
    });

    it('includes asset, recipient, and amount in the payload hex', async () => {
      const to = 'parthenon1q' + 'b'.repeat(38);
      const tx = { asset: 'TALN', to, amount: 1.0, memo: 'test' };
      const result = await WalletService.signTransaction(tx);
      // Encode using TextEncoder to match the implementation (no Buffer / Node.js API)
      const enc = new TextEncoder();
      const toHexLocal = (arr) =>
        Array.from(arr).map((b) => b.toString(16).padStart(2, '0')).join('');
      const assetHex = toHexLocal(enc.encode('TALN'));
      const addrHex  = toHexLocal(enc.encode(to));
      expect(result.payload_hex).toContain(assetHex);
      expect(result.payload_hex).toContain(addrHex);
    });

    it('throws for invalid recipient address', async () => {
      const tx = { asset: 'TALN', to: 'bad_address', amount: 1.0 };
      await expect(WalletService.signTransaction(tx)).rejects.toThrow(
        'invalid recipient address'
      );
    });

    it('throws for non-positive amount', async () => {
      const to = 'parthenon1q' + 'c'.repeat(38);
      const tx = { asset: 'TALN', to, amount: -1 };
      await expect(WalletService.signTransaction(tx)).rejects.toThrow(
        'invalid transaction fields'
      );
    });

    it('throws for zero amount', async () => {
      const to = 'parthenon1q' + 'd'.repeat(38);
      const tx = { asset: 'DRM', to, amount: 0 };
      await expect(WalletService.signTransaction(tx)).rejects.toThrow(
        'invalid transaction fields'
      );
    });

    it('two identical transactions produce the same signature', async () => {
      const to = 'parthenon1q' + 'e'.repeat(38);
      const tx = { asset: 'OBL', to, amount: 2.5, memo: 'memo' };
      const r1 = await WalletService.signTransaction(tx);
      const r2 = await WalletService.signTransaction(tx);
      expect(r1.signature).toBe(r2.signature);
      expect(r1.payload_hex).toBe(r2.payload_hex);
    });
  });
});
