// =============================================================================
// Firmware custom Duco VMC - ESP32 sans ESPHome
//   - Wi-Fi  (STA + mDNS)
//   - MQTT   (PubSubClient)
//   - Web UI (ESPAsyncWebServer, HTML embarque)
//   - OTA    (ArduinoOTA)
//   - Duco   (UART2 + protocole kokx)
// =============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <time.h>

#include "config.h"
#include "duco.h"
#include "web_ui.h"

// ----- Globaux -----
HardwareSerial   DucoSerial(2);            // UART2
Duco             duco;
WiFiClient       wifiClient;
PubSubClient     mqtt(wifiClient);
AsyncWebServer   web(80);
Preferences      prefs;

uint32_t lastPollMs       = 0;
uint32_t lastPubMs        = 0;
uint32_t lastMqttTryMs    = 0;
bool     pollNowRequested = false;
SemaphoreHandle_t ducoMutex = nullptr;

// Limites Duco cote ecriture.
static constexpr uint16_t DUCO_MAX_WRITES_PER_DAY = 200;
static constexpr uint32_t DUCO_MIN_WRITE_INTERVAL_MS = 2000UL;
static constexpr uint32_t DUCO_FALLBACK_DAY_KEY = 1;

uint16_t writesToday = 0;
uint32_t writeDayKey = 0;
uint32_t lastDucoWriteMs = 0;
bool     writeClockSynced = false;

// Regulation auto free-cooling.
static constexpr float WEATHER_LAT = 48.9567f;
static constexpr float WEATHER_LON = 4.3631f;
static constexpr uint32_t WEATHER_REFRESH_MS = 6UL * 60UL * 60UL * 1000UL;
static constexpr uint32_t AUTO_MIN_WRITE_INTERVAL_MS = 10UL * 60UL * 1000UL;
static constexpr uint32_t MANUAL_OVERRIDE_MS = 2UL * 60UL * 60UL * 1000UL;
static constexpr float TOMORROW_HOT_MARGIN_C = 2.0f;
static constexpr float NEED_COOLING_MARGIN_C = 1.0f;
static constexpr float GOOD_DELTA_C = 1.5f;
static constexpr float STRONG_DELTA_C = 3.0f;
static constexpr float MIN_INTERIOR_COOLING_C = 16.0f;
static constexpr float MIN_OUTSIDE_COOLING_C = 8.0f;

bool     autoRegulationEnabled = false;
bool     autoRegulationActive = false;
String   autoRegulationAction = "disabled";
String   autoRegulationReason = "Regulation desactivee";
uint32_t autoManualOverrideUntilMs = 0;
uint32_t lastAutoWriteDecisionMs = 0;
uint32_t lastWeatherFetchMs = 0;
bool     forecastOk = false;
float    forecastTomorrowMax = NAN;
float    forecastTomorrowMin = NAN;
String   forecastError = "";

struct MqttCacheEntry {
    const char* topic;
    String payload;
    bool valid;
};

MqttCacheEntry mqttCache[32];

