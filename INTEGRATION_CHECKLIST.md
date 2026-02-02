# 中文字库集成检查清单

## 📋 前置准备

- [ ] 已获取GB2312字库文件（.fon格式）
  - [ ] 来源：GitHub项目或其他可靠来源
  - [ ] 文件大小在预期范围内（参考IMPLEMENTATION_SUMMARY.md）
  - [ ] 文件格式为.fon（二进制）

- [ ] 已准备好SD卡
  - [ ] SD卡插入计算机并识别
  - [ ] SD卡容量足够（至少需要字库大小+100MB）
  - [ ] SD卡格式为FAT32或exFAT

## 🔧 代码集成

### Step 1: 文件验证

- [ ] 检查以下文件是否已创建：
  ```
  include/font.h                      ✓
  include/oled_chinese.h              ✓
  src/font/font.c                     ✓
  src/oled/oled_chinese.c             ✓
  examples/chinese_font_example.c     ✓
  setup_fonts.sh                      ✓
  ```

### Step 2: 项目配置更新

#### 编辑CMakeLists.txt或platformio.ini

- [ ] 在源文件列表中添加：
  ```cmake
  src/font/font.c
  src/oled/oled_chinese.c
  ```

- [ ] 确保包含目录包含：
  ```cmake
  include/  (应该已有)
  ```

- [ ] 验证编译不报错：
  ```bash
  idf.py build
  # 或
  pio run -t build
  ```

### Step 3: 代码修改

#### 在src/main.c中

- [ ] 添加include语句：
  ```c
  #include "font.h"
  #include "oled_chinese.h"
  ```

- [ ] 在初始化代码中添加字库初始化：
  ```c
  font_init("/font/GB2312-16.fon");
  ```

- [ ] 验证函数调用顺序正确：
  1. sdcard_init()
  2. oled_init()
  3. font_init()

- [ ] 在适当位置添加中文显示代码

#### 在其他需要中文的地方

- [ ] 检查菜单显示代码
- [ ] 检查状态显示代码
- [ ] 检查消息显示代码
- [ ] 根据examples/chinese_font_example.c修改相应函数

## 📱 SD卡准备

### 创建目录结构

- [ ] 将SD卡连接到计算机
- [ ] 创建目录：`/font/`（在SD卡根目录）
- [ ] 将字库文件复制到 `/font/` 目录
  - [ ] 验证文件名正确（例如：GB2312-16.fon）
  - [ ] 验证文件完整（文件大小正确）
  - [ ] 验证文件可读取

### 验证文件

- [ ] 在计算机上验证文件：
  ```bash
  ls -lh /path/to/sd/font/
  # 应该看到GB2312-*.fon 文件
  ```

- [ ] 使用setup_fonts.sh脚本（可选）：
  ```bash
  chmod +x setup_fonts.sh
  ./setup_fonts.sh
  ```

## 🔨 编译和部署

### 编译

- [ ] 清理前置构建：
  ```bash
  idf.py fullclean
  ```

- [ ] 重新编译：
  ```bash
  idf.py build
  ```

- [ ] 检查编译输出：
  - [ ] 无错误（Error）
  - [ ] 警告可以忽略（Warning）
  - [ ] 最后显示"Build complete"

### 上传

- [ ] 将设备连接到计算机
- [ ] 确认USB端口识别：
  ```bash
  ls /dev/tty.usbserial*  # macOS
  # 或
  ls /dev/ttyUSB*          # Linux
  ```

- [ ] 上传固件：
  ```bash
  idf.py flash
  ```

- [ ] 重启设备：
  ```bash
  idf.py monitor
  ```

- [ ] 检查启动日志，应该看到：
  ```
  I (xxx) font: Font detected: 16x16 (32 bytes/char)
  I (xxx) app: Chinese font loaded!
  ```

### SD卡挂载

- [ ] 设备启动后，验证SD卡初始化：
  ```
  I (xxx) sdcard: SD card mounted successfully
  ```

## ✅ 功能测试

### 基础测试

- [ ] OLED屏幕能正常显示
  - [ ] ASCII字符可显示
  - [ ] 屏幕无花纹或亮点

- [ ] 中文文字能显示
  - [ ] 单个中文字显示无误
  - [ ] 中文字符串显示正确
  - [ ] 换行处理正确

