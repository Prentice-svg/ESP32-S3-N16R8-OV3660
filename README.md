# ESP32-S3 Timelapse Camera 使用说明

## 项目概述

本项目是基于 **ESP32-S3-N16R8** 开发板的智能延时摄影相机系统，配备了以下主要功能：

- **3MP OV3660 摄像头** - 高分辨率图像采集
- **64GB+ SD卡存储** - 支持超大容量存储，可存储数千张图片
- **WiFi 远程控制** - 支持 AP 模式和 STA 模式
- **实时Web界面** - 通过浏览器实时监控和控制
- **电池管理** - 包含电压监测和低电量警告
- **深度睡眠** - 功耗优化的低功耗模式
- **可选 LCD 显示屏** - SPI接口的2.8"显示屏支持
- **PSRAM 支持** - 16MB RAM加速图像处理

---

## 硬件需求

### 核心硬件

| 组件 | 规格 | 说明 |
|------|------|------|
| 主控板 | ESP32-S3-N16R8 | 16MB PSRAM, 8MB Flash |
| 摄像头 | OV3660 | 3MP, JPEG输出 |
| 存储 | microSD卡 | 支持64GB+, Class 10推荐 |
| 供电 | 锂电池 | 3.7V LiPo 推荐 3000mAh+ |
| 显示屏(可选) | ST7789 | 2.8" 240x320 SPI接口 |

### 摄像头接线

```
摄像头(OV3660) -> ESP32-S3
=====================================
XCLK       -> GPIO8
SIOD(SDA)  -> GPIO40 (I2C)
SIOC(SCL)  -> GPIO41 (I2C)
VSYNC      -> GPIO6
HREF       -> GPIO7
PCLK       -> GPIO13

D0-D7      -> GPIO11,9,4,5,15,16,17,18
PWD        -> 不连接(或GPIO-1)
RST        -> 不连接(或GPIO-1)
```

### SD卡接线(SDMMC 4-bit 模式)

```
SD卡 -> ESP32-S3
=====================================
CLK  -> GPIO12
CMD  -> GPIO11
D0   -> GPIO13
D1   -> GPIO14
D2   -> GPIO8
D3   -> GPIO46
```

### 可选:LCD接线

```
LCD(ST7789) -> ESP32-S3
=====================================
SCK   -> GPIO36
MOSI  -> GPIO35
CS    -> GPIO34
DC    -> GPIO38
RST   -> GPIO37
```

### 电源接线

```
LiPo电池 -> 分压电路 -> ESP32-S3 ADC
=====================================
电池正极(+) -> 分压电阻(2倍分压) -> GPIO1(ADC)
电池负极(-) -> GND

// 分压计算: GPIO读值 = 电池电压 / 2
// 例: 4.2V电池 -> GPIO读取约2.1V
```

---

## 安装和编译

### 环境准备

#### 方式1: 使用 PlatformIO (推荐)

```bash
# 安装 PlatformIO CLI
pip install platformio

# 克隆项目
git clone <repository-url>
cd ESP32-S3-N16R8-OV3660

# 编译项目
pio run

# 烧录固件
pio run -t upload
```

#### 方式2: 使用 ESP-IDF

```bash
# 设置 ESP-IDF 环境
. $IDF_PATH/export.sh

# 构建
idf.py build

# 烧录
idf.py -p /dev/ttyUSB0 flash

# 监控串口输出
idf.py -p /dev/ttyUSB0 monitor
```

### 配置说明

编辑 `platformio.ini` 进行自定义配置：

```ini
[env:esp32s3cam]
platform = espressif32
board = esp32-s3-devkitc-1
framework = espidf
monitor_speed = 115200
upload_speed = 921600

build_flags =
    -DBOARD_HAS_PSRAM
    -DCONFIG_ESP32S3_DEFAULT_CPU_FREQ_240=y
    -DCONFIG_ESP32_CAMERA_OV3660_SUPPORT=y
    -DCONFIG_SCCB_CLK_FREQ=100000
```

---

## 快速开始

### 1. 初始化设备

设备首次启动时：

```bash
# 串口连接 (115200 baud)
# 查看启动日志
I (100) main: ESP32-S3 Timelapse Camera v1.0
I (120) power: Power management initialized
I (150) sdcard: SD Card mounted: SD Card
I (200) camera: Camera initialized (Sensor ID: 0x3660)
```

### 2. 连接 WiFi

#### AP 模式 (推荐首次使用)

设备启动时默认以 AP 模式运行：

1. 在手机/电脑上搜索 WiFi 网络
2. 查找 SSID: `ESP32-Cam` (可在代码中修改)
3. 连接，密码默认为: `12345678`
4. 打开浏览器访问: `http://192.168.4.1`

