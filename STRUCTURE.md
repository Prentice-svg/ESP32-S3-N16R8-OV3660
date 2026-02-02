# 项目结构说明

## 目录树

```
ESP32-S3-N16R8-OV3660/
├── README.md                    # 项目主文档(详细说明)
├── QUICKSTART.md                # 快速入门指南
├── API.md                        # REST API 完整参考
├── STRUCTURE.md                 # 项目结构说明(本文件)
│
├── platformio.ini               # PlatformIO 配置文件
├── CMakeLists.txt               # CMake 构建配置
├── idf_component.yml            # ESP-IDF 组件配置
├── partitions.csv               # Flash 分区表
│
├── sdkconfig.esp32s3cam         # SDK 配置文件
│
├── include/                     # 头文件目录
│   ├── camera.h                 # 摄像头驱动接口
│   ├── camera_pins.h            # 摄像头引脚定义
│   ├── config.h                 # 系统配置头文件
│   ├── lcd.h                    # LCD 显示屏接口(可选)
│   ├── power.h                  # 电源管理接口
│   ├── sdcard.h                 # SD 卡操作接口
│   ├── sensor.h                 # 传感器类型定义
│   ├── timelapse.h              # 延时摄影功能接口
│   ├── webserver.h              # Web 服务器接口
│   └── wifi.h                   # WiFi 控制接口
│
├── src/                         # 源代码目录
│   ├── main.c                   # 主程序入口点
│   │
│   ├── camera/
│   │   └── camera.c             # 摄像头驱动实现
│   │
│   ├── lcd/
│   │   └── lcd.c                # LCD 显示屏驱动(可选)
│   │
│   ├── power/
│   │   └── power.c              # 电源管理和电池监测
│   │
│   ├── sdcard/
│   │   └── sdcard.c             # SD 卡文件操作实现
│   │
│   ├── timelapse/
│   │   └── timelapse.c          # 延时摄影引擎实现
│   │
│   ├── common/
│   │   └── config.c             # 系统配置管理
│   │
│   └── wifi/
│       ├── wifi.c               # WiFi 连接管理
│       ├── webserver.c          # HTTP Web 服务器实现
│       └── (可能有其他模块)
│
├── data/                        # SPIFFS 文件系统文件(web资源)
│   └── index.html              # Web 界面(内嵌在代码中)
│
├── ref/                         # 参考文档和数据手册
│   └── (芯片和器件文档)
│
└── .pio/                        # PlatformIO 缓存目录(自动生成)
    ├── build/                   # 编译输出
    └── libdeps/                 # 依赖库
```

---

## 核心模块说明

### 1. 主程序 (src/main.c)

**职责:** 系统初始化和主循环

**关键函数:**
- `app_main()` - 启动入口点
- `buttons_init()` - 初始化 BOOT 按钮
- `button_task()` - 按钮处理任务
- `print_status()` - 打印系统信息

**初始化顺序:**
```c
1. 初始化 NVS(非易失性存储)
2. 初始化电源管理
3. 初始化按钮输入
4. 初始化 SD 卡
5. 初始化摄像头
6. 初始化 LCD(可选)
7. 加载配置信息
8. 初始化 WiFi
9. 启动 Web 服务器
10. 初始化延时摄影引擎
11. 启动按钮处理任务
12. 进入主循环
```

**内存布局:**
```
PSRAM (16MB)
├── 摄像头帧缓存      (~10MB)
├── 其他缓冲区         (~4MB)
└── 可用空间          (~2MB)
```

---

### 2. 摄像头模块 (src/camera/)

**职责:** 图像采集和处理

**关键函数:**
- `camera_init()` - 初始化摄像头
- `camera_capture()` - 拍摄一张照片
- `camera_free_fb()` - 释放帧缓存
- `camera_set_framesize()` - 设置分辨率
- `camera_set_quality()` - 设置 JPEG 质量

**配置参数:**
```c
typedef struct {
    int pin_pwdn;           // 电源关闭引脚(通常不用)
    int pin_reset;          // 复位引脚(通常不用)
    int pin_xclk;           // 时钟信号
    int pin_sccb_sda;       // I2C SDA
    int pin_sccb_scl;       // I2C SCL
    int pin_d0 - pin_d7;    // 数据引脚 8 条
    int pin_vsync;          // 行同步
    int pin_href;           // 列同步
    int pin_pclk;           // 像素时钟
} camera_config_t;
```

