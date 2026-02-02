#!/bin/bash
#
# GB2312 Font Deployment Helper
# 自动将字库文件部署到SD卡
#

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
FONT_FILE="$SCRIPT_DIR/font/GB2312-16.fon"
EXPECTED_SIZE=261696

echo "╔════════════════════════════════════════════════════════════╗"
echo "║          GB2312 字库文件部署助手                           ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# 检查文件是否存在
if [ ! -f "$FONT_FILE" ]; then
    echo "✗ 错误: 找不到字库文件"
    echo "  位置: $FONT_FILE"
    echo ""
    echo "请先运行 font_builder/gen_gb2312_font.py 生成字库文件"
    exit 1
fi

# 验证文件大小
FILE_SIZE=$(stat -f%z "$FONT_FILE" 2>/dev/null || stat -c%s "$FONT_FILE" 2>/dev/null)
if [ "$FILE_SIZE" != "$EXPECTED_SIZE" ]; then
    echo "✗ 错误: 字库文件大小不正确"
    echo "  预期: $EXPECTED_SIZE 字节"
    echo "  实际: $FILE_SIZE 字节"
    exit 1
fi

echo "✓ 字库文件验证通过"
echo "  文件: $(basename $FONT_FILE)"
echo "  大小: $(numfmt --to=iec $FILE_SIZE 2>/dev/null || echo "${FILE_SIZE} bytes")"
echo ""

# 检测SD卡挂载点
echo "检测SD卡..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    SD_MOUNT=$(ls -d /Volumes/SD* 2>/dev/null | head -1)
    if [ -z "$SD_MOUNT" ]; then
        echo "✗ 未找到SD卡"
        echo "  请将SD卡插入计算机并挂载"
        exit 1
    fi
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    SD_MOUNT=$(mount | grep -E "/mnt|/media" | awk '{print $3}' | head -1)
    if [ -z "$SD_MOUNT" ]; then
        echo "✗ 未找到SD卡"
        echo "  请将SD卡插入计算机并挂载"
        exit 1
    fi
else
    echo "✗ 不支持的操作系统: $OSTYPE"
    exit 1
fi

echo "✓ 找到SD卡: $SD_MOUNT"
echo ""

# 创建字体目录
FONT_DIR="$SD_MOUNT/font"
echo "创建字体目录..."
mkdir -p "$FONT_DIR"
echo "✓ 目录创建成功: $FONT_DIR"
echo ""

# 复制文件
echo "复制字库文件..."
cp "$FONT_FILE" "$FONT_DIR/GB2312-16.fon"

# 验证复制
if [ ! -f "$FONT_DIR/GB2312-16.fon" ]; then
    echo "✗ 文件复制失败"
    exit 1
fi

# 验证文件大小
COPIED_SIZE=$(stat -f%z "$FONT_DIR/GB2312-16.fon" 2>/dev/null || stat -c%s "$FONT_DIR/GB2312-16.fon" 2>/dev/null)
if [ "$COPIED_SIZE" != "$EXPECTED_SIZE" ]; then
    echo "✗ 复制的文件大小不正确"
    echo "  预期: $EXPECTED_SIZE"
    echo "  实际: $COPIED_SIZE"
    exit 1
fi

echo "✓ 文件复制成功"
echo "  位置: $FONT_DIR/GB2312-16.fon"
echo ""

# 显示下一步步骤
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                    部署完成！                              ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "下一步步骤:"
echo "  1. 从菜单中弹出SD卡（请勿直接拔出）"
echo ""

if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "     diskutil eject $SD_MOUNT"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "     sudo umount $SD_MOUNT"
fi

echo ""
echo "  2. 将SD卡插入ESP32"
echo "  3. 运行编译和部署:"
echo ""
echo "     cd $SCRIPT_DIR"
echo "     idf.py build && idf.py flash"
echo ""
echo "  4. 查看日志输出:"
echo ""
echo "     idf.py monitor"
echo ""
echo "  应该看到类似的输出:"
echo "     I (xxx) font: Font detected: 16x16 (32 bytes/char)"
echo "     I (xxx) app: Chinese font loaded!"
echo ""