#### STA 模式 (连接现有WiFi)

通过Web界面配置：

1. 连接到设备AP
2. 访问 `http://192.168.4.1`
3. 在配置页面输入目标WiFi的SSID和密码
4. 点击"连接"
5. 设备重启后自动连接到指定WiFi

### 3. Web 界面使用

访问 `http://<设备IP>` 打开Web界面：

```
┌─────────────────────────────────────┐
│     Timelapse Camera Control        │
├─────────────────────────────────────┤
│ 状态: Running                       │
│ 进度: 234/1000 shots                │
│ 下次拍摄: 45s                       │
│ 电池: 85.3% (4.15V)                │
│ 可用空间: 58.2 GB                  │
├─────────────────────────────────────┤
│ [Start] [Stop] [Capture]           │
├─────────────────────────────────────┤
│ 间隔时间: [60] 秒                  │
│ 总拍摄数: [1000] 张                │
│ [Update Config]                     │
├─────────────────────────────────────┤
│ IP: 192.168.1.100                  │
│ [View Files] - 查看已拍摄的照片     │
└─────────────────────────────────────┘
```

---

## 使用功能

### 基本操作

#### 开始延时摄影

```
1. 打开Web界面
2. 点击 [Start] 按钮
3. 相机开始按设定间隔拍摄
4. 进度条显示当前拍摄进度
```

#### 停止拍摄

```
1. 点击 [Stop] 按钮
2. 相机停止拍摄，保留已拍摄的照片
3. 可查看已拍摄文件列表
```

#### 单次拍摄

```
1. 点击 [Capture] 按钮
2. 即时拍摄一张照片
3. 照片自动保存到SD卡
```

### 参数配置

| 参数 | 范围 | 说明 | 默认值 |
|------|------|------|--------|
| 间隔时间 | 1-3600s | 两次拍摄间隔 | 60s |
| 总拍摄数 | 1-10000 | 延时序列总数 | 1000 |
| JPEG 质量 | 12-63 | 压缩质量(越小越好) | 12 |
| 分辨率 | 见下表 | 图像输出分辨率 | UXGA(2048x1536) |

#### 支持的分辨率

| 分辨率代码 | 尺寸 | 说明 |
|-----------|------|------|
| UXGA | 2048×1536 | 最高分辨率 |
| SXGA | 1280×1024 | 推荐设置 |
| XGA | 1024×768 | 平衡模式 |
| VGA | 640×480 | 快速模式 |
| QVGA | 320×240 | 超快速 |

### 电池管理

#### 电压监测

设备每5秒更新一次电池信息：

```
正常: > 3.6V (绿色)
警告: 3.3-3.6V (黄色)
低电: < 3.3V (红色)
```

#### 低电量处理

当电池电压低于 3.3V 时：

1. Web界面显示红色警告
2. 日志输出警告信息
3. 自动停止当前拍摄
4. 建议连接USB充电或更换电池

### 深度睡眠

#### 进入深度睡眠

**按键方式:**
```
长按 BOOT 按钮 3 秒
→ LCD熄灭(如有)
→ 设备进入深度睡眠
→ 功耗降至 < 10μA
```

**唤醒方式:**
```
再次按下 BOOT 按钮
→ 设备唤醒
→ 系统重启
→ 恢复之前的拍摄状态
```

#### 定时睡眠

通过API调用进入睡眠：

```bash
# 睡眠 3600 秒(1小时)后自动唤醒
curl -X POST http://192.168.1.100/sleep?seconds=3600
```

---

## 文件管理

### 文件存储结构

```
/sdcard/
├── TIMELAPSE_20240129_120000_00001.jpg
├── TIMELAPSE_20240129_120100_00002.jpg
├── TIMELAPSE_20240129_120200_00003.jpg
└── ...
```

**文件名格式:** `PREFIX_YYYYMMDD_HHMMSS_XXXXX.jpg`

- PREFIX: 文件前缀(默认"TIMELAPSE")
- YYYYMMDD: 拍摄日期
- HHMMSS: 拍摄时间
- XXXXX: 序列号(5位)

### 查看文件列表

#### 通过Web界面

```
1. 打开 http://<IP>/
2. 点击 [View Files]
3. 显示所有拍摄的照片列表
4. 点击照片可下载
```

#### 通过API

```bash
# 获取文件列表 (JSON格式)
curl http://192.168.1.100/files

# 响应示例:
{
  "files": [
    "TIMELAPSE_20240129_120000_00001.jpg",
    "TIMELAPSE_20240129_120100_00002.jpg"
  ],
  "count": 2,
  "total_size": "8.5 MB"
}
```

