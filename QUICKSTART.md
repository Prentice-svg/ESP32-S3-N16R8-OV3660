# 快速入门指南

## 5分钟快速开始

### 步骤 1: 硬件组装 (5分钟)

```
1. 连接摄像头到 ESP32-S3
   - 参考主文档中的接线图
   - 确保所有连接稳固

2. 插入 SD 卡(可选，但推荐)
   - 使用 Class 10 高速卡
   - 容量 8GB 以上

3. 连接电源
   - 使用 USB-C 线连接到电脑
   - 或连接 3.7V LiPo 电池
```

### 步骤 2: 编译和烧录 (10分钟)

```bash
# 1. 进入项目目录
cd ESP32-S3-N16R8-OV3660

# 2. 使用 PlatformIO 编译
pio run

# 3. 烧录到设备(自动识别端口)
pio run -t upload

# 4. 打开串口监视器查看日志
pio device monitor --baud 115200
```

### 步骤 3: 连接 WiFi (2分钟)

```
设备启动后，在你的手机/电脑上：

1. 打开 WiFi 列表
2. 搜索并连接到 "ESP32-Cam"
3. 输入密码: 12345678
4. 打开浏览器，访问: http://192.168.4.1
```

### 步骤 4: 开始拍摄 (1分钟)

```
在 Web 界面上：

1. 设置拍摄间隔: 60 秒
2. 设置总拍摄数: 100 张
3. 点击 [Start] 按钮
4. 观看实时进度条

✅ 完成！相机开始延时摄影
```

---

## 常见配置修改

### 修改 WiFi 密码

编辑 `include/config.h`:
```c
#define DEFAULT_WIFI_PASSWORD  "mynewpassword"
```

### 修改拍摄间隔默认值

编辑 `include/timelapse.h`:
```c
#define DEFAULT_INTERVAL_SEC  120  // 改为 120 秒
```

### 修改图像质量

编辑 `src/main.c` 的 `app_main()`:
```c
.jpeg_quality = 20,  // 改为 20 (质量更高)
```

---

## 常见问题速查

| 问题 | 症状 | 快速解决 |
|------|------|---------|
| 相机无法工作 | 启动日志有错误 | 检查接线，重启 |
| WiFi 无法连接 | 找不到 AP | 检查路由器，重启设备 |
| SD 卡无法读取 | 挂载失败 | 检查接线，格式化SD卡 |
| 拍摄太慢 | 间隔很长 | 降低分辨率，增加质量值 |
| 电池异常 | 显示0% | 检查ADC接线 |
| Web 无法访问 | 连接超时 | 检查WiFi连接，查看IP |

---

## 生成视频文件

将拍摄的JPG序列转换为视频(Linux/Mac):

```bash
# 使用 FFmpeg(需要预先安装)

# 方式 1: MP4 视频
ffmpeg -framerate 30 -pattern_type glob -i '*.jpg' \
  -c:v libx264 -pix_fmt yuv420p output.mp4

# 方式 2: GIF 动画
ffmpeg -framerate 15 -pattern_type glob -i '*.jpg' \
  -vf scale=1280:-1 output.gif

# 方式 3: WebM 视频
ffmpeg -framerate 30 -pattern_type glob -i '*.jpg' \
  -c:v libvpx-vp9 -pix_fmt yuv420p output.webm
```

使用 Python 脚本(跨平台):

```python
import subprocess
import os

image_folder = "./sdcard_images"
output_file = "timelapse.mp4"
fps = 30

# 获取所有jpg文件并排序
images = sorted([f for f in os.listdir(image_folder) if f.endswith('.jpg')])

# 创建文件列表
with open('filelist.txt', 'w') as f:
    for img in images:
        f.write(f"file '{image_folder}/{img}'\n")

# 使用 FFmpeg 生成视频
cmd = f"ffmpeg -f concat -safe 0 -i filelist.txt -c:v libx264 -pix_fmt yuv420p {output_file}"
subprocess.run(cmd, shell=True)

print(f"视频已生成: {output_file}")
```

---

## 命令行工具使用

### 使用 curl 控制摄像机

```bash
# 获取设备状态
curl http://192.168.4.1/status | jq

# 开始拍摄
curl -X POST http://192.168.4.1/start

# 停止拍摄
curl -X POST http://192.168.4.1/stop

# 拍摄一张照片
curl -X POST http://192.168.4.1/capture

# 配置参数
curl -X POST "http://192.168.4.1/config?interval=60&shots=1000&quality=12"

# 获取文件列表
curl http://192.168.4.1/files | jq

# 下载文件
curl http://192.168.4.1/download?name=TIMELAPSE_20240129_120000_00001.jpg > photo.jpg

# 重启设备
curl -X POST http://192.168.4.1/reboot

# 进入睡眠(1小时后唤醒)
curl -X POST http://192.168.4.1/sleep?seconds=3600
```