bool takeDucoBus(uint32_t timeoutMs) {
    if (!ducoMutex) return true;
    return xSemaphoreTake(ducoMutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void releaseDucoBus() {
    if (ducoMutex) xSemaphoreGive(ducoMutex);
}

MqttCacheEntry* mqttCacheFind(const char* topic) {
    for (auto& entry : mqttCache) {
        if (entry.valid && strcmp(entry.topic, topic) == 0) return &entry;
    }
    return nullptr;
}

MqttCacheEntry* mqttCacheSlot(const char* topic) {
    if (MqttCacheEntry* entry = mqttCacheFind(topic)) return entry;
    for (auto& entry : mqttCache) {
        if (!entry.valid) {
            entry.topic = topic;
            entry.payload = "";
            entry.valid = true;
            return &entry;
        }
    }
    return nullptr;
}

bool mqttPublishIfChanged(const char* topic, const String& payload,
                          bool retained = true, bool force = false) {
    MqttCacheEntry* entry = mqttCacheSlot(topic);
    if (entry && !force && entry->payload == payload) return true;

    bool ok = mqtt.publish(topic, payload.c_str(), retained);
    if (ok && entry) entry->payload = payload;
    return ok;
}

String mqttFloatPayload(float value) {
    if (isnan(value)) return "";
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", value);
    return String(buf);
}

bool isValidVentMode(uint8_t mode) {
    switch (mode) {
        case DUCO_MODE_AUTO:
        case DUCO_MODE_MANUAL1:
        case DUCO_MODE_MANUAL2:
        case DUCO_MODE_MANUAL3:
        case DUCO_MODE_NOT_AT_HOME:
        case DUCO_MODE_PERMANENT1:
        case DUCO_MODE_PERMANENT2:
        case DUCO_MODE_PERMANENT3:
        case DUCO_MODE_MANUAL1_X2:
        case DUCO_MODE_MANUAL2_X2:
        case DUCO_MODE_MANUAL3_X2:
        case DUCO_MODE_MANUAL1_X3:
        case DUCO_MODE_MANUAL2_X3:
        case DUCO_MODE_MANUAL3_X3:
            return true;
        default:
            return false;
    }
}

void jsonSetFloat(JsonDocument& doc, const char* key, float value) {
    if (isnan(value)) {
        doc[key] = nullptr;
    } else {
        doc[key] = value;
    }
}

uint32_t currentWriteDayKey() {
    time_t now = time(nullptr);
    if (now > 1700000000) {
        struct tm local;
        localtime_r(&now, &local);
        writeClockSynced = true;
        return (uint32_t)((local.tm_year + 1900) * 1000 + local.tm_yday);
    }

    writeClockSynced = false;
    return DUCO_FALLBACK_DAY_KEY;
}

void persistWriteCounter() {
    prefs.putUInt("writeDay", writeDayKey);
    prefs.putUShort("writes", writesToday);
}

void refreshWriteWindow() {
    uint32_t today = currentWriteDayKey();
    if (writeDayKey == today) return;

    writeDayKey = today;
    uint32_t storedDay = prefs.getUInt("writeDay", 0);
    writesToday = (storedDay == writeDayKey) ? prefs.getUShort("writes", 0) : 0;
    persistWriteCounter();
}

void initWriteLimiter() {
    prefs.begin("duco", false);
    writeDayKey = currentWriteDayKey();
    uint32_t storedDay = prefs.getUInt("writeDay", 0);
    writesToday = (storedDay == writeDayKey) ? prefs.getUShort("writes", 0) : 0;
    autoRegulationEnabled = prefs.getBool("autoReg", false);
    persistWriteCounter();
    Serial.printf("[DUCO] write guard day=%lu used=%u/%u clock=%s\n",
                  (unsigned long)writeDayKey, writesToday, DUCO_MAX_WRITES_PER_DAY,
                  writeClockSynced ? "synced" : "fallback");
    Serial.printf("[AUTO] regulation %s\n", autoRegulationEnabled ? "enabled" : "disabled");
}

bool reserveDucoWrite(String& reason) {
    refreshWriteWindow();

    uint32_t now = millis();
    if (lastDucoWriteMs != 0) {
        uint32_t elapsed = now - lastDucoWriteMs;
        if (elapsed < DUCO_MIN_WRITE_INTERVAL_MS) {
            uint32_t waitMs = DUCO_MIN_WRITE_INTERVAL_MS - elapsed;
            reason = "write interval limit, wait " + String(waitMs) + " ms";
            return false;
        }
    }

    if (writesToday >= DUCO_MAX_WRITES_PER_DAY) {
        reason = "daily write limit reached";
        return false;
    }

    writesToday++;
    lastDucoWriteMs = now;
    persistWriteCounter();
    return true;
}

uint16_t writesRemaining() {
    refreshWriteWindow();
    return writesToday >= DUCO_MAX_WRITES_PER_DAY ? 0 : DUCO_MAX_WRITES_PER_DAY - writesToday;
}

void addWriteStats(JsonDocument& doc) {
    refreshWriteWindow();
    doc["writes_today"] = writesToday;
    doc["writes_remaining"] = writesRemaining();
    doc["write_limit_per_day"] = DUCO_MAX_WRITES_PER_DAY;
    doc["write_min_interval_ms"] = DUCO_MIN_WRITE_INTERVAL_MS;
    doc["write_clock_synced"] = writeClockSynced;
    doc["last_write_ms"] = lastDucoWriteMs;
}

bool manualOverrideActive() {
    return autoManualOverrideUntilMs != 0 &&
           (int32_t)(millis() - autoManualOverrideUntilMs) < 0;
}

uint32_t manualOverrideRemainingMs() {
    if (!manualOverrideActive()) return 0;
    return autoManualOverrideUntilMs - millis();
}

void startManualOverride() {
    autoManualOverrideUntilMs = millis() + MANUAL_OVERRIDE_MS;
    Serial.println("[AUTO] manual override hold for 2h");
}

void clearManualOverride() {
    autoManualOverrideUntilMs = 0;
    Serial.println("[AUTO] manual override cleared");
}

void setAutoRegulationEnabled(bool enabled) {
    autoRegulationEnabled = enabled;
    prefs.putBool("autoReg", enabled);
    if (enabled) clearManualOverride();
    autoRegulationAction = enabled ? "waiting" : "disabled";
    autoRegulationReason = enabled ? "Regulation activee" : "Regulation desactivee";
    autoRegulationActive = false;
    Serial.printf("[AUTO] regulation %s\n", enabled ? "enabled" : "disabled");
}

bool fetchWeatherForecast() {
    if (WiFi.status() != WL_CONNECTED) return false;

    lastWeatherFetchMs = millis();
    forecastError = "";
    HTTPClient http;
    WiFiClient weatherClient;
    String url = "http://api.open-meteo.com/v1/forecast"
                 "?latitude=" + String(WEATHER_LAT, 4) +
                 "&longitude=" + String(WEATHER_LON, 4) +
                 "&daily=temperature_2m_max,temperature_2m_min"
                 "&forecast_days=2&timezone=Europe%2FParis";

    http.setTimeout(7000);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    // Open-Meteo peut repondre en chunked HTTP/1.1 ; HTTP/1.0 garde un corps JSON direct.
    http.useHTTP10(true);
    if (!http.begin(weatherClient, url)) {
        forecastOk = false;
        forecastError = "HTTP begin";
        Serial.println("[WEATHER] begin FAIL");
        return false;
    }
    http.addHeader("User-Agent", "duco-vmc/1.0");

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        forecastOk = false;
        forecastError = "HTTP " + String(code);
        Serial.printf("[WEATHER] GET FAIL code=%d\n", code);
        http.end();
        return false;
    }

    String body = http.getString();
    http.end();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        forecastOk = false;
        forecastError = String("JSON ") + err.c_str();
        Serial.printf("[WEATHER] JSON FAIL %s\n", err.c_str());
        return false;
    }

    JsonArray maxs = doc["daily"]["temperature_2m_max"].as<JsonArray>();
    JsonArray mins = doc["daily"]["temperature_2m_min"].as<JsonArray>();
    if (maxs.size() < 2 || mins.size() < 2) {
        forecastOk = false;
        forecastError = "missing tomorrow";
        Serial.println("[WEATHER] missing tomorrow forecast");
        return false;
    }

    forecastTomorrowMax = maxs[1].as<float>();
    forecastTomorrowMin = mins[1].as<float>();
    forecastOk = true;
    forecastError = "";
    Serial.printf("[WEATHER] tomorrow min=%.1f max=%.1f\n",
                  forecastTomorrowMin, forecastTomorrowMax);
    return true;
}

