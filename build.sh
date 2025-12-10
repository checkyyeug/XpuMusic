#!/bin/bash

# XpuMusic 构建脚本
# 自动检测依赖并构建项目

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查命令是否存在
check_command() {
    if command -v "$1" > /dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# 检查依赖
check_dependencies() {
    print_info "Checking dependencies..."

    # 检查编译器
    if check_command gcc; then
        COMPILER="gcc"
        print_success "Found GCC: $(gcc --version | head -n1)"
    elif check_command clang; then
        COMPILER="clang"
        print_success "Found Clang: $(clang --version | head -n1)"
    else
        print_error "No C++ compiler found. Please install GCC or Clang."
        exit 1
    fi

    # 检查CMake
    if check_command cmake; then
        CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
        print_success "Found CMake: $CMAKE_VERSION"

        # 检查版本
        if [ "$(printf '%s\n' "3.20" "$CMAKE_VERSION" | sort -V | head -n1)" != "3.20" ]; then
            print_error "CMake version 3.20 or higher is required."
            exit 1
        fi
    else
        print_error "CMake not found. Please install CMake 3.20 or higher."
        exit 1
    fi

    # 检查Make
    if check_command make; then
        print_success "Found Make: $(make --version | head -n1)"
    elif check_command ninja; then
        GENERATOR="-G Ninja"
        print_success "Found Ninja: $(ninja --version)"
    else
        print_error "No build system found. Please install Make or Ninja."
        exit 1
    fi

    # 检查pkg-config
    if check_command pkg-config; then
        PKG_CONFIG="pkg-config"
        print_success "Found pkg-config"
    else
        print_warning "pkg-config not found, may affect library detection"
    fi

    # 检查音频库（Linux）
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        print_info "Checking audio libraries..."

        # ALSA
        if $PKG_CONFIG --exists alsa 2>/dev/null; then
            ALSA_VERSION=$($PKG_CONFIG --modversion alsa)
            print_success "Found ALSA: $ALSA_VERSION"
            AUDIO_BACKENDS+=("ALSA")
        else
            print_warning "ALSA not found"
        fi

        # PulseAudio
        if $PKG_CONFIG --exists libpulse 2>/dev/null; then
            PULSE_VERSION=$($PKG_CONFIG --modversion libpulse)
            print_success "Found PulseAudio: $PULSE_VERSION"
            AUDIO_BACKENDS+=("PulseAudio")
        else
            print_warning "PulseAudio not found"
        fi

        if [ ${#AUDIO_BACKENDS[@]} -eq 0 ]; then
            print_warning "No audio libraries found. Will use stub backend."
        fi
    fi

    # 检查nlohmann/json
    print_info "Checking for nlohmann/json..."
    if pkg-config --exists nlohmann_json 2>/dev/null; then
        JSON_VERSION=$(pkg-config --modversion nlohmann_json)
        print_success "Found nlohmann/json: $JSON_VERSION"
    elif [ -f "/usr/include/nlohmann/json.hpp" ] || [ -f "/usr/local/include/nlohmann/json.hpp" ]; then
        print_success "Found nlohmann/json in system include path"
    else
        print_warning "nlohmann/json not found. Please install it:"
        print_warning "  Ubuntu/Debian: sudo apt install nlohmann-json3-dev"
        print_warning "  Fedora: sudo dnf install nlohmann-json-devel"
        print_warning "  Arch: sudo pacman -S nlohmann-json"
        print_warning "Or download from https://github.com/nlohmann/json"
    fi
}

# 配置构建类型
configure_build_type() {
    case "$BUILD_TYPE" in
        "Debug")
            CMAKE_BUILD_TYPE="-DCMAKE_BUILD_TYPE=Debug"
            ;;
        "Release")
            CMAKE_BUILD_TYPE="-DCMAKE_BUILD_TYPE=Release"
            ;;
        "RelWithDebInfo")
            CMAKE_BUILD_TYPE="-DCMAKE_BUILD_TYPE=RelWithDebInfo"
            ;;
        "MinSizeRel")
            CMAKE_BUILD_TYPE="-DCMAKE_BUILD_TYPE=MinSizeRel"
            ;;
        *)
            CMAKE_BUILD_TYPE="-DCMAKE_BUILD_TYPE=Release"
            ;;
    esac
}

