# ESP32CAM-RTSP-Multi

**ESP32-CAM firmware with multi-client Nano-RTSP MJPEG server, 100% centralized configuration**

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Enabled-green.svg)](https://platformio.org/)
[![ESP32](https://img.shields.io/badge/ESP32-Compatible-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![RTSP](https://img.shields.io/badge/RTSP-Server-orange.svg)](https://tools.ietf.org/html/rfc2326)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Home Assistant](https://img.shields.io/badge/Home%20Assistant-Integration-blue.svg?logo=home-assistant)](https://www.home-assistant.io/)
[![Tuya](https://img.shields.io/badge/Tuya-Compatible-orange.svg?logo=tuya)](https://developer.tuya.com/en/docs/iot)
[![Node-RED](https://img.shields.io/badge/Node--RED-Compatible-red.svg?logo=nodered)](https://nodered.org/)
[![Domoticz](https://img.shields.io/badge/Domoticz-Compatible-blue.svg)](https://www.domoticz.com/)
[![Jeedom](https://img.shields.io/badge/Jeedom-Compatible-green.svg)](https://www.jeedom.com/)
[![Homebridge](https://img.shields.io/badge/Homebridge-Compatible-purple.svg?logo=homebridge)](https://homebridge.io/)
[![OpenHAB](https://img.shields.io/badge/OpenHAB-Compatible-orange.svg?logo=openhab)](https://www.openhab.org/)
[![VLC](https://img.shields.io/badge/VLC-Compatible-orange.svg?logo=vlc-media-player)](https://www.videolan.org/vlc/)
[![FFmpeg](https://img.shields.io/badge/FFmpeg-Compatible-darkgreen.svg?logo=ffmpeg)](https://ffmpeg.org/)

## 🚀 Main Features

- **Multi-client RTSP MJPEG server** (compatible with FFmpeg, browsers, VLC, etc.)
- **HTTP MJPEG server** for direct browser access
- **Modular architecture** (CameraManager, WiFiManager, Nano-RTSP, HTTPMJPEGServer, Utils)
- **Centralized logger** with verbosity levels
- **Non-blocking memory and timing management**
- **100% centralized configuration in `src/config.h`**
- **No hardcoded values** : everything is modifiable via macros
- **Universal callback type `CaptureCallback`** for image capture
- **PlatformIO and Arduino IDE compatible**
- **Advanced UDP/TCP fallback** for maximum compatibility
- **Safe camera configuration** with optimized memory management
- **Real-time debugging** with comprehensive logging system
- **✅ RTSP/RTP Standards Compliant** : Correct timestamps, sequence numbers, and payload types
- **✅ MJPEG Validation** : SOI/EOI markers verification for data integrity
- **✅ Fixed Framerate** : Stable 15 FPS with precise 66.67ms intervals
- **✅ SDP Consistency** : Framerate announced matches actual framerate

---

## 🌐 Compatibility & Integrations

This firmware is compatible with a wide range of platforms and smart home systems. You can integrate the ESP32CAM-RTSP-Multi in the following environments:

| Platform         | Integration Type         | Example/Notes                                                                 |
|------------------|-------------------------|-------------------------------------------------------------------------------|
| **Home Assistant** | MJPEG/RTSP Camera       | Native integration via [camera](https://www.home-assistant.io/integrations/camera.mjpeg/) or RTSP. |
| **Tuya**          | RTSP Stream             | Use as a generic RTSP camera in Tuya-compatible apps.                         |
| **Node-RED**      | HTTP/RTSP Input         | Use `node-red-contrib-mjpeg-camera` or RTSP nodes.                            |
| **Domoticz**      | MJPEG/RTSP Camera       | Add as a custom camera device.                                                |
| **Jeedom**        | MJPEG/RTSP Camera       | Use the camera plugin for MJPEG/RTSP streams.                                 |
| **Homebridge**    | RTSP/MJPEG Camera       | Use with `homebridge-camera-ffmpeg` plugin.                                   |
| **OpenHAB**       | MJPEG/RTSP Camera       | Integrate via the [IP Camera Binding](https://www.openhab.org/addons/bindings/ipcamera/). |
| **VLC**           | RTSP/MJPEG Stream       | Open the stream directly in VLC.                                              |
| **FFmpeg**        | RTSP/MJPEG Input        | Use for recording, streaming, or analysis.                                    |
| **Web Browsers**  | MJPEG Stream            | Direct access via `http://<ESP32_IP>:80/mjpeg`.                               |
| **OBS Studio**    | RTSP Source             | Add as a media source for live streaming.                                     |

### Example: Home Assistant (configuration.yaml)
```yaml
camera:
  - platform: mjpeg
    name: ESP32CAM
    mjpeg_url: http://192.168.1.100/mjpeg
  - platform: generic
    name: ESP32CAM RTSP
    still_image_url: http://192.168.1.100/capture
    stream_source: rtsp://192.168.1.100:8554/stream=0
```

### Example: Node-RED
- Use the `node-red-contrib-mjpeg-camera` node and set the MJPEG URL to `http://<ESP32_IP>/mjpeg`.
- For RTSP, use a compatible RTSP node or process with FFmpeg.

### Example: Homebridge (config.json)
```json
{
  "platform": "Camera-ffmpeg",
  "cameras": [
    {
      "name": "ESP32CAM",
      "videoConfig": {
        "source": "-rtsp_transport tcp -i rtsp://192.168.1.100:8554/stream=0",
        "stillImageSource": "http://192.168.1.100/capture"
      }
    }
  ]
}
```

### Example: VLC
- Open network stream: `rtsp://192.168.1.100:8554/stream=0` or `http://192.168.1.100/mjpeg`

### Example: OBS Studio
- Add a new Media Source and set the input to the RTSP URL.

---

## 🎯 Advanced Features for FFmpeg

### ⏰ Advanced Timecodes and Metadata
- **PTS/DTS Timecodes** : Generation of presentation and decoding timecodes for optimal FFmpeg compatibility
- **Clock metadata** : Temporal synchronization with NTP and wall clock information
- **Enriched SDP** : Session description with temporal and MJPEG metadata
- **Configurable timecode modes** :
  - Basic mode (simple timestamp)
  - Advanced mode (PTS/DTS with metadata)
  - Expert mode (complete clock synchronization)

### 🔧 Timecode Configuration
```cpp
// Timecode mode for FFmpeg
#define RTSP_TIMECODE_MODE 1 // Recommended advanced mode

// Temporal metadata for FFmpeg
#define RTSP_ENABLE_CLOCK_METADATA 1

// NTP synchronization (optional)
#define RTSP_ENABLE_NTP_SYNC 0
#define RTSP_NTP_SERVER "pool.ntp.org"

// Advanced MJPEG metadata
#define RTSP_ENABLE_MJPEG_METADATA 1
#define RTSP_MJPEG_QUALITY_METADATA 85
```

### 📊 Advanced SDP Metadata
The server automatically generates an enriched SDP with:
- **Temporal information** : Reference clock, NTP timestamps, wall clock
- **MJPEG metadata** : Quality, dimensions, timecode precision
- **Fragmentation information** : Maximum RTP fragment size
- **Specific attributes** : `a=keyframe-only:1` (MJPEG = 100% keyframes)

### 🎬 Optimized FFmpeg Compatibility
- **Correct timecodes** : PTS/DTS calculated according to RTP standards with precise increments
- **"Clock" metadata** : Temporal information to help FFmpeg decode
- **MJPEG structure** : Each frame is an independent keyframe (no GOP)
- **Standard SDP** : Compatible with all RTSP players
- **Regular keyframes** : All MJPEG frames are keyframes (100%)
- **Optimized quality** : MJPEG parameters optimized for compatibility
- **RTP compliant headers** : Strict adherence to RTP/JPEG standards

### 🔄 Timecode Manager
New `TimecodeManager` module for:
- PTS/DTS timecode generation
- NTP clock synchronization
- Temporal metadata for FFmpeg
- RTP timestamp ↔ milliseconds conversion
- Configurable timecode mode management

## 📦 Project Structure

```
ESP32CAM-RTSP-Multi/
├── src/
│   ├── main.cpp              # Main entry point
│   └── config.h              # Centralized configuration (ALL parameters)
├── lib/
│   ├── CameraManager/        # Camera management
│   ├── WiFiManager/          # WiFi management
│   ├── Nano-RTSP/            # RTSP MJPEG server
│   ├── HTTPMJPEGServer/      # HTTP MJPEG server
│   └── Utils/                # Logger, Helpers, Types
├── platformio.ini            # PlatformIO configuration
└── README.md                 # Documentation
```

## ⚙️ Centralized Configuration

**ALL variable parameters are in `src/config.h`** :
- WiFi SSID/Password
- RTSP/HTTP server ports and paths
- Framerate, JPEG quality, camera resolution
- Server names, RTP port ranges, RTP clock
- Thresholds, delays, buffer sizes, HTTP codes, etc.
- Universal callback type :
  ```cpp
  typedef std::function<camera_fb_t*()> CaptureCallback;
  ```

**Example modification** :
```cpp
#define RTSP_PORT 8554
#define RTSP_PATH "/stream=0"
#define HTTP_SERVER_PORT 80
#define HTTP_MJPEG_PATH "/mjpeg"
#define RTSP_FPS 20
#define CAMERA_JPEG_QUALITY 15
#define WIFI_QUALITY_THRESHOLD 20
```

## 🛠️ Installation and Configuration

### Prerequisites
- PlatformIO (recommended) or Arduino IDE
- ESP32-CAM module (AI-Thinker, etc.)
- USB/CH340 drivers installed
- Stable 5V/2A power supply (recommended for optimal performance)

### WiFi Configuration
**⚠️ IMPORTANT : You MUST modify the WiFi credentials in `src/config.h` :**
```cpp
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
```

### Static IP Configuration (Optional)
By default, the ESP32 uses DHCP for automatic IP assignment. If you need a fixed IP address, you can configure static IP:

**To enable static IP, modify in `src/config.h` :**
```cpp
// Enable static IP configuration
#define WIFI_USE_STATIC_IP 1

// Configure your network parameters
#define WIFI_STATIC_IP "192.168.1.100"      // ESP32 IP address
#define WIFI_STATIC_GATEWAY "192.168.1.1"   // Router/Gateway IP address
#define WIFI_STATIC_SUBNET "255.255.255.0"  // Subnet mask
#define WIFI_STATIC_DNS "8.8.8.8"           // Primary DNS server
```

**Example configurations for different networks :**
- **Home network (192.168.1.x)** : Use `192.168.1.100` with gateway `192.168.1.1`
- **Office network (10.0.0.x)** : Use `10.0.0.50` with gateway `10.0.0.1`
- **Custom network (172.16.0.x)** : Use `172.16.0.10` with gateway `172.16.0.1`

**⚠️ IMPORTANT : Make sure the IP address is not already used by another device!**

### Camera Configuration
The firmware includes optimized camera settings for maximum stability:
- **Safe resolution** : VGA (640x480) for optimal performance
- **Dual buffer system** : 2 frame buffers for smooth operation
- **Controlled framerate** : 10 FPS for network stability
- **Memory management** : Automatic frame buffer release to prevent memory leaks

### Compilation and Deployment

#### With PlatformIO (Recommended)
1. Clone the repository :
   ```bash
   git clone https://github.com/iyotee/ESP32CAM-RTSP-Multi.git
   cd ESP32CAM-RTSP-Multi
   ```
2. Modify `src/config.h` with your WiFi parameters
3. Connect the ESP32-CAM in flash mode
4. Compile and upload :
   ```bash
   pio run --target upload
   ```
5. Open the serial monitor to see logs :
   ```bash
   pio device monitor
   ```

#### With Arduino IDE
1. Copy all files to your sketchbook
2. Open `src/main.cpp` in the IDE
3. Modify `src/config.h` with your WiFi parameters
4. Select "AI Thinker ESP32-CAM" board
5. Compile and upload

### Usage

#### Access to video streams
- **RTSP** : `rtsp://<ESP32_IP>:8554/stream=0`
- **HTTP MJPEG** : `http://<ESP32_IP>:80/mjpeg`

#### Transport Protocol
The firmware automatically handles transport protocol selection:
- **UDP mode** : Fast transmission with automatic TCP fallback
- **TCP mode** : Reliable transmission for problematic networks
- **Automatic fallback** : Seamless switching from UDP to TCP if needed

#### Compatible clients
- **VLC Media Player** : Open Media → Open Network Stream
- **ffmpeg** : `ffplay rtsp://<ESP32_IP>:8554/stream=0`
- **Web browser** : Open the HTTP MJPEG URL
- **OBS Studio** : Add an RTSP source
- **Home Assistant** : Integration via MJPEG stream

#### Advanced usage with FFmpeg
The RTSP server is optimized for FFmpeg with advanced timecodes and metadata :

**Simple playback :**
```bash
ffplay rtsp://192.168.1.100:8554/stream=0
```

**Recording with metadata :**
```bash
ffmpeg -i rtsp://192.168.1.100:8554/stream=0 -c copy output.mkv
```

**Force TCP transport (if UDP issues) :**
```bash
ffmpeg -rtsp_transport tcp -i rtsp://192.168.1.100:8554/stream=0 -c copy output.mkv
```

**Conversion with preserved timecodes :**

## 🔧 Troubleshooting and Stability

### Debug Mode
The firmware includes comprehensive debugging capabilities:
- **Log levels** : ERROR, WARN, INFO, DEBUG, VERBOSE
- **Real-time monitoring** : System health checks every 10 seconds
- **Memory tracking** : Automatic memory usage monitoring
- **WiFi diagnostics** : Connection quality and stability monitoring

### Common Issues and Solutions

#### Camera Initialization Problems
- **Symptom** : Firmware stops after WiFi connection
- **Solution** : Ensure stable 5V/2A power supply, check camera ribbon cable

#### Network Connectivity Issues
- **Symptom** : Stream works but drops frequently
- **Solution** : The firmware automatically falls back from UDP to TCP

#### Memory Issues
- **Symptom** : Firmware crashes after a few frames
- **Solution** : Automatic frame buffer management prevents memory leaks

### Performance Optimization
- **Resolution** : VGA (640x480) provides optimal balance of quality and performance
- **Framerate** : 15 FPS ensures network stability and timing consistency
- **Buffers** : 2 frame buffers prevent memory overflow
- **Transport** : Automatic UDP/TCP fallback for maximum compatibility
```bash
ffmpeg -i rtsp://192.168.1.100:8554/stream=0 -c:v libx264 -preset ultrafast output.mp4
```

**Streaming to another server :**
```bash
ffmpeg -i rtsp://192.168.1.100:8554/stream=0 -c copy -f rtsp rtsp://localhost:8554/relay
```

**Metadata analysis :**
```bash
ffprobe -v quiet -print_format json -show_format -show_streams rtsp://192.168.1.100:8554/stream=0
```

#### Example usage with VLC
1. Open VLC Media Player
2. Go to Media → Open Network Stream
3. Enter : `rtsp://192.168.1.100:8554/stream=0`
4. Click Play

## 🔧 Easy Customization
- **No values are hardcoded** in the source code
- **Change a port, path, threshold, quality** = 1 line to modify in `config.h`
- **Add a parameter** : just add it in `config.h` and use it in the code
- **Universal callback** for image capture :
  ```cpp
  httpMJPEGServer.setCaptureCallback(CameraManager::capture);
  ```

### ⚙️ Advanced Timecode Configuration
All timecode options are configurable in `src/config.h` :

**Timecode mode :**
```cpp
#define RTSP_TIMECODE_MODE 1 // 0=basic, 1=advanced, 2=expert
```

**Temporal metadata :**
```cpp
#define RTSP_ENABLE_CLOCK_METADATA 1    // Clock metadata
#define RTSP_ENABLE_NTP_SYNC 0          // NTP synchronization
#define RTSP_NTP_SERVER "pool.ntp.org"  // NTP server
```

**MJPEG metadata :**
```cpp
#define RTSP_ENABLE_MJPEG_METADATA 1    // MJPEG metadata
#define RTSP_MJPEG_QUALITY_METADATA 85  // Quality in metadata
#define RTSP_ENABLE_FRAGMENTATION_INFO 1 // Fragmentation info
```

**Timecode correction :**
```cpp
#define RTSP_FORCE_INCREASING_TIMECODES 1 // Avoid "Non-increasing DTS" errors
```

**MJPEG video compatibility :**
```cpp
#define RTSP_KEYFRAME_INTERVAL 1                    // Each frame = keyframe
#define RTSP_MJPEG_COMPATIBILITY_QUALITY 25         // Optimized quality
#define RTSP_MJPEG_PROFILE_BASELINE 1               // Baseline profile
#define RTSP_ENABLE_VIDEO_COMPATIBILITY_METADATA 1  // Video metadata
#define RTSP_ENABLE_CODEC_INFO 1                    // Detailed codec info
```

## 📝 Changelog (v4.x)

### **Latest Bug Fixes (v4.2)**
- **✅ Fixed SDP framerate inconsistency** - Aligned SDP framerate (15 FPS) with actual system framerate
- **✅ Fixed RTP timestamps** - Corrected timestamp increments to 6000 per frame (90000/15)
- **✅ Fixed RTP sequence numbers** - Ensured proper +1 increment per packet
- **✅ Fixed RTP payload type validation** - Added validation for payload type 26 (MJPEG)
- **✅ Fixed MJPEG headers** - Added SOI (0xFF 0xD8) and EOI (0xFF 0xD9) markers validation
- **✅ Fixed timing control** - Implemented precise 66.67ms intervals for 15 FPS
- **✅ Fixed logging macros** - Corrected all LOG_WARN/LOG_ERROR to LOG_WARNF/LOG_ERRORF
- **🔧 Enhanced data integrity** - Automatic detection of corrupted JPEG frames
- **🔧 Improved standards compliance** - Full RTSP/RTP standards adherence

### **Previous Features (v4.0)**
- **Advanced PTS/DTS timecodes** for optimal FFmpeg compatibility
- **Clock metadata** with optional NTP synchronization
- **Enriched SDP** with temporal and MJPEG information
- **Timecode manager** (TimecodeManager) for metadata generation
- **Configurable timecode modes** (basic, advanced, expert)
- **MJPEG metadata** in SDP for better compatibility
- Total centralization of parameters in `config.h`
- No more hardcoded values
- Universal `CaptureCallback` type
- Modular and maintainable structure
- Validated PlatformIO build

## ❓ FAQ and Troubleshooting

### Common Issues

#### Module doesn't connect to WiFi
- Verify that WiFi credentials in `config.h` are correct
- Ensure the WiFi network is 2.4GHz (ESP32-CAM doesn't support 5GHz)
- Check distance and WiFi signal strength

#### Static IP configuration issues
- **IP address conflict** : Make sure the static IP is not already used by another device
- **Wrong gateway** : Verify that the gateway IP matches your router's IP address
- **Wrong subnet mask** : Use `255.255.255.0` for most home networks (192.168.x.x)
- **DNS issues** : Try using `8.8.8.8` (Google DNS) or your router's DNS
- **Network compatibility** : Ensure the static IP is in the same subnet as your router

**Advantages of static IP :**
- **Fixed address** : ESP32 always has the same IP address
- **Port forwarding** : Easier to configure router port forwarding
- **Network integration** : Better for integration with other network devices
- **Predictable access** : Always know the ESP32's IP address

**When to use DHCP (default) :**
- **Simple setup** : No network configuration needed
- **Dynamic networks** : Works with changing network configurations
- **Multiple devices** : Avoids IP conflicts automatically
- **Plug and play** : Works immediately after flashing

#### No video stream
- Verify that the IP displayed in logs is correct
- Test the HTTP MJPEG stream in a browser first
- Check that ports 8554 (RTSP) and 80 (HTTP) are not blocked by your router

#### Slow or unstable stream
- Reduce resolution in `config.h` (use `FRAMESIZE_QVGA` or `FRAMESIZE_VGA`)
- Reduce JPEG quality (increase `CAMERA_JPEG_QUALITY` value)
- Limit the number of simultaneous connected clients
- Check WiFi signal quality

#### Unexpected restarts
- Check power supply (ESP32-CAM needs good current supply)
- Reduce XCLK frequency in `config.h`
- Increase delays in configuration

### Performance Optimization
- **Recommended resolution** : `FRAMESIZE_QVGA` (320x240) or `FRAMESIZE_VGA` (640x480)
- **Optimal JPEG quality** : 10-20 for good quality/bitrate compromise
- **Stable framerate** : 15 FPS fixed for optimal timing consistency
- **Number of clients** : Maximum 3-4 simultaneous clients

### Questions about timecodes and FFmpeg

#### FFmpeg doesn't read RTSP stream correctly
- Verify that `RTSP_TIMECODE_MODE` is set to 1 (advanced mode)
- Enable clock metadata : `RTSP_ENABLE_CLOCK_METADATA 1`
- Use command : `ffplay -rtsp_transport tcp rtsp://<IP>:8554/stream=0`

#### Timecodes seem incorrect
- Check NTP synchronization if enabled
- Use expert mode (`RTSP_TIMECODE_MODE 2`) for more precision
- Enable `RTSP_FORCE_INCREASING_TIMECODES 1` to avoid "Non-increasing DTS" errors
- Check logs to see generated timecodes
- Verify that `RTSP_FPS` is consistent with real framerate

#### Compatibility issues with other players
- Basic mode (`RTSP_TIMECODE_MODE 0`) offers maximum compatibility
- Disable advanced metadata if necessary
- Test with VLC before FFmpeg

#### Video compatibility issues
- **Keyframe errors** : Verify that `RTSP_KEYFRAME_INTERVAL` is set to 1
- **Video quality** : Adjust `RTSP_MJPEG_COMPATIBILITY_QUALITY` (20-40 recommended)
- **RTP headers** : Enable `RTSP_ENABLE_VIDEO_COMPATIBILITY_METADATA`
- **Codec info** : Enable `RTSP_ENABLE_CODEC_INFO` for more compatibility

### Logs and debug
Enable DEBUG log level in `config.h` :
```cpp
#define LOG_LEVEL LOG_DEBUG
```

## 📄 License
MIT

## 🙏 Acknowledgments
- ESP32 community, PlatformIO, Arduino
- Users and contributors for feedback and suggestions

---
**For any questions, open an issue on GitHub or consult the online documentation.**
