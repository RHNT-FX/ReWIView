#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// setting ssid dan pw pada wifi
const char* SSID     = "SSID NAME";
const char* PASSWORD = "SSID PASS";

// Threshold: variance RSSI di atas ini = terdeteksi ada orang
const float VARIANCE_THRESHOLD = 8.0;  // variance RSSI 
const int   SAMPLE_SIZE        = 20;   // jumlah sample RSSI per deteksi
const int   SCAN_INTERVAL_MS   = 150;  // scan tiap 150ms

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// buffer RSSI dan state
int   rssiBuffer[SAMPLE_SIZE];
int   bufferIndex  = 0;
bool  humanPresent = false;
float lastVariance = 0;
unsigned long lastScan = 0;
unsigned long lastBroadcast = 0; // throttle websocket updates
bool bufferFull = false;

// hitung variance dari buffer RSSI
float calculateVariance() {
  float sum = 0, mean = 0, variance = 0;
  for (int i = 0; i < SAMPLE_SIZE; i++) sum += rssiBuffer[i];
  mean = sum / SAMPLE_SIZE;
  for (int i = 0; i < SAMPLE_SIZE; i++) {
    float diff = rssiBuffer[i] - mean;
    variance += diff * diff;
  }
  return variance / SAMPLE_SIZE;
}

// kirim data ke semua client WebSocket
void broadcastStatus() {
  JsonDocument doc;
  doc["present"]  = humanPresent;
  doc["variance"] = round(lastVariance * 10) / 10.0;
  doc["rssi"]     = rssiBuffer[(bufferIndex - 1 + SAMPLE_SIZE) % SAMPLE_SIZE];
  doc["time"]     = millis();
  String json;
  serializeJson(doc, json);
  ws.textAll(json);
}

