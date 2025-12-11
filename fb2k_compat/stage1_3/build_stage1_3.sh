#!/bin/bash

# foobar2000兼容层 - 阶段1.3构建脚本
# 构建标准DSP效果器和WASAPI输出设备

echo "=========================================="
echo "foobar2000兼容层 - 阶段1.3构建脚本"
echo "=========================================="
echo "构建时间: $(date)"
echo ""

# 设置构建参数
BUILD_TYPE="Release"
CXX_STANDARD="17"
OUTPUT_DIR="build_stage1_3"
TEST_OUTPUT_DIR="build_test_stage1_3"

# 检测操作系统
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS_TYPE="linux"
    CXX_COMPILER="g++"
    PLATFORM_LIBS="-lpthread -ldl"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS_TYPE="macos"
    CXX_COMPILER="clang++"
    PLATFORM_LIBS="-lpthread"
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    OS_TYPE="windows"
    CXX_COMPILER="g++"
    PLATFORM_LIBS="-lws2_32 -lole32 -loleaut32 -luuid -lavrt"
else
    echo "错误: 不支持的操作系统类型: $OSTYPE"
    exit 1
fi

echo "操作系统: $OS_TYPE"
echo "编译器: $CXX_COMPILER"
echo "构建类型: $BUILD_TYPE"
echo "C++标准: C++$CXX_STANDARD"
echo ""

# 创建输出目录
mkdir -p "$OUTPUT_DIR"
mkdir -p "$TEST_OUTPUT_DIR"

# 基础编译标志
BASE_FLAGS="-std=c++$CXX_STANDARD -Wall -Wextra -O2 -fPIC"
DEBUG_FLAGS="-g -DDEBUG"
RELEASE_FLAGS="-DNDEBUG -O3 -march=native"

# 根据构建类型选择标志
if [ "$BUILD_TYPE" = "Debug" ]; then
    CXX_FLAGS="$BASE_FLAGS $DEBUG_FLAGS"
else
    CXX_FLAGS="$BASE_FLAGS $RELEASE_FLAGS"
fi

# 包含路径
INCLUDE_PATHS="-I. -I../ -I../../"

# 源文件列表
DSP_SOURCES=(
    "dsp_equalizer.cpp"
    "dsp_reverb.cpp"
    "dsp_compressor.cpp"
    "dsp_manager_impl.cpp"
    "audio_block_impl.cpp"
)

OUTPUT_SOURCES=(
    "output_wasapi_impl.cpp"
    "output_device_base.cpp"
)

PROCESSOR_SOURCES=(
    "audio_processor_impl.cpp"
    "ring_buffer.cpp"
    "performance_monitor.cpp"
)

COMMON_SOURCES=(
    "abort_callback.cpp"
    "audio_format_utils.cpp"
    "math_utils.cpp"
)

# 测试源文件
TEST_SOURCES=(
    "test_stage1_3.cpp"
)

echo "=========================================="
echo "编译核心库..."
echo "=========================================="

# 编译DSP模块
echo "编译DSP模块..."
for source in "${DSP_SOURCES[@]}"; do
    if [ -f "$source" ]; then
        echo "  编译: $source"
        $CXX_COMPILER $CXX_FLAGS $INCLUDE_PATHS -c "$source" -o "$OUTPUT_DIR/${source%.cpp}.o" || {
            echo "错误: 编译 $source 失败"
            exit 1
        }
    else
        echo "  警告: 找不到文件 $source"
    fi
done

# 编译输出模块
echo "编译输出模块..."
for source in "${OUTPUT_SOURCES[@]}"; do
    if [ -f "$source" ]; then
        echo "  编译: $source"
        $CXX_COMPILER $CXX_FLAGS $INCLUDE_PATHS -c "$source" -o "$OUTPUT_DIR/${source%.cpp}.o" || {
            echo "错误: 编译 $source 失败"
            exit 1
        }
    else
        echo "  警告: 找不到文件 $source"
    fi
done

# 编译处理器模块
echo "编译处理器模块..."
for source in "${PROCESSOR_SOURCES[@]}"; do
    if [ -f "$source" ]; then
        echo "  编译: $source"
        $CXX_COMPILER $CXX_FLAGS $INCLUDE_PATHS -c "$source" -o "$OUTPUT_DIR/${source%.cpp}.o" || {
            echo "错误: 编译 $source 失败"
            exit 1
        }
    else
        echo "  警告: 找不到文件 $source"
    fi
