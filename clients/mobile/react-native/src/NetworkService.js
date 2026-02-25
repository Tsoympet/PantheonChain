// ParthenonChain Mobile Wallet - Network Service
// Handles blockchain connectivity and RPC communication

class NetworkService {
  constructor() {
    this.rpcUrl = 'http://127.0.0.1:8332';
    this.connected = false;
    this.blockHeight = 0;
    this.requestId = 1;
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