void maybeFetchWeatherForecast(bool force = false) {
    if (WiFi.status() != WL_CONNECTED) return;
    if (!force && lastWeatherFetchMs != 0 &&
        millis() - lastWeatherFetchMs < WEATHER_REFRESH_MS) {
        return;
    }
    fetchWeatherForecast();
}

bool autoInputsReady() {
    return !isnan(duco.tempOda) && !isnan(duco.tempEta) &&
           !isnan(duco.comfortTemp) && duco.currentMode != 0xFF &&
           duco.bypassMode <= DUCO_BYPASS_MODE_OPEN;
}

bool autoReserveWrite(String& reason) {
    if (reserveDucoWrite(reason)) return true;
    autoRegulationReason = reason;
    Serial.printf("[AUTO] write blocked: %s\n", reason.c_str());
    return false;
}

bool autoWriteBypassIfNeeded(uint8_t target) {
    if (duco.bypassMode == target) return true;

    String reason;
    if (!autoReserveWrite(reason)) return false;
    bool ok = duco.writeBypassMode(target);
    if (ok) {
        delay(800);
        duco.readBypassMode();
        duco.readBypass();
        Serial.printf("[AUTO] bypass mode -> %u\n", target);
    } else {
        Serial.printf("[AUTO] bypass mode write FAIL target=%u\n", target);
    }
    return ok;
}

bool autoWriteVentModeIfNeeded(uint8_t target) {
    if (duco.currentMode == target) return true;

    String reason;
    if (!autoReserveWrite(reason)) return false;
    bool ok = duco.writeMode(target);
    if (ok) {
        Serial.printf("[AUTO] vent mode -> %u\n", target);
    } else {
        Serial.printf("[AUTO] vent mode write FAIL target=%u\n", target);
    }
    return ok;
}

