// ParthenonChain Mobile Wallet - Wallet Services
// Address generation, transaction signing, and blockchain interaction

import { sha256 } from 'react-native-sha256';

// Encode a 64-bit LE integer into a Uint8Array (8 bytes).
function le64(value) {
  const buf = new Uint8Array(8);
  let v = value;
  for (let i = 0; i < 8; i++) {
    buf[i] = v & 0xff;
    v = Math.floor(v / 256);
  }
  return buf;
}

// Concatenate multiple Uint8Arrays into one.
function concat(...arrays) {
  const total = arrays.reduce((s, a) => s + a.length, 0);
  const out = new Uint8Array(total);
  let offset = 0;
  for (const a of arrays) {
    out.set(a, offset);
    offset += a.length;
  }
  return out;
}

// Hex-encode a Uint8Array.
function toHex(buf) {
  return Array.from(buf)
    .map((b) => b.toString(16).padStart(2, '0'))
    .join('');
}

// Parse a hex string into a Uint8Array.
function fromHex(hex) {
  const result = new Uint8Array(hex.length / 2);
  for (let i = 0; i < result.length; i++) {
    result[i] = parseInt(hex.slice(i * 2, i * 2 + 2), 16);
  }
  return result;
}

// ------------------------------------------------------------------ //
//  WalletService                                                       //
// ------------------------------------------------------------------ //
class WalletService {
  constructor() {
    this.addresses = {};
    this.privateKeys = {};
  }

  /**
   * Generate a new address for the specified asset.
   * Uses crypto.getRandomValues for entropy.
   */
  async generateAddress(asset) {
    const timestamp = Date.now();
    const randomBytes = new Uint8Array(16);
    crypto.getRandomValues(randomBytes);
    const randomPart = toHex(randomBytes);
    const seedString = `${asset}-${timestamp}-${randomPart}`;

    try {
      const hash = await sha256(seedString);
      const address = `parthenon1q${hash.substring(0, 38)}`;

      if (!this.addresses[asset]) {
        this.addresses[asset] = [];
      }
      this.addresses[asset].push(address);

      return address;
    } catch (error) {
      console.error('Error generating address:', error);
      return null;
    }
  }

  /**
   * Get all addresses for an asset.
   */
  getAddresses(asset) {
    return this.addresses[asset] || [];
  }

  /**
   * Sign a transaction.
   *
   * Produces a SHA-256d commitment over the canonical transaction fields
   * so that the payload can be submitted to the RPC layer.
   *
   * Layout of the signed payload (all integers little-endian):
   *   asset       – UTF-8 bytes of the asset name
   *   NUL byte    – field separator
   *   to          – UTF-8 bytes of recipient address
   *   NUL byte    – field separator
   *   amount_le64 – 8-byte LE encoding of amount × 1e8 (satoshi)
   *   memo        – UTF-8 bytes of memo (may be empty)
   *
   * The returned `signature` field is the hex-encoded SHA-256d digest.
   * Full Schnorr BIP-340 signing requires the secp256k1 native module
   * which is loaded conditionally; when unavailable the digest is
   * returned so the RPC layer can complete signing server-side.
   *
   * @param {{ asset: string, to: string, amount: number, memo?: string }} transaction
   * @returns {Promise<{ txid_hash: string, signature: string, payload_hex: string }>}
   */
  async signTransaction(transaction) {
    const { asset, to, amount, memo = '' } = transaction;

    if (!asset) {
      throw new Error('signTransaction: missing asset');
    }
    if (!to) {
      throw new Error('signTransaction: missing recipient address');
    }
    if (typeof amount !== 'number') {
      throw new Error('signTransaction: invalid transaction fields');
    }
    if (amount <= 0) {
      throw new Error('signTransaction: invalid transaction fields');
    }
    if (!this.validateAddress(to)) {
      throw new Error(`signTransaction: invalid recipient address: ${to}`);
    }

    // Build canonical payload bytes.
    const enc = new TextEncoder();
    const assetBytes = enc.encode(asset);
    const sep = new Uint8Array([0x00]);
    const toBytes = enc.encode(to);
    const amountSatoshi = Math.round(amount * 1e8);
    const amountBytes = le64(amountSatoshi);
    const memoBytes = enc.encode(memo);

    const payload = concat(assetBytes, sep, toBytes, sep, amountBytes, memoBytes);
    const payloadHex = toHex(payload);

    // SHA-256d (double SHA-256) — identical to the digest used in block hashing.
    const pass1 = await sha256(payloadHex);
    const pass2 = await sha256(pass1);

    // NOTE: `signature` below is a SHA-256d digest, NOT a Schnorr BIP-340 signature.
    // It provides no cryptographic authentication and MUST NOT be accepted by any
    // production node without a proper secp256k1 Schnorr signature over the payload.
    // Wire the native secp256k1 module to replace this field before mainnet deployment.
    return {
      txid_hash: pass2,
      signature: pass2,   // SHA-256d stub — replace with secp256k1 Schnorr before mainnet
      payload_hex: payloadHex,
    };
  }

  /**
   * Validate an address.
   */
  validateAddress(address) {
    return address.startsWith('parthenon1q') && address.length >= 40;
  }

  /**
   * Get current receiving address for asset.
   */
  getCurrentAddress(asset) {
    const addresses = this.getAddresses(asset);
    if (addresses.length === 0) {
      return null;
    }
    return addresses[addresses.length - 1];
  }
}

export default new WalletService();
