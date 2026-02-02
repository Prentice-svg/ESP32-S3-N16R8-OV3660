# REST API 文档

## API 基础信息

- **基础URL**: `http://<device-ip>`
- **协议**: HTTP/1.1
- **内容类型**: JSON
- **认证**: 无 (局域网内)
- **速率限制**: 无限制
- **超时**: 30 秒

---

## 1. 状态查询 API

### GET /status

获取设备当前状态和传感器数据。

**请求示例:**
```bash
curl http://192.168.4.1/status
```

**响应示例:**
```json
{
  "state": 1,
  "current_shot": 234,
  "total_shots": 1000,
  "next_shot_sec": 45,
  "battery_voltage": 4.15,
  "battery_percent": 85.3,
  "free_bytes": 62449643520,
  "wifi_mode": 1,
  "uptime_sec": 3600,
  "frame_size": 2,
  "jpeg_quality": 12
}
```

**响应字段说明:**

| 字段 | 类型 | 说明 |
|------|------|------|
| state | int | 拍摄状态: 0=空闲 1=运行中 2=暂停 3=完成 |
| current_shot | uint | 已拍摄的照片数 |
| total_shots | uint | 计划拍摄的总数 |
| next_shot_sec | uint | 下次拍摄倒计时(秒) |
| battery_voltage | float | 电池电压(V) |
| battery_percent | float | 电池容量百分比(%) |
| free_bytes | uint64 | SD卡可用空间(字节) |
| wifi_mode | int | WiFi模式: 1=STA 2=AP |
| uptime_sec | uint | 设备运行时间(秒) |
| frame_size | int | 当前分辨率代码(见下表) |
| jpeg_quality | int | JPEG质量 |

**分辨率代码对照:**

```c
0 = 96x96
1 = QQVGA (160x120)
2 = QCIF (176x144)
3 = HQVGA (240x176)
4 = 240x240
5 = QVGA (320x240)
6 = CIF (400x296)
7 = HVGA (480x360)
8 = VGA (640x480)
9 = SVGA (800x600)
10 = XGA (1024x768)
11 = HD (1280x720)
12 = SXGA (1280x1024)
13 = UXGA (2048x1536)
```

**HTTP 状态码:**
- `200 OK` - 请求成功
- `500 Internal Server Error` - 设备内部错误

---

## 2. 拍摄控制 API

### POST /start

开始延时摄影拍摄。

**请求示例:**
```bash
curl -X POST http://192.168.4.1/start
```

**响应示例:**
```json
{
  "success": true,
  "message": "Timelapse started",
  "state": 1
}
```

**错误响应:**
```json
{
  "success": false,
  "error": "Already running",
  "state": 1
}
```

**状态码:**
- `200 OK` - 启动成功
- `400 Bad Request` - 已在运行或参数错误
- `500 Internal Server Error` - 系统错误

---

### POST /stop

停止当前的延时摄影拍摄。

**请求示例:**
```bash
curl -X POST http://192.168.4.1/stop
```

**响应示例:**
```json
{
  "success": true,
  "message": "Timelapse stopped",
  "state": 0,
  "shots_taken": 234
}
```

**备注:**
- 已拍摄的照片会被保留
- 可随时重新启动继续拍摄
- 会立即停止，不等待当前拍摄完成

---

### POST /capture

立即拍摄一张照片。

**请求示例:**
```bash
curl -X POST http://192.168.4.1/capture
```

**响应示例:**
```json
{
  "success": true,
  "message": "Photo captured",
  "filename": "TIMELAPSE_20240129_120000_00001.jpg",
  "size_bytes": 185634
}
```

**错误响应:**
```json
{
  "success": false,
  "error": "Failed to capture",
  "camera_error": "Frame buffer error"
}
```

**备注:**
- 即使在运行模式下也可调用
- 拍摄时间取决于分辨率和质量设置

---

## 3. 配置 API

### POST /config

配置拍摄参数。支持以下参数的任意组合。

**请求示例:**
```bash
curl -X POST "http://192.168.4.1/config?interval=60&shots=1000&quality=12&framesize=12"
```

**请求参数:**

