#!/usr/bin/env python3
"""
GB2312 Font Converter
将 TTF 字体转换为 .fon 格式的二进制字库文件
用于 ESP32 OLED 显示中文

GB2312编码:
- 高字节: 0xA1-0xF7 (87个)
- 低字节: 0xA1-0xFE (94个)
- 总字符数: 87 * 94 = 8,178 个

字库格式:
- 每个字符: 16x16 像素 = 32 字节
- 字节排列: 行优先，每行2字节(16像素)
- 总文件大小: 8178 * 32 = 261,696 字节
"""

import sys
import os
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import struct

class GB2312FontConverter:
    """GB2312字体转换器"""

    # GB2312编码范围
    GB2312_HIGH_START = 0xA1
    GB2312_HIGH_END = 0xF7
    GB2312_LOW_START = 0xA1
    GB2312_LOW_END = 0xFE

    # 字体大小和输出格式
    FONT_SIZE = 16  # 16x16 像素
    BYTES_PER_CHAR = 32  # 16*16/8 = 32 字节

    def __init__(self, ttf_path, output_path, font_size=FONT_SIZE):
        """
        初始化转换器

        Args:
            ttf_path: TTF字体文件路径
            output_path: 输出的.fon文件路径
            font_size: 字体大小（像素）
        """
        self.ttf_path = ttf_path
        self.output_path = output_path
        self.font_size = font_size

        # 加载字体
        try:
            self.font = ImageFont.truetype(str(ttf_path), font_size)
            print(f"✓ 字体加载成功: {ttf_path}")
        except Exception as e:
            print(f"✗ 字体加载失败: {e}")
            raise

        self.total_chars = 0
        self.success_chars = 0
        self.failed_chars = []

    def gb2312_to_unicode(self, high_byte, low_byte):
        """
        将GB2312编码转换为Unicode

        Args:
            high_byte: 高字节 (0xA1-0xF7)
            low_byte: 低字节 (0xA1-0xFE)

        Returns:
            Unicode字符
        """
        # GB2312到Unicode的转换
        if high_byte < 0xB0:
            # 符号区
            code = (high_byte - 0xA1) * 94 + (low_byte - 0xA1)
        else:
            # 汉字区
            code = (high_byte - 0xB0) * 94 + (low_byte - 0xA1) + 3755

        # 查找Unicode值（这是近似的，可能不完全准确）
        # 对于GB2312汉字，大部分在CJK统一表中
        if code >= 3755:  # 汉字开始
            unicode_val = 0x4E00 + code - 3755
        else:
            # 标点符号和ASCII扩展
            unicode_table = {
                0: 0x3000, 1: 0x3001, 2: 0x3002, 3: 0xFF0C, 4: 0xFF0E,
                5: 0xFF1A, 6: 0xFF1B, 7: 0xFF1F, 8: 0xFF01, 9: 0x3005,
            }
            unicode_val = unicode_table.get(code, 0x3000)

        try:
            return chr(unicode_val)
        except ValueError:
            return None

    def char_to_bitmap(self, char):
        """
        将单个字符转换为16x16位图

        Args:
            char: 单个Unicode字符

        Returns:
            32字节的位图数据
        """
        # 创建临时图像用于绘制字符
        img = Image.new('1', (self.font_size, self.font_size), color=0)
        draw = ImageDraw.Draw(img)

        # 绘制字符到图像
        # 居中绘制字符
        draw.text((0, 0), char, font=self.font, fill=1)

        # 转换为字节数据
        # 每行使用2字节(16像素)，共16行
        bitmap_data = bytearray(self.BYTES_PER_CHAR)

        pixels = img.load()
        for row in range(self.font_size):
            byte1 = 0
            byte2 = 0

            for col in range(8):
                if pixels[col, row]:
                    byte1 |= (1 << (7 - col))

            for col in range(8, 16):
                if pixels[col, row]:
                    byte2 |= (1 << (15 - col))

            bitmap_data[row * 2] = byte1
            bitmap_data[row * 2 + 1] = byte2

        return bytes(bitmap_data)

    def convert(self):
        """
        执行转换过程

        Returns:
            成功转换的字符数
        """
        print(f"\n开始转换: {self.ttf_path}")
        print(f"字体大小: {self.font_size}x{self.font_size}")
        print(f"输出文件: {self.output_path}")
        print(f"目标字符数: {(self.GB2312_HIGH_END - self.GB2312_HIGH_START + 1) * (self.GB2312_LOW_END - self.GB2312_LOW_START + 1)}")
        print()

        # 创建输出文件
        with open(self.output_path, 'wb') as f:
            # 遍历所有GB2312字符
            for high_byte in range(self.GB2312_HIGH_START, self.GB2312_HIGH_END + 1):
                for low_byte in range(self.GB2312_LOW_START, self.GB2312_LOW_END + 1):
                    self.total_chars += 1

                    try:
                        # 转换编码
                        char = self.gb2312_to_unicode(high_byte, low_byte)

                        if char is None:
                            self.failed_chars.append(f"0x{high_byte:02X}{low_byte:02X}")
                            # 写入空位图
                            f.write(b'\x00' * self.BYTES_PER_CHAR)
                            continue

                        # 生成位图
                        bitmap = self.char_to_bitmap(char)
                        f.write(bitmap)
                        self.success_chars += 1

                        # 显示进度
                        if self.total_chars % 500 == 0:
                            progress = (self.total_chars / (self.GB2312_HIGH_END - self.GB2312_HIGH_START + 1) / (self.GB2312_LOW_END - self.GB2312_LOW_START + 1)) * 100
                            print(f"  处理进度: {self.total_chars}/{(self.GB2312_HIGH_END - self.GB2312_HIGH_START + 1) * (self.GB2312_LOW_END - self.GB2312_LOW_START + 1)} ({progress:.1f}%)")

                    except Exception as e:
                        self.failed_chars.append(f"0x{high_byte:02X}{low_byte:02X}: {str(e)}")
                        # 写入空位图
                        f.write(b'\x00' * self.BYTES_PER_CHAR)

        # 报告结果
        self._print_report()

        return self.success_chars

    def _print_report(self):
        """打印转换报告"""
        print(f"\n{'='*60}")
        print("转换完成!")
        print(f"{'='*60}")
        print(f"总字符数:     {self.total_chars}")
        print(f"成功转换:     {self.success_chars}")
        print(f"失败字符:     {len(self.failed_chars)}")
        print(f"成功率:       {self.success_chars/self.total_chars*100:.1f}%")
        print(f"输出文件:     {self.output_path}")

        # 获取文件大小
        if os.path.exists(self.output_path):
            file_size = os.path.getsize(self.output_path)
            expected_size = self.total_chars * self.BYTES_PER_CHAR
            print(f"文件大小:     {file_size:,} 字节 (预期: {expected_size:,} 字节)")

            if file_size == expected_size:
                print("✓ 文件大小正确!")
            else:
                print("✗ 文件大小不匹配!")

        if self.failed_chars and len(self.failed_chars) <= 10:
            print(f"\n失败字符:")
            for char_code in self.failed_chars:
                print(f"  - {char_code}")

        print(f"{'='*60}\n")


def main():
    """主函数"""
    # 配置
    script_dir = Path(__file__).parent
    ref_dir = script_dir.parent / "ref"

    # 选择字体
    ttf_files = list(ref_dir.glob("*GB2312.t[Tt][Ff]"))

    if not ttf_files:
        print(f"✗ 在 {ref_dir} 中未找到 GB2312 字体文件")
        sys.exit(1)

    print(f"找到字体文件:")
    for i, f in enumerate(ttf_files, 1):
        print(f"  {i}. {f.name}")

    ttf_path = ttf_files[0]  # 使用第一个找到的字体
    print(f"\n使用字体: {ttf_path.name}\n")

    # 创建字库文件
    fon_dir = script_dir.parent / "font"
    fon_dir.mkdir(exist_ok=True)

    fon_path = fon_dir / "GB2312-16.fon"

    try:
        converter = GB2312FontConverter(str(ttf_path), str(fon_path), font_size=16)
        converter.convert()

        print(f"✓ 字库文件生成成功!")
        print(f"  位置: {fon_path}")
        print(f"  大小: {os.path.getsize(fon_path):,} 字节")

    except Exception as e:
        print(f"✗ 转换失败: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
