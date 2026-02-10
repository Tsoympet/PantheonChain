import React, { useState, useEffect } from 'react';
import {
  SafeAreaView,
  ScrollView,
  StatusBar,
  StyleSheet,
  Text,
  View,
  TouchableOpacity,
  TextInput,
  Switch,
  Alert,
} from 'react-native';
import WalletService from './WalletService';
import NetworkService from './NetworkService';

// ParthenonChain Mobile Wallet
const App = () => {
  const [selectedAsset, setSelectedAsset] = useState('TALN');
  const [miningEnabled, setMiningEnabled] = useState(false);
  const [balances, setBalances] = useState({
    TALN: 0,
    DRM: 0,
    OBL: 0,
  });
  const [hashrate, setHashrate] = useState(0);
  const [currentScreen, setCurrentScreen] = useState('wallet');
  const [connected, setConnected] = useState(false);
  const [blockHeight, setBlockHeight] = useState(0);
  const [currentAddress, setCurrentAddress] = useState('');
  const [transactions, setTransactions] = useState([]);

  const assets = ['TALN', 'DRM', 'OBL'];

  // Connect to network on mount
  useEffect(() => {
    connectToNetwork();
    const interval = setInterval(updateData, 10000); // Update every 10 seconds
    return () => clearInterval(interval);
  }, []);

  const connectToNetwork = async () => {
    try {
      const isConnected = await NetworkService.connect();
      setConnected(isConnected);
      if (isConnected) {
        updateData();
      }
    } catch (error) {
      console.error('Connection error:', error);
    }
  };

  const updateData = async () => {
    try {
      const newBalances = await NetworkService.getBalance();
      setBalances(newBalances);
      
      const height = await NetworkService.updateBlockHeight();
      setBlockHeight(height);
      
      const txs = await NetworkService.getTransactions(10);
      setTransactions(txs);
    } catch (error) {
      console.error('Update error:', error);
    }
  };

  const toggleMining = () => {
    setMiningEnabled(!miningEnabled);
    setHashrate(miningEnabled ? 0 : 1.2);
  };

  const WalletScreen = () => (
    <View style={styles.container}>
      <Text style={styles.title}>Parthenon Wallet</Text>
      
      {/* Connection Status */}
      <View style={styles.statusBar}>
        <Text style={connected ? styles.statusConnected : styles.statusDisconnected}>
          {connected ? `● Connected (Block ${blockHeight})` : '● Disconnected'}
        </Text>
      </View>
      
      {/* Asset Selector */}
      <View style={styles.assetSelector}>
        {assets.map(asset => (
          <TouchableOpacity
            key={asset}
            style={[
              styles.assetButton,
              selectedAsset === asset && styles.assetButtonActive
            ]}
            onPress={() => setSelectedAsset(asset)}>
            <Text style={[
              styles.assetButtonText,
              selectedAsset === asset && styles.assetButtonTextActive
            ]}>
              {asset}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Balance Display */}
      <View style={styles.balanceCard}>
        <Text style={styles.balanceLabel}>Balance</Text>
        <Text style={styles.balanceAmount}>
          {balances[selectedAsset].toFixed(8)} {selectedAsset}
        </Text>
      </View>

      {/* Action Buttons */}
      <View style={styles.actionButtons}>
        <TouchableOpacity 
          style={styles.actionButton}
          onPress={() => setCurrentScreen('send')}>
          <Text style={styles.actionButtonText}>Send</Text>
        </TouchableOpacity>
        <TouchableOpacity 
          style={styles.actionButton}
          onPress={() => setCurrentScreen('receive')}>
          <Text style={styles.actionButtonText}>Receive</Text>
        </TouchableOpacity>
      </View>

      {/* Mining Section */}
      <View style={styles.miningCard}>
        <Text style={styles.miningTitle}>Share Mining</Text>
        <View style={styles.miningRow}>
          <Text>Enable Mining</Text>
          <Switch value={miningEnabled} onValueChange={toggleMining} />
        </View>
        {miningEnabled && (
          <Text style={styles.hashrate}>
            Hashrate: {hashrate} MH/s
          </Text>
        )}
      </View>

      {/* Transactions Button */}
      <TouchableOpacity 
        style={styles.transactionsButton}
        onPress={() => setCurrentScreen('transactions')}>
        <Text style={styles.transactionsButtonText}>
          View Transactions
        </Text>
      </TouchableOpacity>
    </View>
  );

  const SendScreen = () => {
    const [recipientAddress, setRecipientAddress] = useState('');
    const [amount, setAmount] = useState('');
    const [memo, setMemo] = useState('');
    const [sending, setSending] = useState(false);

    const handleSend = async () => {
      if (!WalletService.validateAddress(recipientAddress)) {
        Alert.alert('Error', 'Invalid recipient address');
        return;
      }

      const amountNum = parseFloat(amount);
      if (isNaN(amountNum) || amountNum <= 0) {
        Alert.alert('Error', 'Invalid amount');
        return;
      }

      if (amountNum > balances[selectedAsset]) {
        Alert.alert('Error', 'Insufficient balance');
        return;
      }

      Alert.alert(
        'Confirm Transaction',
        `Send ${amountNum} ${selectedAsset} to ${recipientAddress}?`,
        [
          { text: 'Cancel', style: 'cancel' },
          { 
            text: 'Send', 
            onPress: async () => {
              try {
                setSending(true);
                
                // Sign transaction
                await WalletService.signTransaction({
                  asset: selectedAsset,
                  to: recipientAddress,
                  amount: amountNum,
                  memo
                });
                
                // Submit to network
                const txid = await NetworkService.sendTransaction(
                  selectedAsset, recipientAddress, amountNum, memo
                );
                
                Alert.alert('Success', `Transaction sent!\nTXID: ${txid}`);
                setCurrentScreen('wallet');
                updateData();
              } catch (error) {
                Alert.alert('Error', error.message);
              } finally {
                setSending(false);
              }
            }
          }
        ]
      );
    };

    return (
      <View style={styles.container}>
        <Text style={styles.title}>Send {selectedAsset}</Text>
        <TouchableOpacity 
          style={styles.backButton}
          onPress={() => setCurrentScreen('wallet')}>
          <Text style={styles.backButtonText}>← Back</Text>
        </TouchableOpacity>
        
        <TextInput
          style={styles.input}
          placeholder="Recipient Address"
          placeholderTextColor="#999"
          value={recipientAddress}
          onChangeText={setRecipientAddress}
        />
        <TextInput
          style={styles.input}
          placeholder="Amount"
          keyboardType="numeric"
          placeholderTextColor="#999"
          value={amount}
          onChangeText={setAmount}
        />
        <TextInput
          style={styles.input}
          placeholder="Memo (optional)"
          placeholderTextColor="#999"
          value={memo}
          onChangeText={setMemo}
        />
        <TouchableOpacity 
          style={[styles.sendButton, sending && styles.sendButtonDisabled]}
          onPress={handleSend}
          disabled={sending}>
          <Text style={styles.sendButtonText}>
            {sending ? 'Sending...' : 'Send Transaction'}
          </Text>
        </TouchableOpacity>
      </View>
    );
  };

  const ReceiveScreen = () => {
    const generateAddress = async () => {
      try {
        const address = await WalletService.generateAddress(selectedAsset);
        setCurrentAddress(address);
      } catch (error) {
        Alert.alert('Error', 'Failed to generate address');
      }
    };

    useEffect(() => {
      const addr = WalletService.getCurrentAddress(selectedAsset);
      if (addr) {
        setCurrentAddress(addr);
      }
    }, [selectedAsset]);

    return (
      <View style={styles.container}>
        <Text style={styles.title}>Receive {selectedAsset}</Text>
        <TouchableOpacity 
          style={styles.backButton}
          onPress={() => setCurrentScreen('wallet')}>
          <Text style={styles.backButtonText}>← Back</Text>
        </TouchableOpacity>
        
        <View style={styles.addressCard}>
          <Text style={styles.addressLabel}>Your Address</Text>
          <Text style={styles.address}>
            {currentAddress || 'No address generated'}
          </Text>
        </View>

        <TouchableOpacity 
          style={styles.generateButton}
          onPress={generateAddress}>
          <Text style={styles.generateButtonText}>Generate New Address</Text>
        </TouchableOpacity>
      </View>
    );
  };

  const TransactionsScreen = () => (
    <View style={styles.container}>
      <Text style={styles.title}>Transactions</Text>
      <TouchableOpacity 
        style={styles.backButton}
        onPress={() => setCurrentScreen('wallet')}>
        <Text style={styles.backButtonText}>← Back</Text>
      </TouchableOpacity>
      
      {transactions.length === 0 ? (
        <Text style={styles.noTransactions}>No transactions yet</Text>
      ) : (
        <ScrollView style={styles.txList}>
          {transactions.map((tx, index) => (
            <View key={index} style={styles.txItem}>
              <Text style={styles.txAsset}>{tx.asset || selectedAsset}</Text>
              <Text style={styles.txAmount}>
                {tx.amount > 0 ? '+' : ''}{tx.amount.toFixed(8)}
              </Text>
              <Text style={styles.txDate}>
                {new Date(tx.time * 1000).toLocaleDateString()}
              </Text>
            </View>
          ))}
        </ScrollView>
      )}
    </View>
  );

  const renderScreen = () => {
    switch (currentScreen) {
      case 'send':
        return <SendScreen />;
      case 'receive':
        return <ReceiveScreen />;
      case 'transactions':
        return <TransactionsScreen />;
      default:
        return <WalletScreen />;
    }
  };

  return (
    <SafeAreaView style={styles.safeArea}>
      <StatusBar barStyle="dark-content" />
      <ScrollView contentInsetAdjustmentBehavior="automatic">
        {renderScreen()}
      </ScrollView>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  safeArea: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  container: {
    flex: 1,
    padding: 20,
  },
  title: {
    fontSize: 28,
    fontWeight: 'bold',
    marginBottom: 20,
    color: '#333',
  },
  statusBar: {
    marginBottom: 15,
    padding: 10,
    backgroundColor: '#fff',
    borderRadius: 8,
  },
  statusConnected: {
    color: '#28a745',
    fontWeight: '600',
  },
  statusDisconnected: {
    color: '#dc3545',
    fontWeight: '600',
  },
  assetSelector: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 20,
  },
  assetButton: {
    paddingVertical: 10,
    paddingHorizontal: 20,
    borderRadius: 20,
    backgroundColor: '#e0e0e0',
  },
  assetButtonActive: {
    backgroundColor: '#007AFF',
  },
  assetButtonText: {
    color: '#666',
    fontWeight: '600',
  },
  assetButtonTextActive: {
    color: '#fff',
  },
  balanceCard: {
    backgroundColor: '#fff',
    padding: 30,
    borderRadius: 15,
    alignItems: 'center',
    marginBottom: 20,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  balanceLabel: {
    fontSize: 16,
    color: '#666',
    marginBottom: 10,
  },
  balanceAmount: {
    fontSize: 36,
    fontWeight: 'bold',
    color: '#333',
  },
  actionButtons: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 20,
  },
  actionButton: {
    flex: 1,
    backgroundColor: '#007AFF',
    padding: 15,
    borderRadius: 10,
    marginHorizontal: 5,
    alignItems: 'center',
  },
  actionButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '600',
  },
  miningCard: {
    backgroundColor: '#fff',
    padding: 20,
    borderRadius: 15,
    marginBottom: 20,
  },
  miningTitle: {
    fontSize: 18,
    fontWeight: '600',
    marginBottom: 15,
    color: '#333',
  },
  miningRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  hashrate: {
    marginTop: 10,
    color: '#28a745',
    fontWeight: '600',
  },
  transactionsButton: {
    padding: 15,
    borderRadius: 10,
    borderWidth: 2,
    borderColor: '#007AFF',
    alignItems: 'center',
  },
  transactionsButtonText: {
    color: '#007AFF',
    fontSize: 16,
    fontWeight: '600',
  },
  backButton: {
    marginBottom: 20,
  },
  backButtonText: {
    color: '#007AFF',
    fontSize: 16,
  },
  input: {
    backgroundColor: '#fff',
    padding: 15,
    borderRadius: 10,
    marginBottom: 15,
    fontSize: 16,
    borderWidth: 1,
    borderColor: '#ddd',
  },
  sendButton: {
    backgroundColor: '#28a745',
    padding: 15,
    borderRadius: 10,
    alignItems: 'center',
  },
  sendButtonDisabled: {
    backgroundColor: '#6c757d',
  },
  sendButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '600',
  },
  addressCard: {
    backgroundColor: '#fff',
    padding: 20,
    borderRadius: 15,
    marginBottom: 20,
  },
  addressLabel: {
    fontSize: 16,
    color: '#666',
    marginBottom: 10,
  },
  address: {
    fontSize: 14,
    fontFamily: 'monospace',
    color: '#333',
  },
  generateButton: {
    backgroundColor: '#007AFF',
    padding: 15,
    borderRadius: 10,
    alignItems: 'center',
  },
  generateButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '600',
  },
  txList: {
    flex: 1,
  },
  txItem: {
    backgroundColor: '#fff',
    padding: 15,
    borderRadius: 10,
    marginBottom: 10,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  txAsset: {
    fontSize: 14,
    fontWeight: '600',
    color: '#007AFF',
  },
  txAmount: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#333',
  },
  txDate: {
    fontSize: 12,
    color: '#666',
  },
  noTransactions: {
    textAlign: 'center',
    color: '#666',
    fontSize: 16,
    marginTop: 50,
  },
});

export default App;
