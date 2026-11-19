#pragma once
// config.h — merged common header for the PlatformIO ESP32-DIV port.
// WiFi / BLE / IR / RFID / GPS / nRF24 / CC1101 / SD / TFT / 触摸 的公共 include 与声明。
// 已删除: PCF8574 物理按钮驱动、XPT2046 电阻触摸、Buzzer、StatusLed。

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <Preferences.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Update.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_system.h>
#include <esp_event.h>
#include <nvs_flash.h>

// Sub-GHz：CC1101 驱动 + RCSwitch 的 OOK 编解码。
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <RCSwitch.h>

// NimBLE BLE stack, via the BleCompat shim which exposes BLEDevice/BLEScan/... aliases.
#include "BleCompat.h"

#include "arduinoFFT.h"
#include "utils.h"
#include "shared.h"

using namespace std;

/* ───────────── Common externs ───────────── */
extern TFT_eSPI tft;
class PCF8574;
extern PCF8574 pcf;

/* ───────────── BLE namespaces ───────────── */
namespace BleSpoofer {
  void spooferSetup();
  void spooferLoop();
  void exit();
}
namespace SourApple {
  void sourappleSetup();
  void sourappleLoop();
  void exit();
}
namespace BleScan {
  void bleScanSetup();
  void bleScanLoop();
  void exit();
  // Background BLE scanner: runs periodically in its own task, caching results.
  void startBackgroundScanner();
  int  getLastCount();
}
namespace BleSniffer {
  void blesnifferSetup();
  void blesnifferLoop();
  void exit();
}

/* ───────────── WiFi namespaces ───────────── */
namespace PacketMonitor {
  void ptmSetup();
  void ptmLoop();
}
namespace BeaconSpammer {
  void beaconSpamSetup();
  void beaconSpamLoop();
}
namespace DeauthDetect {
  void deauthdetectSetup();
  void deauthdetectLoop();
}
namespace WifiScan {
  void wifiscanSetup();
  void wifiscanLoop();
  // Background WiFi scanner: runs when no feature is active, caching results.
  void startBackgroundScanner();
  int  getLastCount();
  bool wifiCacheValidForReuse();
  int  staWifiScanSync();
  bool loadApListFromWifiCache(wifi_ap_record_t** ap_list, int* network_count,
                               int (*compare_ap)(const void*, const void*));
}
namespace CaptivePortal {
  void cportalSetup();
  void cportalLoop();
}
namespace Deauther {
  void deautherSetup();
  void deautherLoop();
}
namespace ProbeRequestFlood {
  void probeRequestFloodSetup();
  void probeRequestFloodLoop();
}
namespace FirmwareUpdate {
  void updateSetup();
  void updateLoop();
}
