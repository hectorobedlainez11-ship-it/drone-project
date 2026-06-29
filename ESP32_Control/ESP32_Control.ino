/*
   ESP32 WiFi Camera + RC Controller for Arduino UNO
   Camera: OV2640
   Communication: UART (Serial2) to Arduino UNO
*/

#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

// =========== WiFi Configuration ===========
const char *ssid = "Drone_ESP32";
const char *password = "12345678";

// =========== UART to Arduino UNO ===========
#define UART_TX 17
#define UART_RX 16
#define UART_BAUD 115200

// =========== Camera Pins (AI-Thinker ESP32-CAM) ===========
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27
#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

// =========== RC Values ===========
volatile int thr = 1000, pit = 0, rol = 0, yaw = 0;
volatile bool newData = false;

WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX, UART_TX);

  // Init camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  // WiFi AP
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(IP);

  // Web routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/control", HTTP_GET, handleControl);
  server.on("/cam.jpg", HTTP_GET, handleJPG);

  server.begin();
  Serial.println("Server started");
}

void loop() {
  server.handleClient();

  if (newData) {
    char buf[64];
    uint8_t ck = 0;
    int len = snprintf(buf, sizeof(buf), "THR:%d,PIT:%d,ROL:%d,YAW:%d,", thr, pit, rol, yaw);
    for (char *p = buf; *p; p++) ck ^= *p;
    snprintf(buf + len, sizeof(buf) - len, "CK:%02X\n", ck);
    Serial2.print(buf);
    newData = false;
  }
}

// =========== Web Handlers ===========

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:Arial;background:#111;color:#fff;text-align:center}
h1{font-size:16px;padding:8px;background:#222}
.stream{width:100%;max-width:640px;margin:auto}
.stream img{width:100%;display:block}
.controls{padding:10px;max-width:500px;margin:auto}
.slider-group{margin:10px 0}
.slider-group label{display:block;font-size:14px;margin-bottom:2px}
.slider-group input{width:100%;height:30px}
.val{font-size:20px;font-weight:bold}
#thrVal{color:#f44}
#pitVal{color:#4f4}
#rolVal{color:#44f}
#yawVal{color:#ff4}
.status{font-size:12px;color:#888;margin-top:10px}
</style>
</head>
<body>
<h1>&#9992; DRONE CONTROL v2</h1>
<div class="stream"><img src="/cam.jpg" id="stream"></div>
<div class="controls">
<div class="slider-group">
<label>THROTTLE <span class="val" id="thrVal">1000</span></label>
<input type="range" id="thr" min="950" max="2000" value="1000" step="1">
</div>
<div class="slider-group">
<label>PITCH <span class="val" id="pitVal">0</span></label>
<input type="range" id="pit" min="-45" max="45" value="0" step="1">
</div>
<div class="slider-group">
<label>ROLL <span class="val" id="rolVal">0</span></label>
<input type="range" id="rol" min="-45" max="45" value="0" step="1">
</div>
<div class="slider-group">
<label>YAW <span class="val" id="yawVal">0</span></label>
<input type="range" id="yaw" min="-45" max="45" value="0" step="1">
</div>
<div class="status">Conectado al drone</div>
</div>
<script>
function send(){var t=document.getElementById('thr').value,p=document.getElementById('pit').value,r=document.getElementById('rol').value,y=document.getElementById('yaw').value;
document.getElementById('thrVal').textContent=t;
document.getElementById('pitVal').textContent=p;
document.getElementById('rolVal').textContent=r;
document.getElementById('yawVal').textContent=y;
fetch('/control?thr='+t+'&pit='+p+'&rol='+r+'&yaw='+y);}
document.getElementById('thr').oninput=send;
document.getElementById('pit').oninput=send;
document.getElementById('rol').oninput=send;
document.getElementById('yaw').oninput=send;
setInterval(function(){document.getElementById('stream').src='/cam.jpg?'+Date.now()},100);
</script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleControl() {
  if (server.hasArg("thr")) thr = constrain(server.arg("thr").toInt(), 950, 2000);
  if (server.hasArg("pit")) pit = constrain(server.arg("pit").toInt(), -45, 45);
  if (server.hasArg("rol")) rol = constrain(server.arg("rol").toInt(), -45, 45);
  if (server.hasArg("yaw")) yaw = constrain(server.arg("yaw").toInt(), -45, 45);
  newData = true;
  server.send(200, "text/plain", "OK");
}

void handleJPG() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { server.send(503, "text/plain", "Camera error"); return; }
  server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}