**JPEG 质量说明:**
```
质量值: 12-63
12    = 最高质量(文件大,~200KB/张)
25    = 高质量(~150KB/张)
50    = 中等质量(~80KB/张)
63    = 最小质量(~30KB/张)
```

---

### 3. 电源管理模块 (src/power/)

**职责:** 电池监测和功耗管理

**关键函数:**
- `power_init()` - 初始化 ADC 和 GPIO
- `power_get_battery_status()` - 获取电池状态
- `power_is_low_battery()` - 检查低电量
- `power_deep_sleep()` - 进入深度睡眠
- `power_usb_connected()` - 检查 USB 电源

**电池计算公式:**
```c
// GPIO1 读取值(mV)
voltage_mv = esp_adc_cal_raw_to_voltage(adc_raw, &adc_chars);

// 实际电池电压 = 分压后的电压 × 2
battery_voltage = (voltage_mv / 1000.0f) * VOLTAGE_DIVIDER;

// 电量百分比(LiPo 3.0V-4.2V)
percentage = (voltage - 3.0) / (4.2 - 3.0) * 100.0f;
```

**功耗模式:**
```
活跃模式          ~200mA
WiFi 开启但空闲   ~100mA
WiFi 关闭         ~50mA
浅睡眠            ~5mA
深度睡眠          <10μA
```

---

### 4. SD 卡模块 (src/sdcard/)

**职责:** 文件系统管理和数据存储

**关键函数:**
- `sdcard_init()` - 初始化 SD 卡
- `sdcard_deinit()` - 卸载 SD 卡
- `sdcard_write_file()` - 写文件
- `sdcard_read_file()` - 读文件
- `sdcard_get_info()` - 获取卡信息
- `sdcard_generate_filename()` - 生成文件名

**支持的接口:**
```c
SDMMC (4-bit 模式)     // 高速，推荐
│
└── 速度: 26MHz, 理论速率 104MB/s
    实际: 30-50MB/s

SPI 模式(备用)          // 低速备用
│
└── 速度: 40MHz, 理论速率 40MB/s
    实际: 5-10MB/s
```

**文件系统:**
```
VFS (Virtual File System)
├── FatFS (主要驱动)
└── /sdcard 挂载点
```

---

### 5. 延时摄影模块 (src/timelapse/)

**职责:** 定时拍摄控制和序列管理

**关键函数:**
- `timelapse_init()` - 初始化定时器
- `timelapse_start()` - 开始延时摄影
- `timelapse_stop()` - 停止延时摄影
- `timelapse_get_state()` - 获取状态
- `timelapse_get_status()` - 获取详细状态
- `save_photo()` - 保存照片到 SD 卡

**定时机制:**
```c
使用 ESP 定时器 (High Resolution Timer)
├── 精度: 1μs
├── 最大延时: 584 年
└── 非阻塞式回调

定时流程:
Start → 等待 interval 秒 → 拍摄 → 保存 → 检查完成
         ↑_________________↓
              循环直到完成
```

**状态机:**
```
IDLE → RUNNING → PAUSED → COMPLETED
 ↓       ↓         ↓
 └───────┴─────────┘
     (任何状态可停止)
```

---

### 6. WiFi 模块 (src/wifi/)

**职责:** 网络连接和远程控制

**子模块:**

#### 6.1 WiFi 控制 (wifi.c)

**关键函数:**
- `wifi_init()` - 初始化 WiFi
- `wifi_start_sta()` - 连接到 WiFi
- `wifi_start_ap()` - 启动 AP 热点
- `wifi_is_connected()` - 检查连接状态
- `wifi_get_ip_address()` - 获取 IP 地址

**模式:**
```
STA 模式 (Station)
├── 连接到现有 WiFi 网络
├── 需要输入 SSID 和密码
└── 获得动态 IP 地址

AP 模式 (Access Point)
├── 作为 WiFi 热点
├── 设置 SSID: ESP32-Cam
├── 密码: 12345678
├── 固定 IP: 192.168.4.1
└── 默认模式(首次启动)
```

#### 6.2 Web 服务器 (webserver.c)

**关键函数:**
- `webserver_init()` - 初始化 HTTP 服务器
- `webserver_start()` - 启动服务器
- `get_index_handler()` - 主页处理
- `get_status_handler()` - 状态 JSON 处理
- `post_start_handler()` - 启动拍摄处理
- (其他请求处理函数)