| 参数 | 类型 | 范围 | 说明 |
|------|------|------|------|
| interval | int | 1-3600 | 拍摄间隔(秒) |
| shots | int | 1-10000 | 计划拍摄总数 |
| quality | int | 12-63 | JPEG质量(越小越好) |
| framesize | int | 0-13 | 分辨率代码(见状态API) |
| prefix | string | 1-32字符 | 文件名前缀 |

**响应示例:**
```json
{
  "success": true,
  "message": "Configuration updated",
  "config": {
    "interval": 60,
    "shots": 1000,
    "quality": 12,
    "framesize": 12,
    "prefix": "TIMELAPSE"
  }
}
```

**错误响应:**
```json
{
  "success": false,
  "error": "Invalid parameter",
  "details": "Quality must be between 12 and 63"
}
```

**示例场景:**

```bash
# 设置高分辨率高质量拍摄
curl -X POST "http://192.168.4.1/config?framesize=12&quality=12"

# 快速拍摄低分辨率
curl -X POST "http://192.168.4.1/config?framesize=5&quality=30&interval=5"

# 改变文件前缀
curl -X POST "http://192.168.4.1/config?prefix=SUNSET"

# 修改拍摄数量
curl -X POST "http://192.168.4.1/config?shots=5000"
```

---

## 4. 文件管理 API

### GET /files

获取 SD 卡上的文件列表。

**请求示例:**
```bash
curl http://192.168.4.1/files
```

**响应示例:**
```json
{
  "success": true,
  "files": [
    "TIMELAPSE_20240129_120000_00001.jpg",
    "TIMELAPSE_20240129_120100_00002.jpg",
    "TIMELAPSE_20240129_120200_00003.jpg"
  ],
  "count": 3,
  "total_size": 557000,
  "free_space": 62449643520,
  "total_space": 67108864000
}
```

**响应字段说明:**

| 字段 | 说明 |
|------|------|
| files | 文件名数组 |
| count | 文件总数 |
| total_size | 所有文件的总大小(字节) |
| free_space | 可用空间(字节) |
| total_space | SD卡总容量(字节) |

**分页查询 (可选):**

```bash
# 获取第 1-50 个文件
curl "http://192.168.4.1/files?offset=0&limit=50"

# 获取第 51-100 个文件
curl "http://192.168.4.1/files?offset=50&limit=50"
```

---

### GET /download

下载指定的照片文件。

**请求示例:**
```bash
curl http://192.168.4.1/download?name=TIMELAPSE_20240129_120000_00001.jpg > photo.jpg
```

**URL 参数:**

| 参数 | 类型 | 说明 |
|------|------|------|
| name | string | 文件名(必须) |

**响应:**
- `200 OK` - 返回文件内容 (Content-Type: image/jpeg)
- `404 Not Found` - 文件不存在
- `500 Internal Server Error` - 读取错误

**响应头示例:**
```
Content-Type: image/jpeg
Content-Length: 185634
Content-Disposition: attachment; filename="TIMELAPSE_20240129_120000_00001.jpg"
```

**批量下载脚本:**
```bash
#!/bin/bash
curl http://192.168.4.1/files | jq -r '.files[]' | while read file; do
  echo "下载: $file"
  curl -o "$file" "http://192.168.4.1/download?name=$file"
done
```

---

### POST /format

格式化 SD 卡(危险操作!)。

**请求示例:**
```bash
curl -X POST "http://192.168.4.1/format?confirm=true"
```

**URL 参数:**

| 参数 | 类型 | 说明 |
|------|------|------|
| confirm | bool | 必须为 true，防止误操作 |

**响应示例:**
```json
{
  "success": true,
  "message": "SD Card formatted successfully"
}
```

**⚠️ 警告:**
- 此操作会删除 SD 卡上的所有文件！
- 操作不可逆转
- 格式化过程需要 30-60 秒
- 格式化期间设备不响应其他请求

**确认删除的正确方式:**
```bash
# 仅当参数为 confirm=true 时才执行
curl -X POST "http://192.168.4.1/format?confirm=true"

# 以下请求会被拒绝:
curl -X POST "http://192.168.4.1/format"
curl -X POST "http://192.168.4.1/format?confirm=false"
```

---

## 5. 系统控制 API

### POST /reboot

重启设备。

**请求示例:**
```bash
curl -X POST http://192.168.4.1/reboot
```