### 下载文件

```bash
# 下载单个文件
curl http://192.168.1.100/download?name=TIMELAPSE_20240129_120000_00001.jpg > photo.jpg

# 或通过Web界面直接点击下载
```

---

## REST API 参考

### 获取状态

```bash
GET /status

# 响应:
{
  "state": 1,                    // 0:空闲 1:运行中 2:暂停 3:完成
  "current_shot": 234,           // 当前拍摄数
  "total_shots": 1000,           // 总计划拍摄数
  "next_shot_sec": 45,           // 下次拍摄倒计时(秒)
  "battery_voltage": 4.15,       // 电池电压(V)
  "battery_percent": 85.3,       // 电池百分比(%)
  "free_bytes": 62449643520,     // SD卡可用空间(字节)
  "wifi_mode": 1                 // WiFi模式 1:STA 2:AP
}
```

### 控制命令

```bash
# 开始拍摄
POST /start

# 停止拍摄
POST /stop

# 单次拍摄
POST /capture

# 配置参数
POST /config
?interval=60           # 拍摄间隔(秒)
&shots=1000           # 总拍摄数
&quality=12           # JPEG质量
&framesize=2          # 分辨率代码
```

### 获取文件列表

```bash
GET /files

# 响应:
{
  "files": ["file1.jpg", "file2.jpg"],
  "count": 2,
  "total_size": 8500000
}
```

### 下载文件

```bash
GET /download?name=filename.jpg
```

### 系统控制

```bash
# 重启设备
POST /reboot

# 进入深度睡眠 (秒数可选)
POST /sleep?seconds=3600

# 格式化SD卡 (谨慎使用!)
POST /format
```

---

## 按键功能

### BOOT 按键 (GPIO0)

| 操作 | 功能 |
|------|------|
| 短按(< 1秒) | 切换拍摄运行/暂停 |
| 长按(3秒) | 进入深度睡眠 |
| 双击(< 500ms) | 触发单次拍摄 |

---

## 常见问题和故障排除

### Q1: 设备无法识别摄像头

**症状:** 启动日志显示 "Camera init failed"

**解决方案:**
```bash
1. 检查摄像头接线是否松动
2. 确认 XCLK, SIOD, SIOC 接线正确
3. 重启设备
4. 如仍不工作，更换摄像头测试
```

### Q2: SD卡无法挂载

**症状:** 启动日志显示 "Failed to mount SD Card"

**解决方案:**
```bash
1. 确认SD卡插入正确并完全
2. 检查SD卡是否需要格式化 (FAT32)
3. 确认接线: CLK, CMD, D0-D3 无误
4. 尝试重新插拔SD卡
5. 如仍不工作，尝试其他SD卡
```

### Q3: WiFi 连接失败

**症状:** 设备无法连接到指定的WiFi

**解决方案:**
```bash
1. 确认 WiFi SSID 和密码正确
2. 检查路由器是否在范围内
3. 重启设备重新尝试连接
4. 如失败多次，重置为 AP 模式:
   - 清除 NVS 闪存: menuconfig -> Partition Table -> OTA disabled
   - 重新编译烧录
```

### Q4: 拍摄过程中频繁停止

**症状:** 运行过程中不定时重启或崩溃

**解决方案:**
```bash
1. 查看串口日志，查找错误信息
2. 增加堆内存:
   - menuconfig -> Component config -> FreeRTOS -> Heap Size
3. 检查电源供应是否稳定
4. 尝试降低拍摄分辨率减少内存占用
5. 检查温度是否过高(CPU < 60°C)
```

### Q5: 文件保存到SD卡速度慢

**症状:** 拍摄间隔很长但SD卡写入缓慢

**解决方案:**
```bash
1. 使用高速 Class 10 SD卡
2. 降低 JPEG 质量 (quality 参数增大)
3. 降低拍摄分辨率
4. 确认 SPI 频率配置:
   - build_flags 中 CONFIG_SCCB_CLK_FREQ=100000
```

### Q6: Web 界面无法访问

**症状:** 浏览器无法连接到设备IP

**解决方案:**
```bash
1. 确认设备已连接到WiFi (检查LED指示灯)
2. 通过 ARP 或路由器查找设备IP:
   arp -a | grep esp
3. 确认防火墙未阻止访问
4. 尝试重启设备和路由器
5. 通过串口查看IP地址信息
```

### Q7: 电池显示异常

**症状:** 电池电压显示不正确或为0

