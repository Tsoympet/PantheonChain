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
    it('returns a signed transaction object with signature and timestamp', async () => {
      const tx = { asset: 'TALN', to: 'parthenon1q' + 'b'.repeat(38), amount: 1.5, memo: '' };
      const signed = await WalletService.signTransaction(tx);
      expect(signed.signed).toBe(true);
      expect(signed.signature).toBeTruthy();
      expect(typeof signed.timestamp).toBe('number');
      expect(signed.asset).toBe('TALN');
      expect(signed.amount).toBe(1.5);
    });

    it('preserves original transaction fields', async () => {
      const tx = { asset: 'DRM', to: 'parthenon1q' + 'c'.repeat(38), amount: 0.5, memo: 'test' };
      const signed = await WalletService.signTransaction(tx);
      expect(signed.memo).toBe('test');
      expect(signed.to).toBe(tx.to);
    });
  });
});