done

# 编译通用模块
echo "编译通用模块..."
for source in "${COMMON_SOURCES[@]}"; do
    if [ -f "$source" ]; then
        echo "  编译: $source"
        $CXX_COMPILER $CXX_FLAGS $INCLUDE_PATHS -c "$source" -o "$OUTPUT_DIR/${source%.cpp}.o" || {
            echo "错误: 编译 $source 失败"
            exit 1
        }
    else
        echo "  警告: 找不到文件 $source"
    fi
done

# 链接核心库
echo "链接核心库..."
CORE_OBJECTS=""
for source in "${DSP_SOURCES[@]}" "${OUTPUT_SOURCES[@]}" "${PROCESSOR_SOURCES[@]}" "${COMMON_SOURCES[@]}"; do
    object_file="$OUTPUT_DIR/${source%.cpp}.o"
    if [ -f "$object_file" ]; then
        CORE_OBJECTS="$CORE_OBJECTS $object_file"
    fi
done

$CXX_COMPILER $CXX_FLAGS -shared $CORE_OBJECTS $PLATFORM_LIBS -o "$OUTPUT_DIR/libstage1_3_core.so" || {
    echo "错误: 链接核心库失败"
    exit 1
}

# 创建静态库（可选）
ar rcs "$OUTPUT_DIR/libstage1_3_core.a" $CORE_OBJECTS || {
    echo "警告: 创建静态库失败"
}

echo "核心库构建完成: $OUTPUT_DIR/libstage1_3_core.so"
echo ""

echo "=========================================="
echo "编译测试程序..."
echo "=========================================="

# 编译测试程序
for test_source in "${TEST_SOURCES[@]}"; do
    if [ -f "$test_source" ]; then
        test_name="${test_source%.cpp}"
        echo "编译测试: $test_source"
        
        $CXX_COMPILER $CXX_FLAGS $INCLUDE_PATHS "$test_source" -L"$OUTPUT_DIR" -lstaging1_3_core -o "$TEST_OUTPUT_DIR/$test_name" || {
            echo "错误: 编译测试程序 $test_source 失败"
            exit 1
        }
    else
        echo "  警告: 找不到测试文件 $test_source"
    fi
done

echo "测试程序构建完成"
echo ""

# 生成pkg-config文件
cat > "$OUTPUT_DIR/stage1_3.pc" << EOF
prefix=$(pwd)
libdir=\${prefix}/$OUTPUT_DIR
includedir=\${prefix}

Name: foobar2000-compat-stage1-3
Description: foobar2000兼容层 - 阶段1.3 DSP效果器和输出设备
Version: 1.3.0
Libs: -L\${libdir} -lstaging1_3_core $PLATFORM_LIBS
Cflags: -I\${includedir}
EOF

echo "=========================================="
echo "构建完成！"
echo "=========================================="
echo "输出目录: $OUTPUT_DIR"
echo "测试目录: $TEST_OUTPUT_DIR"
echo ""
echo "生成的文件:"
ls -la "$OUTPUT_DIR"
echo ""
echo "测试程序:"
ls -la "$TEST_OUTPUT_DIR"
echo ""
echo "运行测试:"
echo "  $TEST_OUTPUT_DIR/test_stage1_3"
echo ""
echo "环境变量设置:"
echo "  export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:$(pwd)/$OUTPUT_DIR"
echo "  export PKG_CONFIG_PATH=\$PKG_CONFIG_PATH:$(pwd)/$OUTPUT_DIR"
echo ""
echo "构建时间: $(date)"
echo "=========================================="

# 如果构建成功，运行基本测试
if [ -f "$TEST_OUTPUT_DIR/test_stage1_3" ]; then
    echo ""
    echo "运行基本功能测试..."
    echo "=========================================="
    
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(pwd)/$OUTPUT_DIR"
    
    # 运行基本测试（快速模式）
    "$TEST_OUTPUT_DIR/test_stage1_3" --duration 2 --quiet || {
        echo "警告: 基本测试失败，但构建已完成"
    }
fi

exit 0