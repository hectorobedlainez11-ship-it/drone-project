/*
   ESP32 WiFi Camera + RC Controller for Arduino UNO (SIN POWER HUB)
   Camera: OV2640
   Communication: UART (Serial2) to Arduino UNO
   Alimentacion: 5V desde BEC del ESC2 (cable rojo del servo)
*/

#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

// =========== WiFi Configuration ===========
const char *ssid = "Drone_ESP32";
const char *password = "12345678";

// =========== UART to Arduino UNO ===========
// GPIO1 (TX) → Arduino D0 (RX),  GPIO3 (RX) ← Arduino D1 (TX)
#define UART_TX 1
#define UART_RX 3
#define UART_BAUD 57600

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
  Serial.begin(UART_BAUD);

  // WiFi AP (siempre, aunque la cámara falle)
  WiFi.softAP(ssid, password);

  // Web routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/control", HTTP_GET, handleControl);
  server.on("/cam.jpg", HTTP_GET, handleJPG);

  server.begin();

  // Init camera (puede fallar, pero el WiFi ya está arriba)
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
    return;
  }
}

void loop() {
  server.handleClient();

  if (newData) {
    char buf[64];
    uint8_t ck = 0;
    int len = snprintf(buf, sizeof(buf), "THR:%d,PIT:%d,ROL:%d,YAW:%d,", thr, pit, rol, yaw);
    for (char *p = buf; *p; p++) ck ^= *p;
    snprintf(buf + len, sizeof(buf) - len, "CK:%02X\n", ck);
    Serial.print(buf);
    newData = false;
  }
}

// =========== Web Handlers ===========

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:Arial,Helvetica,sans-serif;background:#0a0a0a;color:#fff;display:flex;flex-direction:column;align-items:center;min-height:100dvh;padding:8px}
h1{font-size:18px;padding:12px 16px;background:#1a1a2e;border-radius:12px;width:100%;max-width:500px;text-align:center;letter-spacing:1px;margin-bottom:12px}
.stream{width:100%;max-width:500px;aspect-ratio:4/3;background:#1a1a1a;border-radius:12px;overflow:hidden;display:flex;align-items:center;justify-content:center;margin-bottom:12px;position:relative}
.stream img{width:100%;height:100%;object-fit:cover;display:block}
.stream svg{width:64px;height:64px;opacity:.3}
.controls{width:100%;max-width:500px;display:flex;flex-direction:column;gap:10px;flex:1;padding-bottom:12px}
.slider-group{background:#1a1a1a;border-radius:12px;padding:12px 16px 16px}
.slider-group label{display:flex;justify-content:space-between;font-size:14px;font-weight:600;margin-bottom:8px;text-transform:uppercase;letter-spacing:.5px}
.slider-group input{width:100%;height:40px;-webkit-appearance:none;appearance:none;background:transparent;cursor:pointer}
.slider-group input::-webkit-slider-runnable-track{height:6px;background:#333;border-radius:3px}
.slider-group input::-webkit-slider-thumb{-webkit-appearance:none;width:28px;height:28px;border-radius:50%;background:#fff;margin-top:-11px;box-shadow:0 2px 8px rgba(0,0,0,.5)}
.slider-group input[type=range]::-moz-range-track{height:6px;background:#333;border-radius:3px}
.slider-group input[type=range]::-moz-range-thumb{width:28px;height:28px;border-radius:50%;background:#fff;border:none}
.val{font-size:22px;font-weight:800}
#thrVal{color:#ff4444}
#pitVal{color:#44ff88}
#rolVal{color:#4488ff}
#yawVal{color:#ffdd44}
.status{text-align:center;font-size:13px;font-weight:600;color:#00ff88;padding:10px;background:#1a1a2e;border-radius:12px;width:100%;max-width:500px;letter-spacing:.5px}
.cam-offline{display:none;flex-direction:column;align-items:center;gap:8px;color:#555}
.cam-offline svg{width:48px;height:48px;fill:#555}
</style>
</head>
<body>
<h1>&#9992;&#65039; DRONE CONTROL v2</h1>
<div class="stream">
  <img src="/cam.jpg" id="stream" onerror="this.style.display='none';document.getElementById('camFallback').style.display='flex'">
  <div class="cam-offline" id="camFallback">
    <svg viewBox="0 0 24 24"><path d="M23 18V6c0-1.1-.9-2-2-2H3c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h18c1.1 0 2-.9 2-2zM8.5 12.5l2.5 3.01L14.5 11l4.5 6H5l3.5-4.5z"/></svg>
    <span>Cámara no disponible</span>
  </div>
</div>
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
</div>
<div class="status">&#9679; Conectado al dron</div>
<script>
(function(){var t=document.getElementById('thr'),p=document.getElementById('pit'),r=document.getElementById('rol'),y=document.getElementById('yaw'),tv=document.getElementById('thrVal'),pv=document.getElementById('pitVal'),rv=document.getElementById('rolVal'),yv=document.getElementById('yawVal'),img=document.getElementById('stream');function send(){tv.textContent=t.value;pv.textContent=p.value;rv.textContent=r.value;yv.textContent=y.value;fetch('/control?thr='+t.value+'&pit='+p.value+'&rol='+r.value+'&yaw='+y.value)}t.oninput=send;p.oninput=send;r.oninput=send;y.oninput=send;setInterval(function(){img.style.display='';document.getElementById('camFallback').style.display='none';img.src='/cam.jpg?'+Date.now()},200)})();
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


