// ParthenonChain Mobile Wallet - Network Service
// Handles blockchain connectivity and RPC communication

// Network configurations
const NETWORK_CONFIG = {
  mainnet: {
    name: 'Mainnet',
    rpcUrl: 'http://127.0.0.1:8332',
    color: '#1f2a44',
  },
  testnet: {
    name: 'Testnet',
    rpcUrl: 'http://127.0.0.1:18332',
    color: '#fd7e14',
  },
  devnet: {
    name: 'Devnet',
    rpcUrl: 'http://127.0.0.1:18443',
    color: '#6f42c1',
    roleRequired: true,
  },
};

class NetworkService {
  constructor() {
    this.network = 'mainnet';
    this.networkConfig = NETWORK_CONFIG;
    this.rpcUrl = NETWORK_CONFIG.mainnet.rpcUrl;
    this.connected = false;
    this.blockHeight = 0;
    this.requestId = 1;
    this.peerCount = 0;
    this.latencyMs = -1;
    this.nodeVersion = '';
  }

  /**
   * Connect to RPC server
   */
  async connect(url = null) {
    if (url) {
      this.rpcUrl = url;
    }

    try {
      const response = await this.rpcRequest('getblockcount');
      if (response && response.result !== undefined) {
        this.connected = true;
        this.blockHeight = response.result;
        return true;
      }
    } catch (error) {
      console.error('Connection error:', error);
      this.connected = false;
    }
    return false;
  }

  /**
   * Switch to a different network.
   * @param {'mainnet'|'testnet'|'devnet'} network
   * @returns {Promise<boolean>} true if connected successfully
   */
  async setNetwork(network) {
    if (!NETWORK_CONFIG[network]) {
      throw new Error(`Unknown network: ${network}`);
    }
    this.network = network;
    this.rpcUrl  = NETWORK_CONFIG[network].rpcUrl;
    this.connected = false;
    return this.connect();
  }

  /**
   * Returns the config object for the current network.
   */
  getCurrentNetworkConfig() {
    return NETWORK_CONFIG[this.network] || NETWORK_CONFIG.mainnet;
  }

  /**
   * Verify that an address holds a qualifying governance role for Devnet access.
   * Eligible roles: Boule, Prytany, EmergencyCouncil guardian, Apophasis board.
   * @param {string} address - hex address to check
   * @returns {Promise<{granted: boolean, role: string}>}
   */
  async checkDevNetAccess(address) {
    try {
      const response = await this.rpcRequest('network/check_dev_access', [{ address }]);
      const result = response.result || {};
      return {
        granted: result.granted === true,
        role: result.role || '',
      };
    } catch (error) {
      console.error('DevNet access check error:', error);
      return { granted: false, role: '' };
    }
  }

  /**
   * Returns a combined live network status object.
   * @returns {{ network: string, networkName: string, networkColor: string,
   *             connected: boolean, blockHeight: number,
   *             peerCount: number, latencyMs: number, nodeVersion: string }}
   */
  getNetworkStatus() {
    const cfg = this.getCurrentNetworkConfig();
    return {
      network:      this.network,
      networkName:  cfg.name,
      networkColor: cfg.color,
      connected:    this.connected,
      blockHeight:  this.blockHeight,
      peerCount:    this.peerCount,
      latencyMs:    this.latencyMs,
      nodeVersion:  this.nodeVersion,
    };
  }

  /**
   * Fetch live network status from the node (peers, latency, version).
   * Updates internal fields and returns the status object.
   */
  async refreshNetworkStatus() {
    try {
      const response = await this.rpcRequest('network/status');
      if (response && response.result) {
        const s = response.result;
        this.peerCount   = s.peer_count  ?? this.peerCount;
        this.latencyMs   = s.latency_ms  ?? this.latencyMs;
        this.nodeVersion = s.version     ?? this.nodeVersion;
      }
    } catch (error) {
      console.error('Error refreshing network status:', error);
    }
    return this.getNetworkStatus();
  }

