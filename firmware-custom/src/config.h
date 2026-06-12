// =============================================================================
// Configuration — à éditer AVANT de flasher
// =============================================================================
#pragma once

// ----- Wi-Fi -----
#define WIFI_SSID       "CHANGE_ME_WIFI_SSID"
#define WIFI_PASSWORD   "CHANGE_ME_WIFI_PASSWORD"

// Optionnel : IP statique. À utiliser seulement si le SSID est bien sur le
// même LAN que Jeedom/MQTT. Si le DHCP donne 192.168.28.x, corriger d'abord
// le réseau Wi-Fi/VLAN côté routeur ou point d'accès.
#define WIFI_USE_STATIC_IP 1
#define WIFI_STATIC_IP     IPAddress(192, 168, 10, 21)
#define WIFI_GATEWAY       IPAddress(192, 168, 10, 1)
#define WIFI_SUBNET        IPAddress(255, 255, 255, 0)
#define WIFI_DNS1          IPAddress(192, 168, 10, 1)
#define WIFI_DNS2          IPAddress(1, 1, 1, 1)

// ----- Identité réseau -----
#define DEVICE_HOSTNAME "duco-vmc"
#define WIFI_HOSTNAME   "ESP32-Duco"
#define OTA_PASSWORD    "CHANGE_ME_OTA_PASSWORD"   // mot de passe OTA

// ----- MQTT -----
#define MQTT_HOST       "192.168.10.10"  // IP du broker (Jeedom jMQTT ou Mosquitto)
#define MQTT_PORT       1883
#define MQTT_USER       ""              // laisser vide si le broker MQTT n'a pas d'auth
#define MQTT_PASSWORD   ""              // laisser vide si le broker MQTT n'a pas d'auth
#define MQTT_BASE_TOPIC "duco-vmc"      // topics : duco-vmc/state/..., duco-vmc/set/...

// ----- UART vers la DucoBox -----
// UART2 sur GPIO16 (RX) / GPIO17 (TX), 57600 8N1
#define DUCO_RX_PIN     16
#define DUCO_TX_PIN     17
#define DUCO_BAUD       57600
#define DUCO_POLL_INTERVAL_MS 30000UL

// ----- Debug communication -----
// Laisser DUCO_DEBUG_FRAMES à 0 en usage normal : le bus parle beaucoup.
#define DUCO_DEBUG_PROTOCOL 1   // logs OK/timeout ACK/reply
#define DUCO_DEBUG_FRAMES   0   // dump hex des trames TX/RX valides