**解决方案:**
```bash
1. 检查 GPIO1 (ADC) 是否连接分压电路
2. 验证分压比例: 电阻比应为 1:1 (2倍分压)
3. 使用万用表测量GPIO1电压应为电池电压的一半
4. 重新校准:
   - 编辑 power.c 中的分压常数
   - 重新编译烧录
```

### Q8: 存储满后无法继续拍摄

**症状:** SD卡满后拍摄停止

**解决方案:**
```bash
1. 删除不需要的照片，释放空间
2. 或使用以下方式清空:
   - 通过Web界面: POST /format
   - 或取出SD卡用电脑格式化
3. 考虑使用容量更大的SD卡 (64GB+)
```

---

## 性能指标

### 系统参数

| 指标 | 规格 |
|------|------|
| 工作温度 | 0-40°C |
| 工作电压 | 3.3-5.0V |
| 正常功耗 | ~200mA (WiFi on, 拍摄中) |
| 睡眠功耗 | < 10μA (深度睡眠) |
| 拍摄分辨率 | 最高 2048×1536 (UXGA) |
| 帧率 | 最高 30fps (QVGA) |
| JPEG压缩 | 质量 12-63 (可配置) |

### 拍摄性能

以默认配置 (UXGA, 质量=12) 为例：

| 项目 | 数据 |
|------|------|
| 单张文件大小 | ~150-200 KB |
| 拍摄到保存耗时 | ~2-3 秒 |
| 1000张照片总大小 | ~150-200 MB |
| 64GB SD卡容量 | 可拍摄 ~320,000 张 |
| 24小时拍摄(1分钟间隔) | ~1.44 GB |

---

## 安全注意事项

### 电源安全

⚠️ **重要:**
- 仅使用 3.7V LiPo 电池
- 避免过充(超过4.2V)或过放(低于3.0V)
- 长期存放时保持电压在 3.8V
- 不要在高温环境(> 60°C)下工作

### 存储安全

⚠️ **重要:**
- 定期备份重要照片到电脑
- 不要在工作过程中拔出SD卡
- 格式化前确认数据已备份
- 使用校验和验证文件完整性

### 硬件安全

⚠️ **重要:**
- 避免水浸和静电
- 使用合适的外壳保护电路板
- 不要超频运行 (保持 240MHz)
- 确保良好的散热通风

---

## 扩展和定制

### 修改WiFi配置

编辑 `src/wifi/wifi.c`:

```c
// 修改默认 AP 配置
#define WIFI_SSID      "ESP32-Cam"
#define WIFI_PASSWORD  "12345678"

// 修改 STA 模式的目标网络
#define STA_SSID       "Your-WiFi-SSID"
#define STA_PASSWORD   "Your-Password"
```

### 修改文件名前缀

编辑 `src/timelapse/timelapse.c`:

```c
// 修改拍摄文件前缀
#define FILENAME_PREFIX "TIMELAPSE"  // 改为你喜欢的前缀
```

### 添加自定义传感器

1. 在 `include/` 下创建新的头文件
2. 在 `src/` 对应目录实现功能
3. 在 `src/main.c` 的 `app_main()` 中初始化
4. 通过 Web API 暴露控制接口

### 编译自定义固件

```bash
# 修改代码后重新编译
pio run

# 生成二进制文件供分发
# 输出文件: .pio/build/esp32s3cam/firmware.bin
```

---

## 许可证和支持

### 项目信息

- **名称**: ESP32-S3 Timelapse Camera
- **版本**: 1.0.0
- **硬件**: ESP32-S3-N16R8 + OV3660
- **框架**: ESP-IDF 5.3.1

### 获取帮助

如有问题或建议：

1. 查看本文档的常见问题部分
2. 检查源代码注释和日志输出
3. 查阅 ESP-IDF 官方文档: https://docs.espressif.com/
4. 查阅 OV3660 数据手册获取摄像头详细信息

---

## 更新日志

### v1.0.0 (2024-01-29)

- ✅ 初始版本发布
- ✅ 支持 UXGA 分辨率拍摄
- ✅ WiFi AP/STA 双模式
- ✅ Web 远程控制界面
- ✅ SD 卡存储管理
- ✅ 电池监测和低电量保护
- ✅ 深度睡眠功耗优化
- ✅ REST API 接口

---

## 致谢

感谢以下开源项目和资源的支持：

- [ESP-IDF](https://github.com/espressif/esp-idf) - Espressif Systems
- [esp32-camera](https://github.com/espressif/esp32-camera) - OV3660 驱动
- [PlatformIO](https://platformio.org/) - 开发工具

---

**最后更新**: 2024-01-29
**作者**: Claude Code
**维护状态**: ✅ 活跃开发中

