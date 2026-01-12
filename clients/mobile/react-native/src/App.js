import React, { useState } from 'react';
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
} from 'react-native';

// ParthenonChain Mobile Wallet
const App = () => {
  const [selectedAsset, setSelectedAsset] = useState('TALN');
  const [miningEnabled, setMiningEnabled] = useState(false);
  const [balances, setBalances] = useState({
    TALN: 100.50,
    DRM: 250.75,
    OBL: 500.00,
  });
  const [hashrate, setHashrate] = useState(0);
  const [currentScreen, setCurrentScreen] = useState('wallet');

  const assets = ['TALN', 'DRM', 'OBL'];

  const toggleMining = () => {
    setMiningEnabled(!miningEnabled);
    setHashrate(miningEnabled ? 0 : 1.2);
  };

  const WalletScreen = () => (
    <View style={styles.container}>
      <Text style={styles.title}>Parthenon Wallet</Text>
      
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
          {balances[selectedAsset].toFixed(2)} {selectedAsset}
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

  const SendScreen = () => (
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
      />
      <TextInput
        style={styles.input}
        placeholder="Amount"
        keyboardType="numeric"
        placeholderTextColor="#999"
      />
      <TouchableOpacity style={styles.sendButton}>
        <Text style={styles.sendButtonText}>Send</Text>
      </TouchableOpacity>
    </View>
  );

  const ReceiveScreen = () => (
    <View style={styles.container}>
      <Text style={styles.title}>Receive {selectedAsset}</Text>
      <TouchableOpacity 
        style={styles.backButton}
        onPress={() => setCurrentScreen('wallet')}>
        <Text style={styles.backButtonText}>← Back</Text>
      </TouchableOpacity>
      
      <View style={styles.addressCard}>
        <Text style={styles.addressLabel}>Your Address</Text>
        <Text style={styles.address}>parthenon1qxyz...</Text>
      </View>
    </View>
  );

  const TransactionsScreen = () => (
    <View style={styles.container}>
      <Text style={styles.title}>Transactions</Text>
      <TouchableOpacity 
        style={styles.backButton}
        onPress={() => setCurrentScreen('wallet')}>
        <Text style={styles.backButtonText}>← Back</Text>
      </TouchableOpacity>
      
      <Text style={styles.noTransactions}>No transactions yet</Text>
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
  sendButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '600',
  },
  addressCard: {
    backgroundColor: '#fff',
    padding: 20,
    borderRadius: 15,
    alignItems: 'center',
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
  noTransactions: {
    textAlign: 'center',
    color: '#666',
    fontSize: 16,
    marginTop: 50,
  },
});

export default App;