void evaluateAutoRegulation(bool allowWrites) {
    uint8_t targetBypass = DUCO_BYPASS_MODE_AUTO;
    uint8_t targetMode = DUCO_MODE_AUTO;
    bool shouldControl = false;

    if (!autoRegulationEnabled) {
        autoRegulationActive = false;
        autoRegulationAction = "disabled";
        autoRegulationReason = "Regulation desactivee";
        return;
    }

    if (!autoInputsReady()) {
        autoRegulationActive = false;
        autoRegulationAction = "waiting";
        autoRegulationReason = "Donnees Duco incompletes";
        return;
    }

    float delta = duco.tempEta - duco.tempOda;
    bool tomorrowHot = forecastOk &&
        forecastTomorrowMax >= duco.comfortTemp + TOMORROW_HOT_MARGIN_C;
    bool needCooling = tomorrowHot ||
        duco.tempEta >= duco.comfortTemp + NEED_COOLING_MARGIN_C;
    bool goodDelta = delta >= GOOD_DELTA_C;
    bool strongDelta = delta >= STRONG_DELTA_C;
    bool coolingSafe = duco.tempEta > MIN_INTERIOR_COOLING_C &&
        duco.tempOda > MIN_OUTSIDE_COOLING_C;

    if ((tomorrowHot || (!forecastOk && needCooling)) && goodDelta && coolingSafe) {
        targetBypass = DUCO_BYPASS_MODE_OPEN;
        targetMode = strongDelta ? DUCO_MODE_PERMANENT3 : DUCO_MODE_PERMANENT2;
        shouldControl = true;
        autoRegulationAction = tomorrowHot ? "cooling_forecast" : "cooling_local";
        autoRegulationReason = String(tomorrowHot ? "Demain chaud" : "Chaud local") +
            ", delta " + String(delta, 1) + "C";
    } else if (needCooling && duco.tempOda >= duco.tempEta - 0.5f) {
        targetBypass = DUCO_BYPASS_MODE_SHUT;
        targetMode = DUCO_MODE_AUTO;
        shouldControl = true;
        autoRegulationAction = "heat_protection";
        autoRegulationReason = "Exterieur trop chaud";
    } else if (!tomorrowHot && duco.tempEta <= duco.comfortTemp + 0.5f &&
               duco.tempOda <= duco.comfortTemp - 2.0f) {
        targetBypass = DUCO_BYPASS_MODE_SHUT;
        targetMode = DUCO_MODE_AUTO;
        shouldControl = true;
        autoRegulationAction = "winter_protection";
        autoRegulationReason = "Protection chaleur interieure";
    } else {
        targetBypass = DUCO_BYPASS_MODE_AUTO;
        targetMode = DUCO_MODE_AUTO;
        autoRegulationAction = "idle";
        autoRegulationReason = "Aucune action utile";
    }

    autoRegulationActive = shouldControl;

    if (manualOverrideActive()) {
        String desiredReason = autoRegulationReason;
        autoRegulationActive = false;
        autoRegulationAction = "manual_hold";
        autoRegulationReason = "Pause manuelle, " + desiredReason;
        return;
    }

    if (!allowWrites) return;

    bool bypassDiff = duco.bypassMode != targetBypass;
    bool modeDiff = duco.currentMode != targetMode;
    if (!bypassDiff && !modeDiff) return;

    if (lastAutoWriteDecisionMs != 0 &&
        millis() - lastAutoWriteDecisionMs < AUTO_MIN_WRITE_INTERVAL_MS) {
        autoRegulationReason += ", attente cooldown";
        return;
    }

    Serial.printf("[AUTO] action=%s reason=%s target_bypass=%u target_mode=%u\n",
                  autoRegulationAction.c_str(), autoRegulationReason.c_str(),
                  targetBypass, targetMode);

    bool wrote = false;
    bool ok = true;
    if (bypassDiff) {
        ok = autoWriteBypassIfNeeded(targetBypass);
        wrote = wrote || ok;
        if (ok && modeDiff) delay(DUCO_MIN_WRITE_INTERVAL_MS);
    }
    if (ok && modeDiff) {
        ok = autoWriteVentModeIfNeeded(targetMode);
        wrote = wrote || ok;
    }

    if (wrote) {
        lastAutoWriteDecisionMs = millis();
        pollNowRequested = true;
    }
}

void addAutoRegulationState(JsonDocument& doc) {
    evaluateAutoRegulation(false);
    doc["auto_regulation_enabled"] = autoRegulationEnabled;
    doc["auto_regulation_active"] = autoRegulationActive;
    doc["auto_regulation_action"] = autoRegulationAction;
    doc["auto_regulation_reason"] = autoRegulationReason;
    jsonSetFloat(doc, "forecast_tomorrow_max", forecastTomorrowMax);
    jsonSetFloat(doc, "forecast_tomorrow_min", forecastTomorrowMin);
    doc["forecast_ok"] = forecastOk;
    doc["forecast_error"] = forecastError;
    doc["manual_override_active"] = manualOverrideActive();
    doc["manual_override_remaining_ms"] = manualOverrideRemainingMs();
}