**路由表:**
```c
GET  /              → 返回 HTML 主页
GET  /status        → 返回 JSON 状态
POST /start         → 开始拍摄
POST /stop          → 停止拍摄
POST /capture       → 单次拍摄
POST /config        → 配置参数
GET  /files         → 文件列表
GET  /download      → 下载文件
POST /reboot        → 重启设备
POST /sleep         → 深度睡眠
POST /format        → 格式化 SD 卡
```

---

### 7. LCD 模块 (src/lcd/) - 可选

**职责:** 显示屏驱动和 UI

**关键函数:**
- `lcd_init()` - 初始化显示屏
- `lcd_clear()` - 清屏
- `lcd_draw_pixel()` - 绘制像素
- `lcd_draw_line()` - 绘制直线
- `lcd_fill_rect()` - 填充矩形
- `lcd_draw_text()` - 绘制文本(占位符)

**硬件:**
```
ST7789 SPI 显示屏
├── 分辨率: 240x320
├── 色深: 16-bit RGB565
├── 速度: 40MHz
└── DMA 加速
```

---

### 8. 配置管理 (src/common/config.c)

**职责:** 配置存储和恢复

**存储位置:** NVS (Non-Volatile Storage)

**关键数据:**
```c
// WiFi 配置
wifi_ssid
wifi_password
wifi_mode

// 拍摄参数
timelapse_interval
timelapse_shots
jpeg_quality
frame_size

// 文件名前缀
filename_prefix
```

**初始化:**
```c
首次运行 → 创建默认配置 → 存储到 NVS
后续运行 → 从 NVS 读取   → 加载配置
```

---

## 数据流

### 完整的拍摄流程

```
Web 请求 /start
    ↓
POST /start handler
    ↓
timelapse_start()
    ↓
启动定时器任务
    ↓
等待 interval 秒
    ↓
camera_capture()
    ├─ 从摄像头读取 RGB 数据
    └─ ESP-IDF 硬件 JPEG 压缩
    ↓
save_photo()
    ├─ 生成文件名
    ├─ 检查 SD 卡可用空间
    └─ 写入 SD 卡
    ↓
检查是否完成
    ├─ 未完成 → 回到"等待"步骤
    └─ 完成 → 设置状态为 COMPLETED
```

### Web 请求流程

```
浏览器请求
    ↓
HTTP 服务器接收
    ↓
路由匹配
    ├─ 匹配成功 → 调用对应 handler
    └─ 匹配失败 → 返回 404
    ↓
Handler 处理
    ├─ 调用相应功能函数
    └─ 生成响应
    ↓
返回 JSON 或 HTML
    ↓
浏览器渲染/解析
```

---

## 编译和链接

### 编译流程

```
CMakeLists.txt
    ↓
idf.py build (或 pio run)
    ↓
编译所有 .c 源文件
    ├─ src/main.c
    ├─ src/camera/camera.c
    ├─ src/power/power.c
    ├─ src/sdcard/sdcard.c
    ├─ src/timelapse/timelapse.c
    ├─ src/wifi/wifi.c
    ├─ src/wifi/webserver.c
    └─ src/common/config.c
    ↓
链接所有 .o 目标文件
    ↓
链接 ESP-IDF 库
    ├─ driver
    ├─ esp_camera
    ├─ fatfs
    ├─ http_server
    ├─ nvs_flash
    ├─ wifi
    └─ 其他
    ↓
生成 firmware.elf
    ↓
转换为 firmware.bin
    ↓
烧录到闪存
```

---

## 内存分布

### Flash 分布(16MB)

```
0x0000_0000 - 0x0003_FFFF    Bootloader (256KB)
0x0004_0000 - 0x000B_FFFF    Partition Table + OTA (512KB)
0x00C0_0000 - 0x00FF_FFFF    App Firmware (4MB)
0x0100_0000 - 0x0FFF_FFFF    SPIFFS/Data (11MB)
```

### RAM 分布(320KB)

```
内部 RAM (320KB)
├─ 内核使用        (~50KB)
├─ 栈空间           (~50KB)
├─ 堆空间           (~120KB)
└─ 其他             (~100KB)
```

### PSRAM 分布(16MB)

```
PSRAM (16MB)
├─ 摄像头帧缓存
│  ├─ UXGA(2048x1536): ~10MB
│  ├─ SXGA(1280x1024): ~4MB
│  └─ QVGA(320x240): ~150KB
├─ Web 缓冲区: ~1MB
├─ 文件 I/O 缓冲: ~512KB
└─ 动态分配: 变化
```

---

## 关键定义和宏

### 引脚定义 (include/camera_pins.h)

