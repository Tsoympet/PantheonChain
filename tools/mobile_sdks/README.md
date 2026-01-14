# PantheonChain Mobile SDKs

Mobile SDKs for iOS and Android to interact with PantheonChain.

## iOS SDK

### Installation

```swift
// Swift Package Manager
dependencies: [
    .package(url: "https://github.com/Tsoympet/PantheonChain-iOS.git", from: "1.0.0")
]
```

### Usage

```swift
import PantheonChain

// Initialize client
let client = PantheonClient(endpoint: "https://node.pantheonchain.io")

// Get balance
client.getBalance(address: "ptn1q...") { result in
    switch result {
    case .success(let balance):
        print("Balance: \(balance)")
    case .failure(let error):
        print("Error: \(error)")
    }
}

// Send transaction
let tx = Transaction(
    from: "ptn1q...",
    to: "ptn1q...",
    amount: 1000,
    privateKey: privateKey
)

client.sendTransaction(tx) { result in
    switch result {
    case .success(let txHash):
        print("Transaction sent: \(txHash)")
    case .failure(let error):
        print("Error: \(error)")
    }
}
```

## Android SDK

### Installation

```gradle
dependencies {
    implementation 'com.pantheonchain:sdk:1.0.0'
}
```

### Usage

```kotlin
import com.pantheonchain.sdk.PantheonClient

// Initialize client
val client = PantheonClient("https://node.pantheonchain.io")

// Get balance
client.getBalance("ptn1q...") { result ->
    result.onSuccess { balance ->
        println("Balance: $balance")
    }.onFailure { error ->
        println("Error: $error")
    }
}

// Send transaction
val tx = Transaction(
    from = "ptn1q...",
    to = "ptn1q...",
    amount = 1000,
    privateKey = privateKey
)

client.sendTransaction(tx) { result ->
    result.onSuccess { txHash ->
        println("Transaction sent: $txHash")
    }.onFailure { error ->
        println("Error: $error")
    }
}
```

## Features

- **Wallet Management**: Create, import, and manage wallets
- **Transaction Signing**: Sign transactions locally
- **Balance Queries**: Query account balances
- **Transaction History**: Get transaction history
- **Smart Contract Interaction**: Call and deploy smart contracts
- **Event Subscriptions**: Subscribe to blockchain events
- **QR Code Support**: Generate and scan payment QR codes

## API Reference

See the [API documentation](https://docs.pantheonchain.io/mobile-sdk) for detailed reference.
