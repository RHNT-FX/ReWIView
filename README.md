# 📡 ESP32 SmartHome Presence Detector (Device-Free WiFi Sensing)

Proyek ini adalah sistem pendeteksi keberadaan manusia (*Human Presence Detection*) berbasis **ESP32** yang bekerja secara **pasif dan *device-free***. Sistem ini tidak memerlukan sensor gerak konvensional (seperti PIR) atau kamera, melainkan memanfaatkan fluktuasi sinyal WiFi (*Received Signal Strength Indicator* / RSSI) untuk mendeteksi pergerakan di dalam ruangan.

Dilengkapi dengan *dashboard* antarmuka web yang modern, responsif, dan *real-time* menggunakan WebSocket, semua langsung di-*serve* dari memori (flash) ESP32!

---

## ✨ Fitur Utama
- **Device-Free Sensing:** Mendeteksi manusia hanya dari pantulan/gangguan gelombang WiFi pada tubuh manusia.
- **Real-time Web Dashboard:** UI/UX modern beraksen *dark mode* dengan animasi status.
- **Live Chart:** Grafik fluktuasi RSSI yang *update* secara *real-time* via WebSocket.
- **REST API Endpoint:** Tersedia endpoint `/api/status` berformat JSON untuk integrasi ke sistem Smart Home lain (misal: Home Assistant).
- **Algoritma Variance Thresholding:** Meminimalisir *false-alarm* dengan menghitung varians dari sampel RSSI di dalam *buffer*.

---

## 🛠️ Persyaratan (Prerequisites)

### Perangkat Keras (Hardware)
- Board ESP32 (mendukung ESP32 standar maupun ESP32-C3)
- Koneksi ke WiFi Router (2.4 GHz)

### Perangkat Lunak & Library (Software)
Pastikan kamu sudah menginstal library berikut di Arduino IDE / PlatformIO:
- `WiFi.h` (Bawaan core ESP32)
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) (Untuk server web & WebSocket)
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) (Dependensi untuk ESPAsyncWebServer)
- [ArduinoJson](https://arduinojson.org/) (Untuk *parsing* dan *serialize* data JSON)

---

## 🚀 Cara Penggunaan

1. *Clone* repositori ini ke komputer kamu.
2. Buka *source code* (`.ino` atau `main.cpp`).
3. Ubah konfigurasi WiFi pada baris berikut sesuai dengan jaringan kamu:
   ```cpp
   const char* SSID     = "NamaWiFiKamu";
   const char* PASSWORD = "PasswordWiFi";
   
4. Upload kode ke board ESP32 kamu.
5. Buka Serial Monitor (baud rate: 115200) untuk melihat IP Address yang didapatkan oleh ESP32.
6. Buka IP Address tersebut di browser (misal: http://192.168.1.15).
