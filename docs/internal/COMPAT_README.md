# foobar2000 Compatibility Layer

This directory contains the implementation of the foobar2000 compatibility layer for the music player. The compatibility layer enables the use of foobar2000 plugins and provides migration tools for users transitioning from foobar2000.

## Directory Structure

- `adapters/` - Plugin adapter implementations
- `foobar_sdk/` - foobar2000 SDK interface stubs
- `migration/` - Data migration tools
- `tests/` - Unit tests for compatibility layer components

## Components

### XpuMusicCompatManager
The central coordinator for all foobar2000 compatibility features. It handles:
- Detection of foobar2000 installations
- Initialization of compatibility features
- Plugin scanning and management
- Feature enablement based on configuration

### Adapter Framework
Provides base classes and interfaces for implementing compatibility adapters:
- `AdapterBase` - Base class for all adapters
- `InputDecoderAdapter` - Adapts foobar2000 input_decoder to native IDecoder
- `DSPChainAdapter` - Adapts foobar2000 DSP chain to native IDSPProcessor
- `UIExtensionAdapter` - Adapts foobar2000 UI extensions to native UI components

### Data Migration
Tools for migrating user data from foobar2000:
- FPL playlist converter (to M3U)
- Configuration migrator
- Library database importer

### Logging Framework
Unified logging system for compatibility layer components with support for:
- Multiple log destinations (console, file)
- Configurable log levels
- Timestamped log entries

## Build Configuration

The compatibility layer is controlled by the `ENABLE_FOOBAR_COMPAT` CMake option:

```bash
# Enable foobar2000 compatibility (default)
cmake -DENABLE_FOOBAR_COMPAT=ON ..

# Disable foobar2000 compatibility
cmake -DENABLE_FOOBAR_COMPAT=OFF ..
```

## Testing

Unit tests are available in the `tests/` directory and can be built and run with:

```bash
# Build with tests enabled
cmake -DBUILD_TESTS=ON -DENABLE_FOOBAR_COMPAT=ON ..
make

# Run tests
./compat/tests/compat_tests
```

## Implementation Status

This is the initial implementation of the foobar2000 compatibility layer. The following components are implemented:

- [x] Directory structure and build system integration
- [x] XpuMusicCompatManager base class
- [x] foobar2000 SDK interface stubs
- [x] Adapter framework base classes
- [x] Data migration manager stubs
- [x] Logging framework
- [x] Unit test infrastructure

The following components are planned for future implementation:

- [ ] Input decoder adapter
- [ ] DSP chain adapter
- [ ] UI extension adapter
- [ ] FPL playlist converter
- [ ] Configuration migrator
- [ ] Library database importer