- [ ] 混合显示正常
  - [ ] ASCII和中文混合显示
  - [ ] 对齐和间距正确

### 菜单测试

- [ ] 中文菜单显示
  - [ ] 菜单项正确显示
  - [ ] 选中高亮正常
  - [ ] 按键导航正常

### 状态显示测试

- [ ] 拍摄进度显示
  - [ ] 进度条正确
  - [ ] 计数器更新
  - [ ] 文字清晰

- [ ] 系统信息显示
  - [ ] 电池状态正确
  - [ ] WiFi状态正确
  - [ ] SD卡状态正确

## 🐛 故障排除

### 如果编译失败

- [ ] 检查source file path
  ```bash
  # 确保这些文件存在：
  test -f src/font/font.c
  test -f src/oled/oled_chinese.c
  ```

- [ ] 检查CMakeLists.txt语法
  ```bash
  idf.py fullclean
  idf.py build
  ```

- [ ] 查看完整错误信息
  ```bash
  idf.py build -v
  ```

### 如果字库文件找不到

- [ ] 检查SD卡状态
  ```
  I (xxx) sdcard: SD card mounted at /sdcard
  ```

- [ ] 检查文件是否存在
  ```bash
  idf.py monitor
  # 查找: "Font file not found"
  ```

- [ ] 重新复制字库文件到SD卡

### 如果显示乱码

- [ ] 验证字库文件完整性
  ```bash
  # 检查文件大小是否匹配规范
  ls -lh /Volumes/SD/font/GB2312-16.fon
  # 应该约216KB
  ```

- [ ] 尝试不同的字库文件
  ```c
  font_init("/font/GB2312-12.fon");  // 尝试12pt
  ```

- [ ] 检查字库文件格式是否正确

### 如果屏幕显示不全

- [ ] 检查坐标范围
  - [ ] X坐标 0-127
  - [ ] Y坐标 0-63

- [ ] 调整字体大小
  ```c
  oled_draw_mixed_string(x, y, text, 12, true);  // 尝试更小的字体
  ```

- [ ] 检查是否超出屏幕边界

## 📚 文档检查

- [ ] 已阅读快速开始指南：`QUICKSTART_CN.md`
- [ ] 已查看完整文档：`CHINESE_FONT_GUIDE.md`
- [ ] 已查看实现总结：`IMPLEMENTATION_SUMMARY.md`
- [ ] 已参考代码示例：`examples/chinese_font_example.c`

## 🎯 最终验证

### 完整流程测试

- [ ] 全新刷写固件
  ```bash
  idf.py erase_flash
  idf.py flash
  ```

- [ ] 插入SD卡，重启设备

- [ ] 观察启动日志：
  ```
  I (xxx) font: Font detected: 16x16
  I (xxx) app: Chinese font loaded!
  ```

- [ ] OLED屏幕显示中文内容

- [ ] 按键导航测试
  - [ ] 菜单可以上下移动
  - [ ] 中文菜单项正确显示

- [ ] 功能测试
  - [ ] 开始拍摄，进度显示中文
  - [ ] 停止拍摄，状态更新正确
  - [ ] 显示系统信息，中文标签正确

## 🚀 部署完成

- [ ] 所有测试通过
- [ ] 没有致命错误（可忽略警告）
- [ ] 中文显示清晰可读
- [ ] 系统运行稳定

**✅ 恭喜！中文字库集成完成！**

---

## 📞 问题排查指南

### 快速参考

| 问题 | 可能原因 | 解决方案 |
|------|--------|--------|
| Font not found | SD卡未初始化/文件不存在 | 检查SD卡，重新复制文件 |
| 乱码显示 | 字库文件损坏/格式错误 | 重新下载或复制字库 |
| 显示不全 | 坐标超出范围 | 检查并调整坐标 |
| 编译失败 | 源文件未添加 | 检查CMakeLists.txt |
| 内存不足 | 缓冲区太大 | 减小缓冲区大小 |

### 获取帮助

1. 查看对应的文档
2. 查看日志输出（`idf.py monitor`）
3. 参考代码示例
4. 检查是否有常见错误

---

**完成日期**：_______________

**验证者**：____________________

**备注**：

_______________________________________________________________

_______________________________________________________________

_______________________________________________________________