void pollDuco() {
    Serial.println("[DUCO] poll");

    bool modeOk = duco.readMode();
    if (modeOk) {
        Serial.printf("[DUCO] read mode OK mode=%u\n", duco.currentMode);
    } else {
        Serial.println("[DUCO] read mode FAIL");
    }
    delay(80);

    bool comfortOk = duco.readComfortTemp();
    if (comfortOk) {
        Serial.printf("[DUCO] read comfort OK temp=%.1f C\n", duco.comfortTemp);
    } else {
        Serial.println("[DUCO] read comfort FAIL");
    }
    delay(80);

    bool tempsOk = duco.readBoxTemps();
    if (tempsOk) {
        Serial.printf("[DUCO] read temps OK oda=%.1f sup=%.1f eta=%.1f eha=%.1f C\n",
                      duco.tempOda, duco.tempSup, duco.tempEta, duco.tempEha);
    } else {
        Serial.println("[DUCO] read temps FAIL");
    }
    delay(80);

    bool bypassOk = duco.readBypass();
    if (bypassOk) {
        Serial.printf("[DUCO] read bypass OK state=%s position=%u%% flow=%u%%\n",
                      duco.bypassOpen ? "open" : "closed",
                      duco.bypassPosition, duco.flowLevel);
    } else {
        Serial.println("[DUCO] read bypass FAIL");
    }
    delay(80);

    bool bypassModeOk = duco.readBypassMode();
    if (bypassModeOk) {
        Serial.printf("[DUCO] read bypass mode OK mode=%u\n", duco.bypassMode);
    } else {
        Serial.println("[DUCO] read bypass mode FAIL");
    }
    delay(80);

    bool filterOk = duco.readFilterRemaining();
    if (filterOk) {
        Serial.printf("[DUCO] read filter OK remaining=%d d\n", duco.filterRemainingDays);
    } else {
        Serial.println("[DUCO] read filter FAIL");
    }

    if (duco.serialNumber.length() == 0) {
        delay(80);
        bool serialOk = duco.readSerialNumber();
        if (serialOk) {
            Serial.printf("[DUCO] read serial OK serial=%s\n", duco.serialNumber.c_str());
        } else {
            Serial.println("[DUCO] read serial FAIL");
        }
    }
}

// ===========================================================================
// MQTT
// ===========================================================================
String autoActionMqttFr() {
    if (autoRegulationAction == "disabled") return "desactivee";
    if (autoRegulationAction == "waiting") return "attente";
    if (autoRegulationAction == "cooling_forecast") return "refroidissement_prevision";
    if (autoRegulationAction == "cooling_local") return "refroidissement_local";
    if (autoRegulationAction == "heat_protection") return "protection_chaleur";
    if (autoRegulationAction == "winter_protection") return "protection_hiver";
    if (autoRegulationAction == "manual_hold") return "pause_manuelle";
    if (autoRegulationAction == "idle") return "repos";
    return autoRegulationAction;
}

