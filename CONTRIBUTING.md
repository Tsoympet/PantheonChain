# Contributing to ParthenonChain

Thank you for your interest in contributing to ParthenonChain! We welcome contributions from the community and are grateful for your support.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [How to Contribute](#how-to-contribute)
- [Development Workflow](#development-workflow)
- [Coding Standards](#coding-standards)
- [Testing Requirements](#testing-requirements)
- [Commit Message Guidelines](#commit-message-guidelines)
- [Pull Request Process](#pull-request-process)
- [Community](#community)

## Code of Conduct

This project adheres to a Code of Conduct that all contributors are expected to follow. Please read [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) before contributing.

## Getting Started

### Prerequisites

Before you begin, ensure you have the following installed:

- **C++17 compiler** (GCC 7+, Clang 5+, or MSVC 2019+)
- **CMake 3.15+**
- **Git**
- **Dependencies**: OpenSSL, Boost, LevelDB, libevent

See [docs/INSTALLATION.md](docs/INSTALLATION.md) for detailed setup instructions.

### Fork and Clone

1. Fork the repository on GitHub
2. Clone your fork locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/PantheonChain.git
   cd PantheonChain
   ```
3. Add upstream remote:
   ```bash
   git remote add upstream https://github.com/Tsoympet/PantheonChain.git
   ```
4. Initialize submodules:
   ```bash
   git submodule update --init --recursive
   ```

## How to Contribute

### Reporting Bugs

If you find a bug, please create an issue with:

- Clear, descriptive title
- Detailed description of the problem
- Steps to reproduce
- Expected vs actual behavior
- Environment details (OS, compiler version, etc.)
- Relevant logs or error messages

**Security vulnerabilities should be reported privately** - see [SECURITY.md](SECURITY.md).

### Suggesting Enhancements

Enhancement suggestions are welcome! Please:

- Check if the enhancement has already been suggested
- Provide a clear use case
- Explain why it would be useful to the community
- Consider implementation complexity

### Areas for Contribution

We welcome contributions in these areas:

- 🐛 **Bug Fixes**: Fix reported issues
- ✨ **Features**: Implement features from the roadmap
- 📚 **Documentation**: Improve docs, add examples, fix typos
- 🧪 **Testing**: Add test coverage, create test scenarios
- 🎨 **UI/UX**: Improve wallet interfaces
- 🌍 **Translations**: Translate documentation
- ⚡ **Performance**: Optimize code performance
- 🔒 **Security**: Security audits and improvements

## Development Workflow

### 1. Create a Feature Branch

```bash
# Update your fork
git checkout main
git pull upstream main

# Create a feature branch
git checkout -b feature/your-feature-name
```

Branch naming conventions:
- `feature/feature-name` - New features
- `fix/bug-description` - Bug fixes
- `docs/what-changed` - Documentation updates
- `refactor/component-name` - Code refactoring
- `test/test-description` - Test additions

### 2. Make Your Changes

- Write clean, readable code
- Follow existing code style
- Add comments for complex logic
- Update documentation as needed

### 3. Build and Test

```bash
# Build the project
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)

# Run tests
ctest --output-on-failure

# Run specific tests
./layer1/core/crypto/test_crypto
./layer1/core/consensus/test_consensus
```

### 4. Commit Your Changes

```bash
git add .
git commit -m "Brief description of changes"
```

See [Commit Message Guidelines](#commit-message-guidelines) below.

### 5. Push and Create Pull Request

```bash
git push origin feature/your-feature-name
```

Then open a Pull Request on GitHub.

## Coding Standards

### C++ Style Guide

ParthenonChain follows modern C++17 standards:

#### Code Formatting

- Use `.clang-format` configuration (run `clang-format` before committing)
- 4 spaces for indentation (no tabs)
- 100 character line limit
- Use `snake_case` for functions and variables
- Use `PascalCase` for classes and structs
- Use `UPPER_CASE` for constants and macros

#### Code Quality

- Enable all compiler warnings (`-Wall -Wextra -Wpedantic -Werror`)
- Avoid raw pointers; prefer smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Use `const` wherever possible
- Avoid C-style casts; use C++ casts (`static_cast`, etc.)
- Use RAII for resource management
- Avoid global variables

#### Example

```cpp
// Good
class BlockValidator {
public:
    bool ValidateBlock(const Block& block) const;
    
private:
    std::unique_ptr<Consensus> consensus_;
    uint64_t difficulty_target_;
};

// Bad
class blockvalidator {
public:
    int validate(Block* b);  // Non-const, raw pointer
    
    uint64_t difficultyTarget;  // Public member
};
```

### Documentation Standards

- Use Doxygen-style comments for public APIs
- Document parameters, return values, and exceptions
- Provide usage examples for complex functions

```cpp
/**
 * @brief Validates a block according to consensus rules
 * 
 * @param block The block to validate
 * @param chain_state Current blockchain state
 * @return true if block is valid, false otherwise
 * @throws ConsensusException if validation fails critically
 */
bool ValidateBlock(const Block& block, const ChainState& chain_state);
```

## Testing Requirements

### Test Coverage

All new code must include tests:

- **Unit Tests**: Test individual functions/classes
- **Integration Tests**: Test component interactions
- **Consensus Tests**: Verify consensus-critical code

### Writing Tests

Tests use a custom testing framework. Example:

```cpp
TEST(CryptoTests, SchnorrSignatureVerification) {
    // Arrange
    auto keypair = GenerateSchnorrKeypair();
    std::vector<uint8_t> message = {0x01, 0x02, 0x03};
    
    // Act
    auto signature = SchnorrSign(message, keypair.private_key);
    bool valid = SchnorrVerify(message, signature, keypair.public_key);
    
    // Assert
    EXPECT_TRUE(valid);
}
```

### Running Tests

```bash
# All tests
ctest --output-on-failure

# Specific test suite
./layer1/core/crypto/test_crypto

# With verbose output
ctest -V
```

## Commit Message Guidelines

### Format

```
<type>: <subject>

<body>

<footer>
```

### Types

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, no logic change)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Build process, tooling, dependencies

### Examples

```
feat: Add Schnorr signature aggregation support

Implement BIP-340 signature aggregation for multi-signature
transactions. This enables more efficient multi-sig operations
and improves privacy.

Closes #123
```

```
fix: Prevent double-spend in edge case scenario

Fix race condition in mempool that could allow double-spend
when two conflicting transactions arrive simultaneously.

Fixes #456
```

### Rules

- Use imperative mood ("Add feature" not "Added feature")
- First line ≤ 50 characters
- Body wraps at 72 characters
- Reference issues and PRs in footer

## Pull Request Process

### Before Submitting

Ensure your PR:

- ✅ Builds successfully on all platforms
- ✅ Passes all existing tests
- ✅ Includes new tests for new functionality
- ✅ Updates documentation as needed
- ✅ Follows code style guidelines
- ✅ Has clear, descriptive commits
- ✅ Is based on the latest `main` branch

### PR Description

Your PR should include:

- **Summary**: What does this PR do?
- **Motivation**: Why is this change needed?
- **Changes**: List of specific changes made
- **Testing**: How was this tested?
- **Screenshots**: If UI changes, include before/after
- **Related Issues**: Link to related issues

### Review Process

1. Automated checks run (build, tests, linting)
2. Code review by maintainers
3. Address feedback and requested changes
4. Approval by at least one maintainer
5. Merge to `main` branch

### PR Checklist

- [ ] Code builds without errors or warnings
- [ ] All tests pass
- [ ] New tests added for new functionality
- [ ] Documentation updated
- [ ] Code follows style guidelines
- [ ] Commit messages follow guidelines
- [ ] PR description is complete
- [ ] No merge conflicts with `main`

## Development Tips

### Debugging

```bash
# Build with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Use GDB
gdb ./build/clients/core-daemon/parthenond
```

### Performance Profiling

```bash
# Build with profiling
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Use perf (Linux)
perf record ./parthenond
perf report
```

### Memory Leak Detection

```bash
# Use Valgrind
valgrind --leak-check=full ./parthenond

# Use AddressSanitizer
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address"
```

## Project Structure

Understanding the codebase:

```
PantheonChain/
├── layer1/              # Consensus-critical Layer 1
│   ├── core/           # Core blockchain (crypto, consensus, UTXO)
│   ├── evm/            # Smart contract execution
│   ├── network/        # P2P networking
│   └── rpc/            # RPC server
├── layer2/             # Optional Layer 2 extensions
│   ├── payment_channels/
│   ├── bridges/
│   └── indexers/
├── clients/            # Client applications
│   ├── core-daemon/    # Full node daemon
│   ├── cli/            # Command-line interface
│   └── desktop/        # Desktop GUI
├── tests/              # Integration tests
├── docs/               # Documentation
└── third_party/        # External dependencies
```

## Community

### Communication Channels

- **GitHub Discussions**: General questions and discussions
- **GitHub Issues**: Bug reports and feature requests
- **Pull Requests**: Code contributions

### Getting Help

- Read the [documentation](docs/)
- Search [existing issues](https://github.com/Tsoympet/PantheonChain/issues)
- Ask in [GitHub Discussions](https://github.com/Tsoympet/PantheonChain/discussions)

## Recognition

Contributors are recognized in:

- [Contributors page](https://github.com/Tsoympet/PantheonChain/graphs/contributors)
- Release notes for significant contributions
- Project acknowledgments

## License

By contributing to ParthenonChain, you agree that your contributions will be licensed under the [MIT License](LICENSE).

---

**Thank you for contributing to ParthenonChain! Together we're building the future of blockchain technology.** 🏛️