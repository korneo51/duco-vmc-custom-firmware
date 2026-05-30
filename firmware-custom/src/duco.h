// =============================================================================
// Protocole Duco — UART interne du DucoBox Energy Comfort
// Reverse engineering : kokx/duco-analysis
// =============================================================================
// Trame :  0xAA 0x55 | LEN | FN | ID | PAYLOAD... | CRC_LO CRC_HI
//   LEN = taille de (FN + ID + PAYLOAD), sans header ni CRC
//   CRC = CRC-16 Modbus (poly 0xA001), calculé sur tout APRES 0xAA 0x55
//   Escape : si 0xAA apparaît mid-message, il est suivi de 0x01 (ignoré
//   pour LEN et CRC, présent seulement sur le fil)
//
// Conventions de FN observées :
//   0x0C = requête mode (read/write)
//   0x0D = ack
//   0x0E = réponse
//   0x10 = requête capteur CO2 (cache)
//   0x24 = requête registre (read/write paramètre, incl. consigne confort)
//   0x25 = ack
//   0x26 = réponse
// =============================================================================
#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

class Duco {
public:
    // ----- État courant exposé en lecture seule -----
    float   tempOda      = NAN;   // air extérieur entrant
    float   tempSup      = NAN;   // air soufflé maison
    float   tempEta      = NAN;   // air extrait maison ≈ intérieur
    float   tempEha      = NAN;   // air rejeté dehors
    float   comfortTemp  = NAN;   // consigne confort zone 1 (°C)
    uint8_t currentMode  = 0xFF;  // voir DUCO_MODE_* ci-dessous
    bool    bypassOpen   = false;
    uint8_t bypassPosition = 0;    // position physique 0..100 %, registre 0x1009
    uint8_t bypassMode   = 0xFF;  // voir DUCO_BYPASS_MODE_* ci-dessous
    uint8_t flowLevel    = 0;     // % débit cible
    bool    online       = false; // true si trame valide reçue dans les 60 s
    int32_t modeTimeRemainingSec = -1;
    int16_t filterRemainingDays = -1;
    String  serialNumber = "";
    uint32_t lastValidMs = 0;

    // ----- Cycle de vie -----
    void begin(HardwareSerial& serial);
    void loop();   // à appeler très souvent (>10 Hz)

    // ----- API haut niveau -----
    bool readMode();
    bool writeMode(uint8_t mode);          // DUCO_MODE_* values
    bool readComfortTemp();                // -> updates comfortTemp
    bool writeComfortTemp(float celsius);  // 10.0 .. 30.0
    bool readBoxTemps();                   // -> updates tempOda/Sup/Eta/Eha
    bool readBypass();                     // -> updates bypassOpen
    bool readBypassMode();                 // -> updates bypassMode
    bool writeBypassMode(uint8_t mode);    // DUCO_BYPASS_MODE_* values
    bool readFilterRemaining();
    bool readSerialNumber();
    bool readRegister(uint8_t high, uint8_t low, uint8_t* reply,
                      size_t replySize, size_t& replyLen,
                      uint32_t timeoutMs = 1500);
    bool writeRegister8(uint8_t high, uint8_t low, uint8_t value,
                        uint8_t* reply, size_t replySize, size_t& replyLen,
                        uint32_t timeoutMs = 1500);

private:
    HardwareSerial* serial_ = nullptr;
    uint8_t msgId_  = 0x10;

    // ----- Bas niveau -----
    uint16_t crc16Modbus(const uint8_t* data, size_t len);
    void     sendFrame(uint8_t fn, const uint8_t* payload, size_t len);
    bool     receiveFrame(uint8_t* outBuf, size_t bufSize, size_t& outLen,
                          uint32_t timeoutMs);
    bool     waitAckThenReply(uint8_t expectedFn, uint8_t expectedId,
                              uint8_t* outBuf, size_t bufSize,
                              size_t& outLen, uint32_t timeoutMs);

    uint8_t  nextMsgId();
};

// ----- Modes ventilation (cf. Information sheet Modbus TCP L2003592-F) -----
#define DUCO_MODE_AUTO         0
#define DUCO_MODE_MANUAL1      4
#define DUCO_MODE_MANUAL2      5
#define DUCO_MODE_MANUAL3      6
#define DUCO_MODE_NOT_AT_HOME  7
#define DUCO_MODE_PERMANENT1   8
#define DUCO_MODE_PERMANENT2   9
#define DUCO_MODE_PERMANENT3  10
#define DUCO_MODE_MANUAL1_X2  0x84
#define DUCO_MODE_MANUAL2_X2  0x85
#define DUCO_MODE_MANUAL3_X2  0x86
#define DUCO_MODE_MANUAL1_X3  0xC4
#define DUCO_MODE_MANUAL2_X3  0xC5
#define DUCO_MODE_MANUAL3_X3  0xC6

// ----- Mode bypass manuel observe sur registre 0x100A -----
#define DUCO_BYPASS_MODE_AUTO  0
#define DUCO_BYPASS_MODE_SHUT  1
#define DUCO_BYPASS_MODE_OPEN  2

// ----- Types des 4 températures internes de la box -----
// Source: composant ESPHome kokx, DucoBoxTemperatureSensor.
#define DUCO_TEMP_TYPE_ODA     0x00
#define DUCO_TEMP_TYPE_SUP     0x01
#define DUCO_TEMP_TYPE_ETA     0x02
#define DUCO_TEMP_TYPE_EHA     0x03
#define DUCO_REG_COMFORT_Z1    0x0102    // = 258, mais kokx voit 0x120a (4618)
                                         // sur le bus UART -> à confirmer