**响应示例:**
```json
{
  "success": true,
  "message": "Device rebooting",
  "reason": "User requested"
}
```

**备注:**
- 设备会在 2 秒后重启
- 重启期间 WiFi 连接会中断
- 之前的配置会被保留

---

### POST /sleep

进入深度睡眠模式。

**请求示例:**
```bash
# 睡眠 3600 秒(1小时)后自动唤醒
curl -X POST "http://192.168.4.1/sleep?seconds=3600"

# 睡眠直到 BOOT 按钮被按下
curl -X POST "http://192.168.4.1/sleep"
```

**URL 参数:**

| 参数 | 类型 | 范围 | 说明 |
|------|------|------|------|
| seconds | int | 0-unlimited | 睡眠时长(秒)，0表示无限睡眠 |

**响应示例:**
```json
{
  "success": true,
  "message": "Entering deep sleep",
  "duration": 3600,
  "wakeup_source": "timer"
}
```

**唤醒方式:**

```c
// 定时唤醒
esp_sleep_enable_timer_wakeup(3600 * 1000000);

// GPIO 唤醒(BOOT 按钮)
esp_sleep_enable_ext1_wakeup(GPIO_NUM_0, ESP_EXT1_WAKEUP_ANY_LOW);

// 进入睡眠
esp_deep_sleep_start();
```

**能量消耗:**
- 正常运行: ~200 mA
- 浅睡眠: ~5 mA
- 深度睡眠: < 10 μA

---

### GET /info

获取设备信息。

**请求示例:**
```bash
curl http://192.168.4.1/info
```

**响应示例:**
```json
{
  "device_name": "ESP32-S3 Timelapse Camera",
  "firmware_version": "1.0.0",
  "hardware_version": "ESP32-S3-N16R8",
  "mac_address": "3C:71:05:12:34:56",
  "ip_address": "192.168.4.1",
  "wifi_mode": "AP",
  "wifi_ssid": "ESP32-Cam",
  "uptime_seconds": 3600,
  "free_heap": 125000,
  "total_heap": 500000,
  "board_temperature": 35.5,
  "camera_model": "OV3660",
  "sdcard_present": true,
  "battery_voltage": 4.15
}
```

---

## 6. WiFi 管理 API

### POST /wifi/config

配置 WiFi 连接参数。

**请求示例:**
```bash
curl -X POST "http://192.168.4.1/wifi/config?mode=sta&ssid=MyWiFi&password=secret123"
```

**URL 参数:**

| 参数 | 类型 | 值 | 说明 |
|------|------|-----|------|
| mode | string | sta/ap | WiFi 模式 |
| ssid | string | 1-32 | 网络 SSID |
| password | string | 8-64 | 网络密码 |

**响应示例:**
```json
{
  "success": true,
  "message": "WiFi configuration saved",
  "mode": "sta",
  "ssid": "MyWiFi",
  "applying": true
}
```

**备注:**
- 改变为 STA 模式时设备会重启
- 可能需要 10-20 秒建立新连接
- 如连接失败，设备会回退到 AP 模式

---

## 7. 错误处理

### 标准错误响应格式

```json
{
  "success": false,
  "error": "Error description",
  "error_code": 400,
  "details": "Additional error information"
}
```

### 常见错误代码

| 代码 | 说明 | 解决方案 |
|------|------|---------|
| 400 | 坏请求 | 检查参数是否正确 |
| 404 | 资源不存在 | 检查文件名或 API 路径 |
| 500 | 服务器错误 | 查看设备日志，检查硬件 |
| 503 | 服务不可用 | 设备忙碌或内存不足 |

### 错误恢复建议

```bash
# 重试机制
max_retries=3
for i in $(seq 1 $max_retries); do
  response=$(curl -s http://192.168.4.1/status)
  if [ $? -eq 0 ]; then
    echo "请求成功"
    break
  else
    echo "请求失败，$((10 * i)) 秒后重试..."
    sleep $((10 * i))
  fi
done
```

---

## 8. 使用示例

### 完整的拍摄工作流程

