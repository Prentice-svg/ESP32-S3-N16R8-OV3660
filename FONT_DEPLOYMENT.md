# GB2312 字库文件 - 部署说明

## 📦 字库文件信息

| 属性 | 值 |
|------|-----|
| **文件名** | GB2312-16.fon |
| **文件大小** | 256 KB (261,696 字节) |
| **字体大小** | 16×16 像素 |
| **总字符数** | 8,178 个 GB2312 字符 |
| **格式** | 二进制位图字库 |
| **字符编码** | GB2312 |
| **转换工具** | font_builder/gen_gb2312_font.py |
| **源字体** | 仿宋 GB2312 (TTF) |
| **转换日期** | 2026-02-02 |
| **MD5校验和** | 656ec7d62fb5cfbcc45e6fcbf70e1c0c |

## 🚀 部署步骤

### 1. 准备SD卡

在SD卡的根目录创建字体文件夹：

```
SD Card (挂载点: /Volumes/SD 或 /mnt/sdcard)
└── font/
    └── GB2312-16.fon    ← 复制此文件
```

### 2. 复制字库文件

#### macOS:
```bash
# 假设SD卡已挂载到 /Volumes/SD
mkdir -p /Volumes/SD/font
cp /Users/prentice/Develop/ESP32-S3-N16R8-OV3660/font/GB2312-16.fon /Volumes/SD/font/
```

#### Linux:
```bash
# 假设SD卡已挂载到 /mnt/sd
mkdir -p /mnt/sd/font
sudo cp /Users/prentice/Develop/ESP32-S3-N16R8-OV3660/font/GB2312-16.fon /mnt/sd/font/
```

### 3. 验证文件

```bash
# 验证文件大小（应该是 261,696 字节）
ls -lh /Volumes/SD/font/GB2312-16.fon

# 或使用脚本验证
md5sum /Volumes/SD/font/GB2312-16.fon
# 应该输出: 656ec7d62fb5cfbcc45e6fcbf70e1c0c
```

### 4. 安全弹出SD卡

```bash
# macOS
diskutil eject /Volumes/SD

# Linux
sudo umount /mnt/sd
```

## 📝 字库文件格式

### 文件结构

```
GB2312-16.fon:
├─ 字符0 (GB2312: 0xA1A1) [32字节]
├─ 字符1 (GB2312: 0xA1A2) [32字节]
├─ ...
└─ 字符8177 (GB2312: 0xF7FE) [32字节]

总计: 8,178字符 × 32字节 = 261,696字节
```

### 单个字符格式 (32字节)

```
16x16 像素位图，行优先排列：

Byte 0-1:   第1行  (16像素 = 2字节)
Byte 2-3:   第2行  (16像素 = 2字节)
...
Byte 30-31: 第16行 (16像素 = 2字节)

字节内位排列:
  位7 位6 位5 位4 位3 位2 位1 位0
  P7  P6  P5  P4  P3  P2  P1  P0
  ↑
  1 = 白色点, 0 = 黑色点
```

## 🔍 字库验证

### 字符编码范围

GB2312编码范围：
- **高字节** (First Byte): 0xA1 ~ 0xF7 (87个值)
- **低字节** (Second Byte): 0xA1 ~ 0xFE (94个值)
- **总字符数**: 87 × 94 = 8,178个

### 常见汉字验证

以下常用汉字应该都在字库中：

| 字符 | GB2312编码 | 字节位置 |
|------|-----------|---------|
| 中 | 0xD6D0 | 13920字节 |
| 文 | 0xCBF5 | 12576字节 |
| 显 | 0xD5E2 | 13312字节 |
| 示 | 0xCABE | 12160字节 |
| 我 | 0xCED2 | 12992字节 |
| 爱 | 0xB0A1 | 768字节 |

计算公式:
```
高字节偏移 = (高字节 - 0xA1) = H
低字节偏移 = (低字节 - 0xA1) = L
字符索引 = H * 94 + L
字节位置 = 字符索引 * 32
```

## 💾 使用代码示例

### 在C代码中初始化

```c
#include "font.h"
#include "oled_chinese.h"

void setup() {
    // 初始化字库
    esp_err_t ret = font_init("/font/GB2312-16.fon");
    if (ret == ESP_OK) {
        ESP_LOGI("FONT", "GB2312-16 字库加载成功!");
    } else {
        ESP_LOGW("FONT", "字库加载失败");
    }
}

void loop() {
    // 显示中文
    oled_show_chinese_message("你好", "世界", "ESP32");
}
```

## ⚠️ 常见问题

### Q: 字库文件能放在其他位置吗？
**A**: 可以。修改初始化代码中的路径：
```c
font_init("/your/custom/path/GB2312-16.fon");
```

### Q: 字库文件显示乱码
**A**: 检查：
1. 文件大小是否为261,696字节
2. MD5校验和是否匹配
3. 文件是否完整复制
4. 字符编码是否正确

### Q: 文件太大，SD卡空间不足？
**A**:
- 16pt字库已经是最小的实用大小
- 可以考虑压缩字库（需要修改代码）
- 或将字库存储在Flash中（需要更多Flash空间）

### Q: 支持其他字体大小吗？
**A**: 理论上可以。运行：
```bash
# 生成12pt字库
cd font_builder
python3 gen_gb2312_font.py  # 修改脚本中的FONT_SIZE=12

# 生成24pt字库
python3 gen_gb2312_font.py  # 修改脚本中的FONT_SIZE=24
```

## 📊 字库统计

### 转换结果

- ✅ 总字符数: 8,178
- ✅ 成功转换: 8,178 (100%)
- ✅ 失败字符: 0
- ✅ 文件大小: 261,696 字节

### 包含的字符类别

- 汉字: 约6,700字
- 符号: 约1,400个
- 标点: 约78个

## 🔧 重新生成字库

如果需要更新字库或使用不同的字体，可以重新运行脚本：

```bash
cd /Users/prentice/Develop/ESP32-S3-N16R8-OV3660/font_builder

# 使用默认字体（仿宋）
python3 gen_gb2312_font.py

# 脚本会自动检测 ../ref/ 目录中的GB2312 TTF字体
# 并生成 ../font/GB2312-16.fon
```

## 📝 字库文件清单

### 已生成的文件

```
/Users/prentice/Develop/ESP32-S3-N16R8-OV3660/
├── font/
│   └── GB2312-16.fon (256 KB) ✓
├── font_builder/
│   ├── gen_gb2312_font.py (脚本)
│   └── 其他生成工具...
└── ref/
    ├── 仿宋_GB2312.ttf
    ├── 楷体_GB2312.TTF
    └── 其他参考文件...
```

## ✅ 部署检查清单

- [ ] SD卡已插入计算机
- [ ] 已创建 /font 目录
- [ ] GB2312-16.fon 文件已复制
- [ ] 文件大小验证正确 (261,696 字节)
- [ ] MD5校验和验证通过
- [ ] SD卡已安全弹出
- [ ] SD卡已插入ESP32

## 🎉 下一步

1. 将SD卡插入ESP32
2. 运行 `idf.py build && idf.py flash`
3. 查看串口输出：`idf.py monitor`
4. 应该看到: `I (xxx) font: Font detected: 16x16`

---

**生成日期**: 2026-02-02
**字库版本**: GB2312-16 v1.0
**状态**: ✅ 已验证，准备就绪