### 批量下载所有照片

```bash
#!/bin/bash

# 获取文件列表
files=$(curl -s http://192.168.4.1/files | jq -r '.files[]')

# 创建输出目录
mkdir -p photos

# 逐个下载
for file in $files; do
    echo "下载: $file"
    curl -o "photos/$file" "http://192.168.4.1/download?name=$file"
done

echo "完成！所有照片已保存到 ./photos 目录"
```

---

## LED 指示灯说明

| LED | 含义 |
|-----|------|
| 红灯(闪烁) | WiFi 搜索中 |
| 红灯(常亮) | WiFi 已连接 |
| 蓝灯(闪烁) | 拍摄中 |
| 蓝灯(常亮) | 待机中 |
| 绿灯 | 电池正常 |
| 红灯(脉冲) | 低电量警告 |

---

## 日志输出说明

### 正常启动日志示例

```
I (100) main: ========================================
I (101) main: ESP32-S3 Timelapse Camera v1.0
I (102) main: ========================================
I (103) main: Initial memory: 500000 bytes
I (200) nvs_flash: NVS Flash initialized
I (250) power: Power management initialized
I (300) power: ADC calibrated using eFuse Two Point values
I (350) sdcard: Initializing SD Card...
I (500) sdcard: SD Card mounted: SD Card
I (600) camera: Initializing camera with pin config:
I (700) camera: Camera initialized (Sensor ID: 0x3660)
I (800) wifi: WiFi initialized in AP mode
I (900) webserver: Web server started at http://192.168.4.1
I (950) timelapse: Timelapse engine initialized
I (1000) main: Timelapse camera ready!
```

### 常见错误日志

```
E (300) power: Failed to initialize ADC
    → 检查 power_init() 返回值，查看ADC配置

E (400) sdcard: Failed to mount SD Card: No such device
    → SD 卡接线问题，检查 CLK, CMD, D0-D3

E (500) camera: Camera init failed
    → 摄像头接线问题，检查 XCLK, SIOD, SIOC

W (600) wifi: WiFi connection failed
    → WiFi 密码错误或信号弱，查看连接日志

E (700) webserver: Failed to start HTTP server
    → 端口被占用或内存不足
```

---

## 性能调优建议

### 如果拍摄太慢

1. **降低分辨率**
   ```
   改为 SXGA (1280×1024) 或 XGA (1024×768)
   可增加 50% 的拍摄速度
   ```

2. **提高 JPEG 质量值**
   ```
   改为 quality=20 或更高
   文件更小，传输更快
   ```

3. **使用更快的 SD 卡**
   ```
   UHS-II 或 V90 等级卡
   顺序写入速度 > 90MB/s
   ```

### 如果占用内存过多

1. **减少帧缓冲**
   ```c
   .fb_count = 1,  // 改为只用 1 个缓冲
   ```

2. **关闭不需要的功能**
   ```c
   // 注释掉 LCD 初始化
   // ret = lcd_init(...);
   ```

3. **使用小分辨率**
   ```
   改为 QVGA (320×240)
   仅需约 100KB 内存
   ```

### 如果功耗过高

1. **增加拍摄间隔**
   ```
   改为每 5 分钟拍摄一次
   WiFi 保持连接但大部分时间休眠
   ```

2. **启用自动睡眠**
   ```
   拍摄完成后进入浅睡眠
   定时唤醒继续拍摄
   ```

3. **关闭 WiFi**
   ```
   拍摄完成后禁用 WiFi 模块
   重新启用时恢复连接
   ```

---

## 调试技巧

### 启用详细日志

编辑 `platformio.ini`:
```ini
build_flags =
    ...
    -DESP_LOGLEVEL=5  # 调试级别日志
    -DLOG_LOCAL_LEVEL=5
```

### 连接 JTAG 调试器 (可选)

```bash
# 使用 OpenOCD 进行硬件调试
openocd -f board/esp32s3-builtin.cfg

# 在另一个终口使用 GDB
xtensa-esp-elf-gdb ./build/app.elf
```

### 内存泄漏检测

在 `menuconfig` 中启用：
```
Component config → FreeRTOS → Enable memory debug features
```

查看堆内存统计：
```c
#include "esp_heap_trace.h"

heap_trace_init_standalone(trace_record, 100);
heap_trace_start(HEAP_TRACE_ALL);

// ... 运行代码 ...

heap_trace_stop();
heap_trace_dump(stdout);
```

---

## 下一步

- 📚 详细配置: 查看 `README.md` 的"硬件需求"和"配置说明"
- 🔧 API 参考: 查看 `README.md` 的"REST API 参考"
- 🐛 故障排除: 查看 `README.md` 的"常见问题和故障排除"
- 📖 源代码: 查看 `src/` 目录下的源文件和注释

---

**祝您使用愉快！** 🎉

