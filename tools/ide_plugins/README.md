# PantheonChain IDE Plugins

IDE integration for PantheonChain smart contract development.

## VSCode Extension

### Installation

1. Open VSCode
2. Go to Extensions (Ctrl+Shift+X)
3. Search for "PantheonChain"
4. Click Install

### Features

- **Syntax Highlighting**: Full support for Solidity and PantheonChain-specific syntax
- **Code Completion**: Intelligent autocomplete for smart contracts
- **Inline Documentation**: Hover over functions to see documentation
- **Error Detection**: Real-time syntax and semantic error checking
- **Deployment**: Deploy contracts directly from the IDE
- **Debugging**: Step-through debugging with breakpoints
- **Gas Estimation**: See estimated gas costs inline

### Usage

```solidity
// Create a new contract
// File > New > PantheonChain Contract

pragma solidity ^0.8.0;

contract MyContract {
    uint256 public value;
    
    // Hover over function to see gas estimate
    function setValue(uint256 _value) public {
        value = _value;
    }
}
```

### Configuration

Create `.vscode/pantheon.json`:

```json
{
    "network": "testnet",
    "endpoint": "https://testnet.pantheonchain.io",
    "compiler": {
        "version": "0.8.19",
        "optimize": true,
        "runs": 200
    }
}
```

## IntelliJ IDEA Plugin

### Installation

1. Open IntelliJ IDEA
2. Go to Settings > Plugins
3. Search for "PantheonChain"
4. Click Install

### Features

- **Project Templates**: Quick-start templates for DApps
- **Smart Refactoring**: Contract-aware refactoring tools
- **Testing Integration**: Run tests from the IDE
- **Version Control**: Git integration for contracts
- **Live Templates**: Code snippets for common patterns

### Usage

```solidity
// Type 'erc20' and press Tab for ERC-20 template
// Type 'ownable' and press Tab for Ownable pattern
// Type 'test' and press Tab for test template
```

## Sublime Text Package

### Installation

```bash
# Using Package Control
1. Ctrl+Shift+P
2. "Package Control: Install Package"
3. Search "PantheonChain"
```

### Features

- **Syntax Highlighting**
- **Build System** (Ctrl+B to compile)
- **Snippets**

## Vim Plugin

### Installation

```vim
" Using vim-plug
Plug 'pantheonchain/vim-pantheon'

" Then run :PlugInstall
```

### Features

- **Syntax Highlighting**
- **Code Folding**
- **Tag Navigation**

## Emacs Mode

### Installation

```elisp
;; Add to .emacs or init.el
(require 'pantheon-mode)
(add-to-list 'auto-mode-alist '("\\.sol\\'" . pantheon-mode))
```

## Language Server Protocol (LSP)

PantheonChain provides an LSP server for IDE-agnostic support.

### Installation

```bash
npm install -g @pantheonchain/language-server
```

### Configuration

Any editor supporting LSP can use the PantheonChain language server.

Example for VSCode:

```json
{
    "pantheonchain.languageServer": {
        "command": "pantheon-language-server",
        "args": ["--stdio"]
    }
}
```

## Development

### Building from Source

```bash
git clone https://github.com/Tsoympet/PantheonChain-IDE-Plugins.git
cd PantheonChain-IDE-Plugins

# VSCode
cd vscode
npm install
npm run compile

# IntelliJ
cd intellij
./gradlew build
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.
