// ParthenonChain Mobile Wallet - Staking Screen
// Stake / unstake tokens on L2 (DRACHMA) and L3 (OBOLOS) layers

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  TextInput,
  ScrollView,
  Alert,
  ActivityIndicator,
} from 'react-native';
import NetworkService from '../NetworkService';
import { formatAmount } from '../utils/format';

const LAYERS = [
  { id: 'l2', label: 'L2 – DRACHMA', token: 'DRM', description: 'Proof-of-Stake payment layer' },
  { id: 'l3', label: 'L3 – OBOLOS', token: 'OBL', description: 'Proof-of-Stake EVM execution layer' },
];

// ------------------------------------------------------------------ //
//  Staking info card                                                   //
// ------------------------------------------------------------------ //
const StakingInfoCard = ({ layer, power, loading }) => (
  <View style={styles.infoCard}>
    <Text style={styles.infoCardLayer}>{layer.label}</Text>
    <Text style={styles.infoCardDesc}>{layer.description}</Text>
    {loading ? (
      <ActivityIndicator size="small" color="#007AFF" style={{ marginTop: 8 }} />
    ) : (
      <Text style={styles.infoCardPower}>
        Staking power: {formatAmount(power || 0, layer.token)} {layer.token}
      </Text>
    )}
  </View>
);