void mqttPublishState() {
    JsonDocument doc;
    evaluateAutoRegulation(false);

    doc["en_ligne"]  = duco.online;
    doc["mode"]    = duco.currentMode;
    jsonSetFloat(doc, "consigne", duco.comfortTemp);
    doc["bypass"]  = duco.bypassOpen;
    doc["ouverture_bypass"] = duco.bypassPosition;
    doc["mode_bypass"] = duco.bypassMode;
    doc["debit"]    = duco.flowLevel;
    if (duco.modeTimeRemainingSec >= 0) doc["temps_mode_restant_s"] = duco.modeTimeRemainingSec;
    else doc["temps_mode_restant_s"] = nullptr;
    if (duco.filterRemainingDays >= 0) doc["filtre_restant_jours"] = duco.filterRemainingDays;
    else doc["filtre_restant_jours"] = nullptr;
    doc["numero_serie"] = duco.serialNumber;
    jsonSetFloat(doc, "exterieur", duco.tempOda);
    jsonSetFloat(doc, "air_soufflee", duco.tempSup);
    jsonSetFloat(doc, "air_extraite", duco.tempEta);
    jsonSetFloat(doc, "air_rejetee", duco.tempEha);
    refreshWriteWindow();
    doc["ecritures_jour"] = writesToday;
    doc["ecritures_restantes"] = writesRemaining();
    doc["limite_ecritures_jour"] = DUCO_MAX_WRITES_PER_DAY;
    doc["intervalle_min_ecriture_ms"] = DUCO_MIN_WRITE_INTERVAL_MS;
    doc["horloge_ecriture_synchro"] = writeClockSynced;
    doc["derniere_ecriture_ms"] = lastDucoWriteMs;
    doc["regulation_auto_activee"] = autoRegulationEnabled;
    doc["regulation_auto_en_cours"] = autoRegulationActive;
    doc["regulation_auto_action"] = autoActionMqttFr();
    doc["regulation_auto_raison"] = autoRegulationReason;
    jsonSetFloat(doc, "prevision_demain_max", forecastTomorrowMax);
    jsonSetFloat(doc, "prevision_demain_min", forecastTomorrowMin);
    doc["prevision_ok"] = forecastOk;
    doc["erreur_prevision"] = forecastError;
    doc["pause_manuelle_active"] = manualOverrideActive();

    String json;
    serializeJson(doc, json);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state", json, true);

    // Topics granulaires utiles pour Jeedom jMQTT.
    auto pubf = [&](const char* topic, float v) {
        if (isnan(v)) return;
        mqttPublishIfChanged(topic, mqttFloatPayload(v), true);
    };
    pubf(MQTT_BASE_TOPIC "/state/comfort", duco.comfortTemp);
    pubf(MQTT_BASE_TOPIC "/state/oda", duco.tempOda);
    pubf(MQTT_BASE_TOPIC "/state/sup", duco.tempSup);
    pubf(MQTT_BASE_TOPIC "/state/eta", duco.tempEta);
    pubf(MQTT_BASE_TOPIC "/state/eha", duco.tempEha);

    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/mode", String(duco.currentMode), true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/bypass", duco.bypassOpen ? "1" : "0", true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/bypass_position", String(duco.bypassPosition), true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/bypass_mode", String(duco.bypassMode), true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/mode_time_remaining_sec",
                         duco.modeTimeRemainingSec >= 0 ? String(duco.modeTimeRemainingSec) : "", true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/filter_remaining_days",
                         duco.filterRemainingDays >= 0 ? String(duco.filterRemainingDays) : "", true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/serial", duco.serialNumber, true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/writes_today", String(writesToday), true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/writes_remaining", String(writesRemaining()), true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/auto_regulation_enabled",
                         autoRegulationEnabled ? "1" : "0", true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/auto_regulation_active",
                         autoRegulationActive ? "1" : "0", true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/auto_regulation_action",
                         autoRegulationAction, true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/auto_regulation_reason",
                         autoRegulationReason, true);
    pubf(MQTT_BASE_TOPIC "/state/forecast_tomorrow_max", forecastTomorrowMax);
    pubf(MQTT_BASE_TOPIC "/state/forecast_tomorrow_min", forecastTomorrowMin);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/forecast_ok", forecastOk ? "1" : "0", true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/forecast_error", forecastError, true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/manual_override_active",
                         manualOverrideActive() ? "1" : "0", true);
    mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/online", duco.online ? "1" : "0", true);
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
    char body[64];
    size_t n = min((size_t)len, sizeof(body) - 1);
    memcpy(body, payload, n);
    body[n] = 0;
    String t(topic);
    String reason;

    if (t == MQTT_BASE_TOPIC "/set/mode") {
        int m = atoi(body);
        Serial.printf("[MQTT] set mode = %d\n", m);
        if (!isValidVentMode((uint8_t)m)) {
            Serial.println("[MQTT] reject mode: bad value");
        } else if (takeDucoBus(2500)) {
            if (reserveDucoWrite(reason)) {
                if (duco.writeMode((uint8_t)m)) startManualOverride();
            } else Serial.printf("[MQTT] reject write: %s\n", reason.c_str());
            releaseDucoBus();
        }
    } else if (t == MQTT_BASE_TOPIC "/set/comfort") {
        float c = atof(body);
        Serial.printf("[MQTT] set comfort = %.1f\n", c);
        if (c < 10.0f || c > 30.0f) {
            Serial.println("[MQTT] reject comfort: bad value");
        } else if (takeDucoBus(2500)) {
            if (reserveDucoWrite(reason)) {
                if (duco.writeComfortTemp(c)) startManualOverride();
            } else Serial.printf("[MQTT] reject write: %s\n", reason.c_str());
            releaseDucoBus();
        }
    } else if (t == MQTT_BASE_TOPIC "/set/bypass_mode") {
        int m = atoi(body);
        Serial.printf("[MQTT] set bypass mode = %d\n", m);
        if (m < DUCO_BYPASS_MODE_AUTO || m > DUCO_BYPASS_MODE_OPEN) {
            Serial.println("[MQTT] reject bypass mode: bad value");
        } else if (takeDucoBus(2500)) {
            if (reserveDucoWrite(reason)) {
                if (duco.writeBypassMode((uint8_t)m)) {
                    startManualOverride();
                    delay(800);
                    duco.readBypassMode();
                    duco.readBypass();
                    pollNowRequested = true;
                    lastPollMs = 0;
                }
            } else {
                Serial.printf("[MQTT] reject write: %s\n", reason.c_str());
            }
            releaseDucoBus();
        }
    } else if (t == MQTT_BASE_TOPIC "/set/auto_regulation") {
        bool enabled = atoi(body) != 0;
        setAutoRegulationEnabled(enabled);
    } else if (t == MQTT_BASE_TOPIC "/set/auto_hold_clear") {
        clearManualOverride();
    }

    mqttPublishState();
}