# 运行构建
run_build() {
    print_info "Configuring build..."

    # 创建构建目录
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    # 配置CMake
    cmake .. \
        $GENERATOR \
        $CMAKE_BUILD_TYPE \
        -DCMAKE_CXX_COMPILER=$COMPILER \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_INSTALL_PREFIX=/usr/local

    print_info "Building project..."

    # 获取CPU核心数
    if check_command nproc; then
        CORES=$(nproc)
    elif check_command sysctl; then
        CORES=$(sysctl -n hw.ncpu)
    else
        CORES=4
    fi

    # 构建
    if [ "$GENERATOR" == "-G Ninja" ]; then
        ninja -j $CORES
    else
        make -j$CORES
    fi

    cd ..
}

# 运行测试
run_tests() {
    if [ "$SKIP_TESTS" != "true" ]; then
        print_info "Running tests..."
        cd "$BUILD_DIR"
        if [ -f "test-audio" ]; then
            ./test-audio
        fi
        if [ -f "test-plugin" ]; then
            ./test-plugin
        fi
        cd ..
    fi
}

# 安装
install_project() {
    if [ "$SKIP_INSTALL" != "true" ]; then
        print_info "Installing project..."
        cd "$BUILD_DIR"

        if [ "$EUID" -eq 0 ]; then
            make install
        else
            print_warning "Not running as root. Installing to ~/.local..."
            cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
            make install
        fi

        cd ..
    fi
}

# 显示帮助
show_help() {
    echo "XpuMusic Build Script"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -h, --help          Show this help message"
    echo "  -t, --type TYPE     Build type: Debug, Release, RelWithDebInfo, MinSizeRel"
    echo "  -d, --dir DIR       Build directory (default: build)"
    echo "  -j, --jobs N        Number of parallel jobs (default: auto)"
    echo "  -c, --clean         Clean build directory before building"
    echo "  -s, --skip-tests    Skip running tests"
    echo "  -i, --skip-install  Skip installation"
    echo "  --ninja             Force use Ninja generator"
    echo "  --static            Build static binaries"
    echo ""
    echo "Examples:"
    echo "  $0                  # Default release build"
    echo "  $0 -t Debug         # Debug build"
    echo "  $0 -c               # Clean and build"
    echo "  $0 -j 8             # Build with 8 parallel jobs"
}

# 默认参数
BUILD_TYPE="Release"
BUILD_DIR="build"
SKIP_TESTS="false"
SKIP_INSTALL="false"
CLEAN_BUILD="false"
AUDIO_BACKENDS=()

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -d|--dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -j|--jobs)
            CORES="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN_BUILD="true"
            shift
            ;;
        -s|--skip-tests)
            SKIP_TESTS="true"
            shift
            ;;
        -i|--skip-install)
            SKIP_INSTALL="true"
            shift
            ;;
        --ninja)
            GENERATOR="-G Ninja"
            shift
            ;;
        --static)
            STATIC_BUILD="-DBUILD_SHARED_LIBS=OFF"
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# 主流程
main() {
    print_info "Starting XpuMusic build..."

    # 检查依赖
    check_dependencies

    # 清理构建目录
    if [ "$CLEAN_BUILD" == "true" ]; then
        print_info "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
    fi

    # 配置构建类型
    configure_build_type

    # 运行构建
    run_build

    # 运行测试
    run_tests

    # 安装
    install_project

    print_success "Build completed successfully!"
    print_info "Executables are available in: $BUILD_DIR/bin/"
    print_info "Plugins are available in: $BUILD_DIR/plugins/"

    if [ "$SKIP_INSTALL" != "true" ]; then
        print_info "Installation completed!"
    fi
}

# 运行主函数
main