// ------------------------------------------------------------------ //
//  Main staking screen                                                 //
// ------------------------------------------------------------------ //
const StakingScreen = ({ onBack }) => {
  const [selectedLayer, setSelectedLayer] = useState('l2');
  const [amount, setAmount] = useState('');
  const [stakingPowers, setStakingPowers] = useState({ l2: 0, l3: 0 });
  const [loadingPower, setLoadingPower] = useState(true);
  const [submitting, setSubmitting] = useState(false);

  // Demo address – in production this comes from WalletService
  const demoAddress = 'demo_address';

  const loadStakingPower = useCallback(async () => {
    setLoadingPower(true);
    try {
      const [l2, l3] = await Promise.all([
        NetworkService.getStakingPower(demoAddress),
        NetworkService.getStakingPower(demoAddress),
      ]);
      setStakingPowers({
        l2: l2?.voting_power || 0,
        l3: l3?.voting_power || 0,
      });
    } catch (_err) {
      // ignore; staking power stays at 0
    } finally {
      setLoadingPower(false);
    }
  }, []);

  useEffect(() => {
    loadStakingPower();
  }, [loadStakingPower]);

  const handleStake = async () => {
    const amountNum = parseFloat(amount);
    if (isNaN(amountNum) || amountNum <= 0) {
      Alert.alert('Error', 'Enter a valid amount to stake');
      return;
    }

    const layer = LAYERS.find((l) => l.id === selectedLayer);
    Alert.alert(
      'Confirm Stake',
      `Stake ${formatAmount(amountNum, layer.token)} ${layer.token} on ${layer.label}?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Stake',
          onPress: async () => {
            try {
              setSubmitting(true);
              await NetworkService.stake(demoAddress, amountNum, selectedLayer);
              Alert.alert('Staked', `${formatAmount(amountNum, layer.token)} ${layer.token} staked on ${layer.label}.`);
              setAmount('');
              await loadStakingPower();
            } catch (err) {
              Alert.alert('Error', err.message || 'Stake failed');
            } finally {
              setSubmitting(false);
            }
          },
        },
      ]
    );
  };

  const handleUnstake = async () => {
    const amountNum = parseFloat(amount);
    if (isNaN(amountNum) || amountNum <= 0) {
      Alert.alert('Error', 'Enter a valid amount to unstake');
      return;
    }

    const layer = LAYERS.find((l) => l.id === selectedLayer);
    Alert.alert(
      'Confirm Unstake',
      `Unstake ${formatAmount(amountNum, layer.token)} ${layer.token} from ${layer.label}?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Unstake',
          onPress: async () => {
            try {
              setSubmitting(true);
              await NetworkService.unstake(demoAddress, amountNum, selectedLayer);
              Alert.alert('Unstaked', `${formatAmount(amountNum, layer.token)} ${layer.token} unstaked from ${layer.label}.`);
              setAmount('');
              await loadStakingPower();
            } catch (err) {
              Alert.alert('Error', err.message || 'Unstake failed');
            } finally {
              setSubmitting(false);
            }
          },
        },
      ]
    );
  };

  const currentLayer = LAYERS.find((l) => l.id === selectedLayer);

  return (
    <ScrollView style={styles.container}>
      <TouchableOpacity style={styles.backButton} onPress={onBack}>
        <Text style={styles.backButtonText}>← Back</Text>
      </TouchableOpacity>
      <Text style={styles.screenTitle}>Staking</Text>

      {/* Info banner */}
      <View style={styles.infoBanner}>
        <Text style={styles.infoBannerText}>
          Stake tokens to participate in Proof-of-Stake consensus and earn governance voting power.
          L2 uses DRACHMA; L3 uses OBOLOS.
        </Text>
      </View>

      {/* Layer selector */}
      <Text style={styles.sectionLabel}>Select Layer</Text>
      <View style={styles.layerSelector}>
        {LAYERS.map((layer) => (
          <TouchableOpacity
            key={layer.id}
            style={[styles.layerButton, selectedLayer === layer.id && styles.layerButtonActive]}
            onPress={() => setSelectedLayer(layer.id)}>
            <Text
              style={[
                styles.layerButtonText,
                selectedLayer === layer.id && styles.layerButtonTextActive,
              ]}>
              {layer.label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Staking power cards */}
      <Text style={styles.sectionLabel}>Your Staking Power</Text>
      {LAYERS.map((layer) => (
        <StakingInfoCard
          key={layer.id}
          layer={layer}
          power={stakingPowers[layer.id]}
          loading={loadingPower}
        />
      ))}

      {/* Stake / Unstake form */}
      <Text style={styles.sectionLabel}>
        Manage Stake on {currentLayer.label}
      </Text>
      <View style={styles.card}>
        <TextInput
          style={styles.input}
          placeholder={`Amount (${currentLayer.token})`}
          placeholderTextColor="#999"
          value={amount}
          onChangeText={setAmount}
          keyboardType="numeric"
        />

        {submitting ? (
          <ActivityIndicator size="small" color="#007AFF" style={{ marginVertical: 12 }} />
        ) : (
          <View style={styles.actionRow}>
            <TouchableOpacity
              style={[styles.actionButton, { backgroundColor: '#28a745' }]}
              onPress={handleStake}>
              <Text style={styles.actionButtonText}>Stake</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.actionButton, { backgroundColor: '#dc3545' }]}
              onPress={handleUnstake}>
              <Text style={styles.actionButtonText}>Unstake</Text>
            </TouchableOpacity>
          </View>
        )}
      </View>

      {/* Staking notes */}
      <View style={styles.notesCard}>
        <Text style={styles.notesTitle}>Notes</Text>
        <Text style={styles.notesText}>
          • Staked tokens are locked during the unbonding period.
        </Text>
        <Text style={styles.notesText}>
          • Staking increases your governance voting power proportionally.
        </Text>
        <Text style={styles.notesText}>
          • Slashing may apply for validator misbehaviour.
        </Text>
        <Text style={styles.notesText}>
          • Rewards are distributed at the end of each epoch.
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    padding: 16,
    backgroundColor: '#f5f5f5',
  },
  backButton: {
    marginBottom: 12,
  },
  backButtonText: {
    color: '#007AFF',
    fontSize: 16,
  },
  screenTitle: {
    fontSize: 26,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 12,
  },
  infoBanner: {
    backgroundColor: '#e8f4fd',
    borderRadius: 10,
    padding: 12,
    marginBottom: 16,
    borderLeftWidth: 4,
    borderLeftColor: '#007AFF',
  },
  infoBannerText: {
    fontSize: 13,
    color: '#444',
    lineHeight: 18,
  },
  sectionLabel: {
    fontSize: 13,
    fontWeight: '600',
    color: '#666',
    textTransform: 'uppercase',
    letterSpacing: 0.8,
    marginBottom: 8,
    marginTop: 4,
  },
  layerSelector: {
    flexDirection: 'row',
    marginBottom: 16,
  },
  layerButton: {
    flex: 1,
    paddingVertical: 10,
    borderRadius: 8,
    backgroundColor: '#e0e0e0',
    alignItems: 'center',
    marginHorizontal: 4,
  },
  layerButtonActive: {
    backgroundColor: '#007AFF',
  },
  layerButtonText: {
    fontSize: 13,
    fontWeight: '600',
    color: '#555',
  },
  layerButtonTextActive: {
    color: '#fff',
  },
  infoCard: {
    backgroundColor: '#fff',
    borderRadius: 10,
    padding: 14,
    marginBottom: 8,
  },
  infoCardLayer: {
    fontSize: 15,
    fontWeight: '700',
    color: '#222',
  },
  infoCardDesc: {
    fontSize: 12,
    color: '#888',
    marginTop: 2,
    marginBottom: 6,
  },
  infoCardPower: {
    fontSize: 14,
    fontWeight: '600',
    color: '#007AFF',
  },
  card: {
    backgroundColor: '#fff',
    borderRadius: 10,
    padding: 16,
    marginBottom: 12,
  },
  input: {
    backgroundColor: '#f5f5f5',
    borderRadius: 8,
    padding: 12,
    fontSize: 15,
    borderWidth: 1,
    borderColor: '#ddd',
    marginBottom: 12,
  },
  actionRow: {
    flexDirection: 'row',
  },
  actionButton: {
    flex: 1,
    paddingVertical: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginHorizontal: 4,
  },
  actionButtonText: {
    color: '#fff',
    fontWeight: '700',
    fontSize: 15,
  },
  notesCard: {
    backgroundColor: '#fff',
    borderRadius: 10,
    padding: 14,
    marginBottom: 24,
  },
  notesTitle: {
    fontSize: 14,
    fontWeight: '600',
    color: '#666',
    marginBottom: 8,
  },
  notesText: {
    fontSize: 13,
    color: '#555',
    marginBottom: 4,
    lineHeight: 18,
  },
});

export default StakingScreen;
