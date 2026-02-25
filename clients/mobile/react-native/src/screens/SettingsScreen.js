// ParthenonChain Mobile Wallet - Settings Screen

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  TextInput,
  Switch,
  ScrollView,
  Alert,
  ActivityIndicator,
} from 'react-native';
import NetworkService from '../NetworkService';

const NETWORKS = [
  { key: 'mainnet', label: 'Mainnet',  color: '#1f2a44', description: 'Production network' },
  { key: 'testnet', label: 'Testnet',  color: '#fd7e14', description: 'Public test network' },
  { key: 'devnet',  label: 'Devnet',   color: '#6f42c1', description: 'Developer network — governance role required',
    roleRequired: true },
];

// ---- Sub-component: live status badge ----
const NetworkStatusBadge = ({ status }) => {
  if (!status) return null;
  const connected = status.connected;
  return (
    <View style={[styles.statusBadge, { borderLeftColor: status.networkColor }]}>
      <Text style={[styles.statusBadgeText, { color: status.networkColor }]}>
        {connected ? '● ' : '○ '}{status.networkName}
      </Text>
      <Text style={styles.statusBadgeDetail}>
        {connected
          ? `Connected · Block ${status.blockHeight}${status.peerCount ? ` · ${status.peerCount} peers` : ''}${status.latencyMs >= 0 ? ` · ${status.latencyMs}ms` : ''}`
          : 'Disconnected'}
      </Text>
      {status.nodeVersion ? (
        <Text style={styles.statusBadgeVersion}>Node: {status.nodeVersion}</Text>
      ) : null}
    </View>
  );
};