  /**
   * Check connection status
   */
  isConnected() {
    return this.connected;
  }

  /**
   * Get current block height
   */
  getBlockHeight() {
    return this.blockHeight;
  }

  /**
   * Make RPC request
   */
  async rpcRequest(method, params = []) {
    const request = {
      jsonrpc: '2.0',
      id: this.requestId++,
      method,
      params
    };

    try {
      const response = await fetch(this.rpcUrl, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(request),
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data = await response.json();
      
      if (data.error) {
        throw new Error(data.error.message);
      }

      return data;
    } catch (error) {
      console.error('RPC request error:', error);
      this.connected = false;
      throw error;
    }
  }

  /**
   * Get balance for all assets
   */
  async getBalance() {
    try {
      const response = await this.rpcRequest('getbalance');
      return response.result || { TALN: 0, DRM: 0, OBL: 0 };
    } catch (error) {
      console.error('Error getting balance:', error);
      return { TALN: 0, DRM: 0, OBL: 0 };
    }
  }

  /**
   * Send transaction
   */
  async sendTransaction(asset, address, amount, memo = '') {
    try {
      const params = [asset, address, amount];
      if (memo) {
        params.push(memo);
      }
      
      const response = await this.rpcRequest('sendtoaddress', params);
      return response.result; // Returns TXID
    } catch (error) {
      console.error('Error sending transaction:', error);
      throw error;
    }
  }

  /**
   * Get transaction history
   */
  async getTransactions(limit = 100) {
    try {
      const response = await this.rpcRequest('listtransactions', [limit]);
      return response.result || [];
    } catch (error) {
      console.error('Error getting transactions:', error);
      return [];
    }
  }

  /**
   * Update block height
   */
  async updateBlockHeight() {
    try {
      const response = await this.rpcRequest('getblockcount');
      if (response && response.result !== undefined) {
        this.blockHeight = response.result;
        return this.blockHeight;
      }
    } catch (error) {
      console.error('Error updating block height:', error);
    }
    return this.blockHeight;
  }

  /**
   * Get new address from node
   */
  async getNewAddress() {
    try {
      const response = await this.rpcRequest('getnewaddress');
      return response.result;
    } catch (error) {
      console.error('Error getting new address:', error);
      throw error;
    }
  }

  // ------------------------------------------------------------------ //
  //  Staking                                                             //
  // ------------------------------------------------------------------ //

  /**
   * Stake tokens on a given layer.
   * @param {string} address   - staker address (hex)
   * @param {number} amount    - amount to stake
   * @param {string} layer     - 'l2' or 'l3'
   * @returns {Promise<Object>}
   */
  async stake(address, amount, layer = 'l2') {
    try {
      const response = await this.rpcRequest('staking/stake', [{ address, amount, layer }]);
      return response.result;
    } catch (error) {
      console.error('Error staking:', error);
      throw error;
    }
  }

  /**
   * Unstake tokens from a given layer.
   * @param {string} address
   * @param {number} amount
   * @param {string} layer     - 'l2' or 'l3'
   * @returns {Promise<Object>}
   */
  async unstake(address, amount, layer = 'l2') {
    try {
      const response = await this.rpcRequest('staking/unstake', [{ address, amount, layer }]);
      return response.result;
    } catch (error) {
      console.error('Error unstaking:', error);
      throw error;
    }
  }

  /**
   * Get staking power for an address.
   * @param {string} address
   * @returns {Promise<Object>}  { address, voting_power, layer }
   */
  async getStakingPower(address) {
    try {
      const response = await this.rpcRequest('staking/get_power', [{ address }]);
      return response.result || { address, voting_power: 0 };
    } catch (error) {
      console.error('Error getting staking power:', error);
      return { address, voting_power: 0 };
    }
  }
}

export default new NetworkService();