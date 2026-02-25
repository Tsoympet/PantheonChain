// ParthenonChain Mobile Wallet - Settings Screen

import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  TextInput,
  Switch,
  ScrollView,
  Alert,
} from 'react-native';
import NetworkService from '../NetworkService';

const SettingsScreen = ({ onBack }) => {
  const [rpcUrl, setRpcUrl] = useState(NetworkService.rpcUrl);
  const [darkMode, setDarkMode] = useState(false);
  const [notifications, setNotifications] = useState(true);

  const handleSaveRpcUrl = () => {
    if (!rpcUrl.startsWith('http://') && !rpcUrl.startsWith('https://')) {
      Alert.alert('Error', 'RPC URL must start with http:// or https://');
      return;
    }
    NetworkService.rpcUrl = rpcUrl;
    Alert.alert('Saved', 'RPC URL updated. Reconnecting...');
    NetworkService.connect(rpcUrl).catch(() => {
      Alert.alert('Warning', 'Could not connect to the new RPC URL.');
    });
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Settings</Text>
      <TouchableOpacity style={styles.backButton} onPress={onBack}>
        <Text style={styles.backButtonText}>← Back</Text>
      </TouchableOpacity>

      {/* Network Section */}
      <Text style={styles.sectionHeader}>Network</Text>
      <View style={styles.card}>
        <Text style={styles.label}>RPC URL</Text>
        <TextInput
          style={styles.input}
          value={rpcUrl}
          onChangeText={setRpcUrl}
          placeholder="http://127.0.0.1:8332"
          placeholderTextColor="#999"
          autoCapitalize="none"
          autoCorrect={false}
        />
        <TouchableOpacity style={styles.saveButton} onPress={handleSaveRpcUrl}>
          <Text style={styles.saveButtonText}>Save</Text>
        </TouchableOpacity>
      </View>

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
  backButton: {
    marginBottom: 20,
  },
  backButtonText: {
    color: '#007AFF',
    fontSize: 16,
  },
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
  label: {
    fontSize: 16,
    color: '#333',
    marginBottom: 8,
  },
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
  saveButton: {
    backgroundColor: '#007AFF',
    padding: 10,
    borderRadius: 8,
    alignItems: 'center',
  },
  saveButtonText: {
    color: '#fff',
    fontWeight: '600',
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  infoText: {
    fontSize: 14,
    color: '#666',
    marginBottom: 4,
  },
});

export default SettingsScreen;