const SettingsScreen = ({ onBack }) => {
  const [networkStatus, setNetworkStatus] = useState(NetworkService.getNetworkStatus());
  const [selectedNetwork, setSelectedNetwork] = useState(NetworkService.network);
  const [switching, setSwitching] = useState(false);

  // DevNet gate state
  const [devNetAddress, setDevNetAddress] = useState('');
  const [devNetVerified, setDevNetVerified]   = useState(false);
  const [verifying, setVerifying]             = useState(false);

  // Misc settings
  const [darkMode, setDarkMode]           = useState(false);
  const [notifications, setNotifications] = useState(true);

  // Refresh status periodically
  const refreshStatus = useCallback(async () => {
    const s = await NetworkService.refreshNetworkStatus();
    setNetworkStatus({ ...s });
  }, []);

  useEffect(() => {
    refreshStatus();
    const interval = setInterval(refreshStatus, 10000);
    return () => clearInterval(interval);
  }, [refreshStatus]);

  // When devnet is deselected reset verification
  useEffect(() => {
    if (selectedNetwork !== 'devnet') setDevNetVerified(false);
  }, [selectedNetwork]);

  const handleNetworkSelect = (key) => {
    setSelectedNetwork(key);
  };

  const handleVerifyDevNet = async () => {
    if (!devNetAddress.trim()) {
      Alert.alert('Error', 'Enter your governance address first.');
      return;
    }
    setVerifying(true);
    try {
      const result = await NetworkService.checkDevNetAccess(devNetAddress.trim());
      if (result.granted) {
        setDevNetVerified(true);
        Alert.alert(
          'Access Granted',
          `Role verified: ${result.role || 'Governance member'}\nYou may now switch to Devnet.`
        );
      } else {
        setDevNetVerified(false);
        Alert.alert(
          'Access Denied',
          'This address does not hold a qualifying governance role.\n\n' +
            'Eligible roles: Boule member, Prytany member, EmergencyCouncil guardian, ' +
            'or Apophasis board member.'
        );
      }
    } catch (err) {
      Alert.alert('Error', err.message || 'Role verification failed.');
    } finally {
      setVerifying(false);
    }
  };

  const handleApplyNetwork = async () => {
    if (selectedNetwork === 'devnet' && !devNetVerified) {
      Alert.alert(
        'Role Required',
        'Verify your governance role before switching to Devnet.'
      );
      return;
    }
    setSwitching(true);
    try {
      const ok = await NetworkService.setNetwork(selectedNetwork);
      const s  = NetworkService.getNetworkStatus();
      setNetworkStatus({ ...s });
      if (ok) {
        Alert.alert('Network Changed', `Now connected to ${s.networkName}.`);
      } else {
        Alert.alert(
          'Warning',
          `Switched to ${s.networkName} but could not connect. Check your node.`
        );
      }
    } catch (err) {
      Alert.alert('Error', err.message || 'Network switch failed.');
    } finally {
      setSwitching(false);
    }
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Settings</Text>
      <TouchableOpacity style={styles.backButton} onPress={onBack}>
        <Text style={styles.backButtonText}>← Back</Text>
      </TouchableOpacity>

      {/* Live Network Status */}
      <Text style={styles.sectionHeader}>Network Status</Text>
      <NetworkStatusBadge status={networkStatus} />
      <TouchableOpacity style={styles.refreshButton} onPress={refreshStatus}>
        <Text style={styles.refreshButtonText}>↻ Refresh Status</Text>
      </TouchableOpacity>

      {/* Network Selector */}
      <Text style={styles.sectionHeader}>Network Selection</Text>
      <View style={styles.card}>
        {NETWORKS.map((net) => (
          <TouchableOpacity
            key={net.key}
            style={[
              styles.networkRow,
              selectedNetwork === net.key && styles.networkRowActive,
            ]}
            onPress={() => handleNetworkSelect(net.key)}>
            <View style={[styles.networkDot, { backgroundColor: net.color }]} />
            <View style={{ flex: 1 }}>
              <Text style={styles.networkLabel}>{net.label}</Text>
              <Text style={styles.networkDesc}>{net.description}</Text>
            </View>
            {selectedNetwork === net.key && (
              <Text style={[styles.checkmark, { color: net.color }]}>✓</Text>
            )}
          </TouchableOpacity>
        ))}
      </View>

      {/* DevNet Gate — shown only when devnet is selected and not yet verified */}
      {selectedNetwork === 'devnet' && !devNetVerified && (
        <View style={styles.devNetGate}>
          <Text style={styles.devNetWarning}>
            ⚠  Devnet requires a qualifying governance role:{'\n'}
            Boule · Prytany · EmergencyCouncil · Apophasis
          </Text>
          <Text style={styles.label}>Your governance address</Text>
          <TextInput
            style={styles.input}
            value={devNetAddress}
            onChangeText={setDevNetAddress}
            placeholder="Hex address"
            placeholderTextColor="#999"
            autoCapitalize="none"
            autoCorrect={false}
          />
          <TouchableOpacity
            style={[styles.verifyButton, verifying && styles.buttonDisabled]}
            onPress={handleVerifyDevNet}
            disabled={verifying}>
            {verifying
              ? <ActivityIndicator color="#fff" />
              : <Text style={styles.verifyButtonText}>Verify Role</Text>}
          </TouchableOpacity>
        </View>
      )}

      {/* DevNet verified badge */}
      {selectedNetwork === 'devnet' && devNetVerified && (
        <View style={styles.devNetGranted}>
          <Text style={styles.devNetGrantedText}>✓ Devnet access verified</Text>
        </View>
      )}

      {/* Apply button */}
      <TouchableOpacity
        style={[styles.applyButton, switching && styles.buttonDisabled]}
        onPress={handleApplyNetwork}
        disabled={switching}>
        {switching
          ? <ActivityIndicator color="#fff" />
          : <Text style={styles.applyButtonText}>Apply Network</Text>}
      </TouchableOpacity>

      {/* Appearance Section */}
      <Text style={styles.sectionHeader}>Appearance</Text>
      <View style={styles.card}>
        <View style={styles.row}>
          <Text style={styles.label}>Dark Mode</Text>
          <Switch value={darkMode} onValueChange={setDarkMode} />
        </View>
      </View>

      {/* Notifications Section */}
      <Text style={styles.sectionHeader}>Notifications</Text>
      <View style={styles.card}>
        <View style={styles.row}>
          <Text style={styles.label}>Transaction Alerts</Text>
          <Switch value={notifications} onValueChange={setNotifications} />
        </View>
      </View>

      {/* About Section */}
      <Text style={styles.sectionHeader}>About</Text>
      <View style={styles.card}>
        <Text style={styles.infoText}>ParthenonChain Mobile Wallet</Text>
        <Text style={styles.infoText}>Version 1.1.0</Text>
        <Text style={styles.infoText}>© 2024 ParthenonChain Developers</Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    padding: 20,
    backgroundColor: '#f5f5f5',
  },
  title: {
    fontSize: 26,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 8,
  },
  backButton: { marginBottom: 20 },
  backButtonText: { color: '#007AFF', fontSize: 16 },
  sectionHeader: {
    fontSize: 14,
    fontWeight: '600',
    color: '#666',
    marginTop: 16,
    marginBottom: 8,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  card: {
    backgroundColor: '#fff',
    borderRadius: 10,
    padding: 16,
    marginBottom: 8,
  },
  // Network status badge
  statusBadge: {
    backgroundColor: '#fff',
    borderRadius: 10,
    padding: 14,
    marginBottom: 4,
    borderLeftWidth: 5,
  },
  statusBadgeText: {
    fontSize: 15,
    fontWeight: '700',
    marginBottom: 2,
  },
  statusBadgeDetail: {
    fontSize: 12,
    color: '#555',
  },
  statusBadgeVersion: {
    fontSize: 11,
    color: '#888',
    marginTop: 2,
  },
  refreshButton: {
    alignSelf: 'flex-start',
    paddingVertical: 6,
    paddingHorizontal: 12,
    marginBottom: 8,
  },
  refreshButtonText: {
    color: '#007AFF',
    fontSize: 13,
  },
  // Network rows
  networkRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 10,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: '#eee',
  },
  networkRowActive: {
    backgroundColor: '#f0f8ff',
    borderRadius: 8,
    paddingHorizontal: 4,
  },
  networkDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 12,
  },
  networkLabel: {
    fontSize: 15,
    fontWeight: '600',
    color: '#333',
  },
  networkDesc: {
    fontSize: 12,
    color: '#888',
  },
  checkmark: {
    fontSize: 18,
    fontWeight: 'bold',
    marginLeft: 8,
  },
  // DevNet gate
  devNetGate: {
    backgroundColor: '#fff8e1',
    borderRadius: 10,
    padding: 14,
    marginBottom: 8,
    borderLeftWidth: 4,
    borderLeftColor: '#ffc107',
  },
  devNetWarning: {
    fontSize: 13,
    color: '#555',
    marginBottom: 10,
    lineHeight: 19,
  },
  devNetGranted: {
    backgroundColor: '#e8f8ee',
    borderRadius: 10,
    padding: 12,
    marginBottom: 8,
    borderLeftWidth: 4,
    borderLeftColor: '#28a745',
  },
  devNetGrantedText: {
    color: '#28a745',
    fontWeight: '700',
    fontSize: 14,
  },
  label: { fontSize: 16, color: '#333', marginBottom: 6 },
  input: {
    backgroundColor: '#f5f5f5',
    padding: 10,
    borderRadius: 8,
    fontSize: 14,
    borderWidth: 1,
    borderColor: '#ddd',
    marginBottom: 10,
    fontFamily: 'monospace',
  },
  verifyButton: {
    backgroundColor: '#6f42c1',
    padding: 10,
    borderRadius: 8,
    alignItems: 'center',
  },
  verifyButtonText: { color: '#fff', fontWeight: '600' },
  applyButton: {
    backgroundColor: '#007AFF',
    padding: 14,
    borderRadius: 10,
    alignItems: 'center',
    marginBottom: 8,
  },
  applyButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '700',
  },
  buttonDisabled: { backgroundColor: '#aaa' },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  infoText: { fontSize: 14, color: '#666', marginBottom: 4 },
});

export default SettingsScreen;