```bash
#!/bin/bash

DEVICE="http://192.168.4.1"

# 1. 检查设备状态
echo "检查设备状态..."
curl -s $DEVICE/status | jq .

# 2. 配置拍摄参数
echo "配置拍摄参数..."
curl -s -X POST "$DEVICE/config?interval=120&shots=500&quality=15" | jq .

# 3. 开始拍摄
echo "开始延时摄影..."
curl -s -X POST $DEVICE/start | jq .

# 4. 监控进度
for i in {1..60}; do
  echo "第 $i 分钟..."
  curl -s $DEVICE/status | jq '{current_shot, total_shots, next_shot_sec}'
  sleep 60
done

# 5. 停止拍摄
echo "停止拍摄..."
curl -s -X POST $DEVICE/stop | jq .

# 6. 获取文件列表
echo "获取文件列表..."
curl -s $DEVICE/files | jq '{count, total_size}'

# 7. 下载所有文件
echo "下载所有照片..."
mkdir -p photos
curl -s $DEVICE/files | jq -r '.files[]' | while read file; do
  curl -s -o "photos/$file" "$DEVICE/download?name=$file"
done

echo "完成!"
```

### Python 控制脚本

```python
import requests
import time
import json

class TimelapseCameraControl:
    def __init__(self, device_ip="192.168.4.1", port=80):
        self.base_url = f"http://{device_ip}:{port}"

    def get_status(self):
        """获取设备状态"""
        response = requests.get(f"{self.base_url}/status")
        return response.json()

    def start_timelapse(self):
        """开始拍摄"""
        response = requests.post(f"{self.base_url}/start")
        return response.json()

    def stop_timelapse(self):
        """停止拍摄"""
        response = requests.post(f"{self.base_url}/stop")
        return response.json()

    def configure(self, **kwargs):
        """配置参数"""
        params = "&".join([f"{k}={v}" for k, v in kwargs.items()])
        response = requests.post(f"{self.base_url}/config?{params}")
        return response.json()

    def capture(self):
        """拍摄一张照片"""
        response = requests.post(f"{self.base_url}/capture")
        return response.json()

    def get_files(self):
        """获取文件列表"""
        response = requests.get(f"{self.base_url}/files")
        return response.json()

    def download_file(self, filename, save_path):
        """下载文件"""
        response = requests.get(
            f"{self.base_url}/download",
            params={"name": filename}
        )
        with open(save_path, 'wb') as f:
            f.write(response.content)

    def monitor(self, interval=10):
        """监控拍摄进度"""
        while True:
            status = self.get_status()
            print(f"进度: {status['current_shot']}/{status['total_shots']}")
            print(f"下次拍摄: {status['next_shot_sec']}s")
            print(f"电池: {status['battery_percent']:.1f}%")
            print("---")
            time.sleep(interval)

# 使用示例
camera = TimelapseCameraControl("192.168.4.1")
camera.configure(interval=60, shots=1000)
camera.start_timelapse()
camera.monitor(interval=30)
```

---

## 9. 性能考虑

### 请求限制

- **并发连接**: 支持最多 10 个并发 HTTP 连接
- **请求大小**: 最大 4KB
- **响应大小**: 文件列表最大 100KB
- **超时**: 所有请求 30 秒超时

### 优化建议

```bash
# 1. 使用连接复用
curl -b cookie.txt -c cookie.txt http://192.168.4.1/status

# 2. 减少轮询频率
# 不要频繁调用 /status，建议间隔 5-10 秒

# 3. 批量操作
# 一次性获取所有文件列表，而不是逐个检查

# 4. 使用 gzip 压缩(如果支持)
curl -H "Accept-Encoding: gzip" http://192.168.4.1/status | gunzip
```

---

## 10. 故障排查

### 连接问题

```bash
# 测试基本连接
ping 192.168.4.1

# 测试 HTTP 连接
curl -v http://192.168.4.1/

# 测试 DNS 解析
nslookup esp32-cam.local
curl http://esp32-cam.local/
```

### 响应缓慢

```bash
# 增加超时时间
curl --connect-timeout 10 --max-time 60 http://192.168.4.1/status

# 检查网络带宽
iperf3 -c 192.168.4.1
```

### 文件下载失败

```bash
# 显示详细信息
curl -v http://192.168.4.1/download?name=test.jpg

# 验证文件完整性
curl -I http://192.168.4.1/download?name=test.jpg  # 检查 Content-Length
```

---

**文档版本**: 1.0
**最后更新**: 2024-01-29