void mqttReconnect() {
    if (mqtt.connected()) return;
    if (millis() - lastMqttTryMs < 5000) return;
    lastMqttTryMs = millis();

    Serial.print("[MQTT] connect... ");
    bool ok;
    if (strlen(MQTT_USER) > 0) {
        ok = mqtt.connect(DEVICE_HOSTNAME, MQTT_USER, MQTT_PASSWORD,
                          MQTT_BASE_TOPIC "/state/online", 0, true, "0");
    } else {
        ok = mqtt.connect(DEVICE_HOSTNAME, MQTT_BASE_TOPIC "/state/online",
                          0, true, "0");
    }

    if (ok) {
        Serial.println("OK");
        mqtt.subscribe(MQTT_BASE_TOPIC "/set/mode");
        mqtt.subscribe(MQTT_BASE_TOPIC "/set/comfort");
        mqtt.subscribe(MQTT_BASE_TOPIC "/set/bypass_mode");
        mqtt.subscribe(MQTT_BASE_TOPIC "/set/auto_regulation");
        mqtt.subscribe(MQTT_BASE_TOPIC "/set/auto_hold_clear");
        mqttPublishIfChanged(MQTT_BASE_TOPIC "/state/online", "1", true, true);
    } else {
        Serial.printf("FAIL rc=%d\n", mqtt.state());
    }
}

// ===========================================================================
// HTTP - serveur web embarque
// ===========================================================================
bool checkAuth(AsyncWebServerRequest* req) {
    (void)req;
    return true;
}

void sendState(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["online"]  = duco.online;
    doc["mode"]    = duco.currentMode;
    jsonSetFloat(doc, "comfort", duco.comfortTemp);
    doc["bypass"]  = duco.bypassOpen;
    doc["bypass_position"] = duco.bypassPosition;
    doc["bypass_mode"] = duco.bypassMode;
    doc["flow"]    = duco.flowLevel;
    if (duco.modeTimeRemainingSec >= 0) doc["mode_time_remaining_sec"] = duco.modeTimeRemainingSec;
    else doc["mode_time_remaining_sec"] = nullptr;
    if (duco.filterRemainingDays >= 0) doc["filter_remaining_days"] = duco.filterRemainingDays;
    else doc["filter_remaining_days"] = nullptr;
    doc["serial"] = duco.serialNumber;
    jsonSetFloat(doc, "oda", duco.tempOda);
    jsonSetFloat(doc, "sup", duco.tempSup);
    jsonSetFloat(doc, "eta", duco.tempEta);
    jsonSetFloat(doc, "eha", duco.tempEha);
    addWriteStats(doc);
    addAutoRegulationState(doc);

    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
}

bool reserveHttpWrite(AsyncWebServerRequest* req) {
    String reason;
    if (reserveDucoWrite(reason)) return true;
    req->send(429, "text/plain", reason);
    return false;
}

void setupWeb() {
    web.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!checkAuth(req)) return;
        req->send(200, "text/html; charset=utf-8", INDEX_HTML);
    });

    web.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!checkAuth(req)) return;
        sendState(req);
    });

    web.on("/api/set_mode", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!checkAuth(req)) return;
        if (!req->hasParam("value")) { req->send(400, "text/plain", "missing value"); return; }
        int m = req->getParam("value")->value().toInt();
        if (!isValidVentMode((uint8_t)m)) { req->send(400, "text/plain", "bad value"); return; }
        if (!takeDucoBus(2500)) { req->send(503, "text/plain", "bus busy"); return; }
        if (!reserveHttpWrite(req)) { releaseDucoBus(); return; }
        bool ok = duco.writeMode((uint8_t)m);
        if (ok) startManualOverride();
        releaseDucoBus();
        pollNowRequested = true;
        req->send(ok ? 200 : 500, "text/plain", ok ? "OK" : "FAIL");
    });

    web.on("/api/set_comfort", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!checkAuth(req)) return;
        if (!req->hasParam("value")) { req->send(400, "text/plain", "missing value"); return; }
        float t = req->getParam("value")->value().toFloat();
        if (t < 10.0f || t > 30.0f) { req->send(400, "text/plain", "bad value"); return; }
        if (!takeDucoBus(2500)) { req->send(503, "text/plain", "bus busy"); return; }
        if (!reserveHttpWrite(req)) { releaseDucoBus(); return; }
        bool ok = duco.writeComfortTemp(t);
        if (ok) startManualOverride();
        releaseDucoBus();
        pollNowRequested = true;
        req->send(ok ? 200 : 500, "text/plain", ok ? "OK" : "FAIL");
    });

    web.on("/api/set_bypass_mode", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!checkAuth(req)) return;
        if (!req->hasParam("value")) { req->send(400, "text/plain", "missing value"); return; }
        int m = req->getParam("value")->value().toInt();
        if (m < DUCO_BYPASS_MODE_AUTO || m > DUCO_BYPASS_MODE_OPEN) {
            req->send(400, "text/plain", "bad value");
            return;
        }
        if (!takeDucoBus(2500)) { req->send(503, "text/plain", "bus busy"); return; }
        if (!reserveHttpWrite(req)) { releaseDucoBus(); return; }
        bool ok = duco.writeBypassMode((uint8_t)m);
        if (ok) {
            startManualOverride();
            delay(800);
            duco.readBypassMode();
            duco.readBypass();
        }
        releaseDucoBus();
        lastPollMs = 0;
        pollNowRequested = true;
        req->send(ok ? 200 : 500, "text/plain", ok ? "OK" : "FAIL");
    });

    web.on("/api/refresh_state", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!checkAuth(req)) return;
        pollNowRequested = true;
        req->send(200, "text/plain", "OK");
    });

    web.on("/api/set_auto_regulation", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!checkAuth(req)) return;
        if (!req->hasParam("enabled")) { req->send(400, "text/plain", "missing enabled"); return; }
        bool enabled = req->getParam("enabled")->value().toInt() != 0;
        setAutoRegulationEnabled(enabled);
        req->send(200, "text/plain", "OK");
    });

    web.on("/api/clear_auto_hold", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!checkAuth(req)) return;
        clearManualOverride();
        req->send(200, "text/plain", "OK");
    });

    web.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "text/plain", "Not found");
    });

    web.begin();
    Serial.println("[WEB] started on port 80");
}