// HTML dashboard (disimpan di flash, served langsung dari ESP32)
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>SmartHome Presence</title>
<style>
  *{margin:0;padding:0;box-sizing:border-box}
  body{font-family:'Segoe UI',sans-serif;background:#0f172a;color:#e2e8f0;min-height:100vh;padding:24px}
  h1{font-size:1.4rem;font-weight:600;color:#38bdf8;margin-bottom:4px}
  .sub{font-size:.8rem;color:#64748b;margin-bottom:24px}
  .status-card{background:#1e293b;border-radius:16px;padding:32px;text-align:center;margin-bottom:16px;border:2px solid #334155;transition:border-color .4s}
  .status-card.present{border-color:#22c55e}
  .status-card.absent{border-color:#ef4444}
  .dot{width:72px;height:72px;border-radius:50%;margin:0 auto 16px;transition:background .4s,box-shadow .4s}
  .dot.present{background:#22c55e;box-shadow:0 0 30px #22c55e88}
  .dot.absent{background:#ef4444;box-shadow:0 0 30px #ef444488}
  .status-text{font-size:1.6rem;font-weight:700;margin-bottom:8px}
  .present .status-text{color:#22c55e}
  .absent .status-text{color:#ef4444}
  .metrics{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px}
  .metric{background:#1e293b;border-radius:12px;padding:16px;border:1px solid #334155}
  .metric-label{font-size:.75rem;color:#64748b;margin-bottom:4px}
  .metric-value{font-size:1.4rem;font-weight:600;color:#e2e8f0}
  canvas{background:#1e293b;border-radius:12px;border:1px solid #334155;display:block;width:100%}
  .conn{font-size:.75rem;padding:4px 10px;border-radius:20px;display:inline-block;margin-bottom:12px}
  .conn.ok{background:#16451d;color:#22c55e}
  .conn.err{background:#450b0b;color:#ef4444}
</style>
</head>
<body>
<h1>ReWIVew</h1>
<p class="sub">ESP32 WiFi Sensor &mdash; Real-time</p>
<span class="conn err" id="conn">Menghubungkan...</span>

<div class="status-card absent" id="card">
  <div class="dot absent" id="dot"></div>
  <div class="status-text" id="statusText">Tidak Ada</div>
  <div style="font-size:.85rem;color:#64748b" id="statusSub">Ruangan kosong</div>
</div>

<div class="metrics">
  <div class="metric">
    <div class="metric-label">RSSI (dBm)</div>
    <div class="metric-value" id="rssiVal">--</div>
  </div>
  <div class="metric">
    <div class="metric-label">Variance</div>
    <div class="metric-value" id="varVal">--</div>
  </div>
</div>

<canvas id="chart" height="120"></canvas>

<script>
const MAX_POINTS = 60;
const rssiHistory = [];
const canvas = document.getElementById('chart');
const ctx = canvas.getContext('2d');

function drawChart() {
  canvas.width = canvas.offsetWidth;
  canvas.height = 120;
  ctx.clearRect(0,0,canvas.width,canvas.height);
  if (rssiHistory.length < 2) return;
  const w = canvas.width, h = canvas.height;
  const min = Math.min(...rssiHistory)-5, max = Math.max(...rssiHistory)+5;
  ctx.beginPath();
  ctx.strokeStyle = '#38bdf8';
  ctx.lineWidth = 2;
  rssiHistory.forEach((v,i) => {
    const x = (i/(MAX_POINTS-1))*w;
    const y = h - ((v-min)/(max-min))*h*0.8 - h*0.1;
    i===0 ? ctx.moveTo(x,y) : ctx.lineTo(x,y);
  });
  ctx.stroke();
  ctx.fillStyle = '#64748b';
  ctx.font = '10px Segoe UI';
  ctx.fillText(max+' dBm', 4, 14);
  ctx.fillText(min+' dBm', 4, h-4);
}

const ws = new WebSocket('ws://' + location.hostname + '/ws');
ws.onopen = () => { document.getElementById('conn').className='conn ok'; document.getElementById('conn').textContent='Terhubung'; };
ws.onclose = () => { document.getElementById('conn').className='conn err'; document.getElementById('conn').textContent='Terputus'; };
ws.onmessage = e => {
  const d = JSON.parse(e.data);
  const present = d.present;
  document.getElementById('card').className = 'status-card ' + (present?'present':'absent');
  document.getElementById('dot').className = 'dot ' + (present?'present':'absent');
  document.getElementById('statusText').textContent = present ? 'Ada Orang' : 'Tidak Ada';
  document.getElementById('statusSub').textContent = present ? 'Gerakan terdeteksi!' : 'Ruangan kosong';
  document.getElementById('rssiVal').textContent = d.rssi + ' dBm';
  document.getElementById('varVal').textContent = d.variance.toFixed(1);
  rssiHistory.push(d.rssi);
  if (rssiHistory.length > MAX_POINTS) rssiHistory.shift();
  drawChart();
};
window.addEventListener('resize', drawChart);
</script>
</body></html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  delay(3000);

  WiFiManager wm;
  // Jika gagal terhubung, ESP32 akan membuat AP "SmartHome-Setup"
  bool res = wm.autoConnect("SmartHome-Setup");
  if(!res) {
      Serial.println("Gagal terhubung dan kehabisan waktu!");
      // Restart jika gagal
      ESP.restart();
  }
  else {
      Serial.println("\nTerhubung! IP: " + WiFi.localIP().toString());
  }

  // inisialisasi buffer RSSI
  for (int i = 0; i < SAMPLE_SIZE; i++) rssiBuffer[i] = WiFi.RSSI();

  // webSocket handler
  ws.onEvent([](AsyncWebSocket* s, AsyncWebSocketClient* c,
                AwsEventType type, void*, uint8_t*, size_t) {
    if (type == WS_EVT_CONNECT) Serial.println("Client WS terhubung");
  });
  server.addHandler(&ws);

  // serve halaman HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send_P(200, "text/html", HTML_PAGE);
  });

  // REST API endpoint
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["present"]  = humanPresent;
    doc["variance"] = lastVariance;
    doc["rssi"]     = rssiBuffer[(bufferIndex-1+SAMPLE_SIZE)%SAMPLE_SIZE];
    String json; serializeJson(doc, json);
    req->send(200, "application/json", json);
  });

  server.begin();
  Serial.println("Server jalan di http://" + WiFi.localIP().toString());
}

void loop() {
  if (millis() - lastScan >= SCAN_INTERVAL_MS) {
    lastScan = millis();
    rssiBuffer[bufferIndex] = WiFi.RSSI();
    bufferIndex = (bufferIndex + 1) % SAMPLE_SIZE;

    if (bufferIndex == 0) {
      bufferFull = true;
    }

    if (bufferFull) {   // jika buffer sudah penuh sekali, gunakan algoritma sliding window
      lastVariance = calculateVariance();
      bool detected = (lastVariance > VARIANCE_THRESHOLD);
      bool statusChanged = false;

      if (detected != humanPresent) {
        humanPresent = detected;
        statusChanged = true;
        Serial.println(humanPresent ? ">>> ADA ORANG" : ">>> RUANGAN KOSONG");
      }

      // Update setiap 450ms atau saat status berubah, untuk menghemat resource ESP32 & mengurangi traffic WebSocket
      if (statusChanged || millis() - lastBroadcast >= 450) {
        broadcastStatus();
        lastBroadcast = millis();
      }
    }
  }
  ws.cleanupClients();
}
