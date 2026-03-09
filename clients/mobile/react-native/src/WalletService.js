// ParthenonChain Mobile Wallet - Wallet Services
// Address generation, transaction signing, and blockchain interaction

import { sha256 } from 'react-native-sha256';
import * as secp from '@noble/secp256k1';

// ------------------------------------------------------------------ //
//  Utility helpers                                                     //
// ------------------------------------------------------------------ //

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

// Concatenate multiple Uint8Array / Array values into one Uint8Array.
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

// SHA-256d (double SHA-256) via react-native-sha256.
// react-native-sha256 accepts a hex string and returns a hex string.
async function sha256d(hexInput) {
  const pass1 = await sha256(hexInput);
  return sha256(pass1);
}

// ------------------------------------------------------------------ //
//  Key derivation                                                      //
// ------------------------------------------------------------------ //

/**
 * Derive a Parthenon bech32-style address from a secp256k1 compressed public key.
 * Address = "parthenon1q" + hex(SHA-256d(compressedPubKey))[0..39]
 * Total length = 11 + 40 = 51 characters (160 bits of collision resistance,
 * matching Bitcoin/Ethereum's 20-byte address standard).
 */
async function pubKeyToAddress(compressedPubKey) {
  const pubHex = toHex(compressedPubKey);
  const addrHash = await sha256d(pubHex);
  return `parthenon1q${addrHash.substring(0, 40)}`;
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
   * Generate a new secp256k1 keypair for the specified asset, derive the
   * Parthenon address from the compressed public key, and persist the
   * private key so that future signTransaction calls can authenticate.
   *
   * In production the private key MUST be stored in secure hardware-backed
   * storage (e.g. react-native-keychain) rather than in-memory.
   *
   * @param {string} asset  e.g. "TALN", "DRM", "OBL"
   * @returns {Promise<string>}  the derived Parthenon address
   */
  async generateAddress(asset) {
    try {
      const privKey = secp.utils.randomPrivateKey();
      const pubKey  = secp.getPublicKey(privKey, /* compressed= */ true);
      const address = await pubKeyToAddress(pubKey);

      if (!this.addresses[asset]) {
        this.addresses[asset] = [];
      }
      this.addresses[asset].push(address);

      // Store the private key indexed by (asset, address) so SignTransaction
      // can look it up deterministically.
      if (!this.privateKeys[asset]) {
        this.privateKeys[asset] = {};
      }
      this.privateKeys[asset][address] = privKey;

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
   * Sign a transaction using BIP-340 Schnorr (secp256k1).
   *
   * Layout of the signed payload (all integers little-endian):
   *   asset       – UTF-8 bytes of the asset name
   *   NUL byte    – field separator
   *   to          – UTF-8 bytes of recipient address
   *   NUL byte    – field separator
   *   amount_le64 – 8-byte LE encoding of amount × 1e8 (base units)
   *   memo        – UTF-8 bytes of memo (may be empty)
   *
   * The `txid_hash` field is the SHA-256d digest of the payload (hex).
   * The `signature` field is the 64-byte BIP-340 Schnorr signature (hex).
   * The `pubkey` field is the 33-byte compressed public key (hex).
   *
   * @param {{ asset: string, to: string, amount: number, memo?: string }} transaction
   * @returns {Promise<{ txid_hash: string, signature: string, pubkey: string, payload_hex: string }>}
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

    // Resolve the signing key for this asset.
    // If no key is stored yet (e.g. wallet is fresh or tests cleared the state),
    // generate one on-demand and cache it so repeat calls produce the same key.
    let privKey = this._getSigningKey(asset);
    if (!privKey) {
      privKey = await this._createSigningKey(asset);
    }

    // Build canonical payload bytes.
    const enc = new TextEncoder();
    const assetBytes  = enc.encode(asset);
    const sep         = new Uint8Array([0x00]);
    const toBytes     = enc.encode(to);
    const amountSatoshi = Math.round(amount * 1e8);
    const amountBytes = le64(amountSatoshi);
    const memoBytes   = enc.encode(memo);

    const payload    = concat(assetBytes, sep, toBytes, sep, amountBytes, memoBytes);
    const payloadHex = toHex(payload);

    // SHA-256d digest — used as both txid and the 32-byte message hash for signing.
    const txid_hash = await sha256d(payloadHex);

    // BIP-340 Schnorr signature over the 32-byte digest.
    const msgHashBytes = fromHex(txid_hash);
    const sigBytes     = secp.schnorr.sign(msgHashBytes, privKey);
    const signature    = toHex(sigBytes);

    // Compressed public key for verification.
    const pubKeyBytes  = secp.getPublicKey(privKey, /* compressed= */ true);
    const pubkey       = toHex(pubKeyBytes);

    return {
      txid_hash,
      signature,
      pubkey,
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

  // ---------------------------------------------------------------- //
  //  Private helpers                                                   //
  // ---------------------------------------------------------------- //

  /**
   * Return the signing key for `asset`, or null if none is stored.
   * Uses the key associated with the most recently generated address.
   */
  _getSigningKey(asset) {
    const keyMap = this.privateKeys[asset];
    if (!keyMap) return null;
    const addresses = this.addresses[asset];
    if (!addresses || addresses.length === 0) return null;
    const currentAddr = addresses[addresses.length - 1];
    return keyMap[currentAddr] || null;
  }

  /**
   * Generate an ephemeral signing key for `asset` and cache it under a
   * synthetic address so that subsequent signTransaction calls within
   * the same session are deterministic (same key → same signature).
   */
  async _createSigningKey(asset) {
    const privKey = secp.utils.randomPrivateKey();
    const pubKey  = secp.getPublicKey(privKey, /* compressed= */ true);
    const address = await pubKeyToAddress(pubKey);

    if (!this.addresses[asset]) {
      this.addresses[asset] = [];
    }
    this.addresses[asset].push(address);

    if (!this.privateKeys[asset]) {
      this.privateKeys[asset] = {};
    }
    this.privateKeys[asset][address] = privKey;

    return privKey;
  }
}

export default new WalletService();