// ===========================================================================
// Setup / Loop
// ===========================================================================
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== Duco VMC custom firmware ===");
    ducoMutex = xSemaphoreCreateMutex();

    // ----- Wi-Fi -----
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(DEVICE_HOSTNAME);
#if WIFI_USE_STATIC_IP
    if (!WiFi.config(WIFI_STATIC_IP, WIFI_GATEWAY, WIFI_SUBNET, WIFI_DNS1, WIFI_DNS2)) {
        Serial.println("[WiFi] static IP config FAIL");
    }
#endif
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("[WiFi] connecting");
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
        delay(250); Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] OK ip=%s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WiFi] ssid=%s bssid=%s rssi=%d dBm\n",
                      WiFi.SSID().c_str(), WiFi.BSSIDstr().c_str(), WiFi.RSSI());
        Serial.printf("[WiFi] gateway=%s subnet=%s dns=%s\n",
                      WiFi.gatewayIP().toString().c_str(),
                      WiFi.subnetMask().toString().c_str(),
                      WiFi.dnsIP().toString().c_str());
    } else {
        Serial.println("\n[WiFi] FAIL, restart"); ESP.restart();
    }

    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
    t0 = millis();
    while (time(nullptr) < 1700000000 && millis() - t0 < 4000) {
        delay(100);
    }
    initWriteLimiter();
    maybeFetchWeatherForecast(true);

    // ----- mDNS (pour acceder a duco-vmc.local) -----
    if (MDNS.begin(DEVICE_HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("[mDNS] http://%s.local\n", DEVICE_HOSTNAME);
    }

    // ----- OTA -----
    ArduinoOTA.setHostname(DEVICE_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.onStart([](){ Serial.println("[OTA] start"); });
    ArduinoOTA.onEnd  ([](){ Serial.println("[OTA] end");   });
    ArduinoOTA.begin();

    // ----- MQTT -----
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
    mqtt.setBufferSize(2048);

    // ----- UART Duco -----
    DucoSerial.begin(DUCO_BAUD, SERIAL_8N1, DUCO_RX_PIN, DUCO_TX_PIN);
    duco.begin(DucoSerial);
    Serial.printf("[DUCO] UART2 @ %d bauds, RX=%d TX=%d\n",
                  DUCO_BAUD, DUCO_RX_PIN, DUCO_TX_PIN);

    // ----- Web -----
    setupWeb();
}

void loop() {
    ArduinoOTA.handle();

    // Bus Duco : a appeler tres souvent.
    if (takeDucoBus(0)) {
        duco.loop();
        releaseDucoBus();
    }

    // Poll actif : interroge la box regulierement.
    if (pollNowRequested || lastPollMs == 0 || millis() - lastPollMs > DUCO_POLL_INTERVAL_MS) {
        if (takeDucoBus(0)) {
            pollNowRequested = false;
            lastPollMs = millis();
            pollDuco();
            evaluateAutoRegulation(true);
            releaseDucoBus();
        }
    }

    // MQTT
    if (WiFi.status() == WL_CONNECTED) {
        maybeFetchWeatherForecast();
        if (!mqtt.connected()) mqttReconnect();
        else                   mqtt.loop();

        // Publish toutes les 10 s
        if (mqtt.connected() && (millis() - lastPubMs > 10000UL)) {
            lastPubMs = millis();
            mqttPublishState();
        }
    }
}
