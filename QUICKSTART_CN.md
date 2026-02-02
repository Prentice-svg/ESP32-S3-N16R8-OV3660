# 中文字库集成 - 快速开始指南

**预计时间**：5-10分钟

## 概述

本指南将帮助您在ESP32-S3项目中快速启用中文显示功能。

## 📋 需要的文件

1. **GB2312字库文件**（.fon格式）
   - 推荐：`GB2312-16.fon`（~216KB）
   - 来源：https://github.com/kaixindelele/ssd1306-MicroPython-ESP32-Chinese

2. **新增代码文件**（已为您创建）
   ```
   include/font.h                    ✓
   include/oled_chinese.h            ✓
   src/font/font.c                   ✓
   src/oled/oled_chinese.c           ✓
   examples/chinese_font_example.c   ✓
   ```

## 🚀 快速集成步骤

### 1. 获取字库文件

**选项A：从GitHub下载**
```bash
# 访问项目并下载字库文件
https://github.com/kaixindelele/ssd1306-MicroPython-ESP32-Chinese
# 下载 GB2312-16.fon
```

**选项B：使用辅助脚本**
```bash
cd /path/to/your/esp32/project
chmod +x setup_fonts.sh
./setup_fonts.sh
# 按照提示操作
```

### 2. 准备SD卡

在SD卡创建以下目录结构：
```
SD Card/
└── font/
    └── GB2312-16.fon    (复制您的字库文件)
```

### 3. 更新项目配置

#### 编辑 `CMakeLists.txt`

查找包含源文件的部分，添加新文件：

```cmake
set(SOURCES
    src/font/font.c              # 添加这一行
    src/oled/oled.c
    src/oled/oled_chinese.c      # 添加这一行
    # ... 其他文件
)
```

#### 或编辑 `platformio.ini`

确保包含新文件（通常自动检测）

### 4. 在代码中初始化

#### 在 `src/main.c` 的初始化代码中添加：

```c
#include "font.h"
#include "oled_chinese.h"

// 在 app_main() 或 main() 函数中
{
    // ... 现有的初始化代码 ...

    // 初始化 OLED
    oled_init(GPIO_NUM_2, GPIO_NUM_3, 0x3C);

    // 初始化中文字库 ✨ 新增
    esp_err_t ret = font_init("/font/GB2312-16.fon");
    if (ret == ESP_OK) {
        ESP_LOGI("APP", "Chinese font loaded!");
    } else {
        ESP_LOGW("APP", "Chinese font not available");
    }

    // ... 继续您的应用 ...
}
```

### 5. 使用中文显示

#### 简单示例：显示中文消息

```c
// 显示中文消息
oled_show_chinese_message(
    "你好",
    "中文显示",
    "成功!"
);
```

#### 混合中英文

```c
// 显示混合内容
oled_clear();
oled_draw_mixed_string(0, 0, "拍摄进度: 50%", 16, true);
oled_draw_mixed_string(0, 20, "Battery: 85%", 16, true);
oled_draw_mixed_string(0, 40, "SD卡: 正常", 16, true);
oled_update();
```

#### 菜单示例

```c
const char *menu[] = {
    "开始延时摄影",
    "停止延时摄影",
    "单张拍摄",
    "实时预览"
};

int selected = 0;

// 显示菜单
oled_clear();
for (int i = 0; i < 4; i++) {
    if (i == selected) {
        oled_fill_rect(0, i*15, 128, 15, true);
        oled_draw_mixed_string(2, i*15, menu[i], 16, false);
    } else {
        oled_draw_mixed_string(2, i*15, menu[i], 16, true);
    }
}
oled_update();
```

## 📖 API 速查表

### 初始化
```c
font_init("/font/GB2312-16.fon")      // 加载字库
bool font_is_chinese_available()      // 检查字库是否可用
font_deinit()                          // 释放资源
```

### 显示函数
```c
// 中文
oled_draw_chinese_string(x, y, str, on)     // 中文字符串
oled_draw_chinese_char(x, y, hi, lo, on)    // 单个字符

// 混合中英文
oled_draw_mixed_string(x, y, str, size, on)

// 快捷方式
oled_show_chinese_message(line1, line2, line3)
```

## ✅ 验证安装

### 1. 编译项目
```bash
idf.py build
# 或
pio run -t build
```

如果有编译错误：
- ✓ 检查文件路径是否正确
- ✓ 确认 CMakeLists.txt 更新正确
- ✓ 查看包含路径设置

### 2. 上传到设备
```bash
idf.py flash
# 或
pio run -t upload
```

### 3. 检查日志
```bash
idf.py monitor
# 或
pio device monitor
```

**预期输出：**
```
I (xxx) font: Font detected: 16x16 (32 bytes/char)
I (xxx) app: Chinese font loaded!
```

### 4. 查看显示

OLED屏幕应该显示中文文本。

## 🔍 故障排除

### ❌ "Font file not found"

**原因**：字库文件不存在
```
W (xxx) font: Font file not found: /font/GB2312-16.fon
```

**解决**：
1. 检查SD卡是否正确安装
2. 验证字库文件是否在 `/font/` 目录
3. 确认文件名正确拼写
4. 尝试重新复制文件

### ❌ 显示乱码或不显示

**原因**：字库文件格式不正确或损坏

**解决**：
1. 验证文件大小（参见文件规格表）
2. 重新下载或复制字库文件
3. 检查SD卡是否损坏

### ❌ 编译失败 - "undefined reference to..."

**原因**：未正确添加源文件

**解决**：
1. 确认 CMakeLists.txt 中包含所有新文件
2. 运行 `idf.py fullclean` 清理缓存
3. 重新构建项目

### ❌ 中文显示位置不对

**原因**：坐标或大小设置不当

**解决**：
1. 检查 x, y 坐标范围 (0-127, 0-63)
2. 调整字体大小以适应空间
3. 参考示例代码中的坐标设置

## 📚 参考资源

| 资源 | 链接 |
|------|------|
| 原始教程 | https://github.com/kaixindelele/ssd1306-MicroPython-ESP32-Chinese |
| GB2312编码表 | https://www.unicode.org/charts/PDF/U4E00.pdf |
| ESP-IDF文档 | https://docs.espressif.com/projects/esp-idf/ |

## 💡 下一步

1. ✅ 现在可以显示中文了！
2. 📖 详细说明见：`CHINESE_FONT_GUIDE.md`
3. 📝 代码示例见：`examples/chinese_font_example.c`
4. 🔧 更多功能可参考 API 文档

## 常见问题

**Q: 可以使用其他字体大小吗？**
A: 可以！下载 GB2312-12.fon, GB2312-24.fon 或 GB2312-32.fon，然后：
```c
font_init("/font/GB2312-24.fon");  // 24pt字体
```

**Q: 如何显示繁体中文？**
A: 当前系统只支持GB2312（简体中文）。如需繁体，需要使用Big5字库。

**Q: 字库文件可以存在其他地方吗？**
A: 可以。只需更改路径：
```c
font_init("/your/custom/path/GB2312-16.fon");
```

**Q: 占用多少内存？**
A: 约为字库文件大小（16pt约216KB）+ 缓冲区（<100KB），总共<500KB。

---

**需要帮助？** 参考完整文档：`CHINESE_FONT_GUIDE.md`

**上次更新**：2026年2月