```c
// 摄像头接口
#define CAM_PIN_XCLK   8
#define CAM_PIN_SIOD  40
#define CAM_PIN_SIOC  41
#define CAM_PIN_D0   11
#define CAM_PIN_D1    9
// ...等等

// SD 卡接口
#define SD_PIN_CLK   12
#define SD_PIN_CMD   11
#define SD_PIN_D0   13
// ...等等

// 其他
#define BOOT_PIN   0
#define BATTERY_ADC_PIN  1
```

### 系统配置 (include/config.h)

```c
#define TIMELAPSE_DEFAULT_INTERVAL  60
#define TIMELAPSE_MAX_SHOTS         10000
#define JPEG_QUALITY_DEFAULT        12
#define FRAMESIZE_DEFAULT           FRAMESIZE_UXGA
```

---

## 修改和扩展指南

### 添加新功能的步骤

1. **在 include/ 中创建头文件**
   ```c
   // include/myfeature.h
   #ifndef __MYFEATURE_H
   #define __MYFEATURE_H

   esp_err_t myfeature_init(void);
   void myfeature_do_something(void);

   #endif
   ```

2. **在 src/myfeature/ 中实现**
   ```c
   // src/myfeature/myfeature.c
   #include "myfeature.h"

   esp_err_t myfeature_init(void) {
       // 初始化代码
       return ESP_OK;
   }
   ```

3. **在 main.c 中集成**
   ```c
   #include "myfeature.h"

   void app_main(void) {
       // ...其他初始化...
       myfeature_init();  // 添加到初始化序列
   }
   ```

4. **更新 CMakeLists.txt**
   ```cmake
   idf_component_register(
       SRCS "src/main.c" "src/myfeature/myfeature.c" ...
       INCLUDE_DIRS "include"
   )
   ```

### 添加 Web API 的步骤

1. 在 webserver.c 中创建 handler 函数
2. 在路由表中注册
3. 在 HTML 前端添加对应按钮/控件
4. 在 API.md 中文档化

---

## 文件大小参考

```
编译输出大小:

firmware.elf       ~3.2MB (ELF 格式，包含调试信息)
firmware.bin       ~1.8MB (二进制格式，用于烧录)
partitions.bin     ~64B

编译时间(首次):
首次完整编译: ~60s
增量编译:     ~10s
清理重编:     ~60s
```

---

## 常见编译错误和解决

### 错误: undefined reference to `ll_cam_xxx`

**原因:** 摄像头 HAL 库未正确链接

**解决:**
```
检查 platformio.ini 是否包含:
build_flags = ... -DCONFIG_ESP32_CAMERA_OV3660_SUPPORT=y
```

### 错误: No member named `csd` in `sdmmc_card_t`

**原因:** 使用了错误的 SD 卡 API

**解决:**
```
使用最新的 esp-idf FatFS 挂载 API:
esp_vfs_fat_sdmmc_mount() 或
esp_vfs_fat_sdspi_mount()
```

### 错误: STACK_SMASH_DETECTED

**原因:** 栈溢出，局部变量过大

**解决:**
```c
// 错误:
void func() {
    uint8_t big_buffer[100000];  // 栈上分配100KB!
}

// 正确:
void func() {
    uint8_t *big_buffer = malloc(100000);  // 堆上分配
    // ...
    free(big_buffer);
}
```

---

## 性能分析

### 关键函数执行时间

| 操作 | 时间 |
|------|------|
| 摄像头初始化 | ~500ms |
| 拍摄一张照片 | ~800ms (UXGA) |
| JPEG 压缩 | ~400ms |
| SD 卡写入 (100KB) | ~500ms |
| Web 页面加载 | ~1s |

### 瓶颈分析

1. **JPEG 压缩** - 最耗时的操作
   - 使用硬件加速(已启用)
   - 降低分辨率可显著加速

2. **SD 卡写入** - 第二耗时操作
   - 使用高速 Class 10 卡
   - 提高 JPEG 质量值减小文件

3. **WiFi 传输** - 可选操作
   - 可根据网络情况优化

---

## 调试技巧

### 获取详细日志

```bash
pio device monitor -b 115200 -f esp32_exception_decoder
```

### 条件编译调试代码

```c
#ifdef DEBUG
    ESP_LOGI(TAG, "Debug: value=%d", value);
#endif
```

在 platformio.ini 中:
```ini
build_flags = -DDEBUG
```

### 远程调试 (GDB)

```bash
pio device debug
```

---

**文档版本:** 1.0
**最后更新:** 2024-01-29

