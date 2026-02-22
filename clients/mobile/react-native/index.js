// ParthenonChain Mobile Wallet - App Entry Point
// Registers the root component for both Expo Go and standalone/native builds.

import { registerRootComponent } from 'expo';

import App from './src/App';

// registerRootComponent calls AppRegistry.registerComponent('main', () => App);
// It also ensures that whether you load the app in Expo Go or in a native build,
// the environment is set up appropriately.
registerRootComponent(App);
