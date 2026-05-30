// =============================================================================
// Implémentation protocole Duco — UART interne
// Voir duco.h pour la doc protocole
// =============================================================================
#include "duco.h"
#include "config.h"

#ifndef DUCO_DEBUG_PROTOCOL
#define DUCO_DEBUG_PROTOCOL 0
#endif

#ifndef DUCO_DEBUG_FRAMES
#define DUCO_DEBUG_FRAMES 0
#endif

#define DUCO_FRAME_HEADER0  0xAA
#define DUCO_FRAME_HEADER1  0x55
#define DUCO_ESCAPE_BYTE    0x01

#define DUCO_FN_MODE_REQ    0x0C
#define DUCO_FN_MODE_ACK    0x0D
#define DUCO_FN_MODE_REPLY  0x0E

#define DUCO_FN_REG_REQ     0x24
#define DUCO_FN_REG_ACK     0x25
#define DUCO_FN_REG_REPLY   0x26

static bool isValidMode(uint8_t mode) {
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

static bool decodeTenthsTemperature(const uint8_t* data, size_t len, float& out) {
    if (len < 8) return false;

    int16_t raw = (int16_t)(data[6] | (data[7] << 8));
    if (raw <= -1000 || raw > 1000) return false;

    out = raw / 10.0f;
    return true;
}

// ---------------------------------------------------------------------------
// CRC-16 Modbus (poly 0xA001, init 0xFFFF, refin/refout, no xorout)
// ---------------------------------------------------------------------------
uint16_t Duco::crc16Modbus(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
            else              crc >>= 1;
        }
    }
    return crc;
}

uint8_t Duco::nextMsgId() {
    msgId_++;
    if (msgId_ == 0xAA) msgId_++;   // évite collision avec header
    return msgId_;
}

// ---------------------------------------------------------------------------
// Envoi d'une trame : applique header, len, CRC, et escape les 0xAA
// ---------------------------------------------------------------------------
void Duco::sendFrame(uint8_t fn, const uint8_t* payload, size_t len) {
    // Trame logique : LEN | FN | ID | payload
    uint8_t id  = nextMsgId();
    uint8_t logical[256];
    size_t  llen = 0;

    logical[llen++] = (uint8_t)(2 + len);  // LEN = FN + ID + payload
    logical[llen++] = fn;
    logical[llen++] = id;
    memcpy(logical + llen, payload, len);
    llen += len;

    uint16_t crc = crc16Modbus(logical, llen);

#if DUCO_DEBUG_FRAMES
    Serial.printf("[DUCO TX] fn=0x%02X id=0x%02X len=%u payload=", fn, id, (unsigned)len);
    for (size_t i = 0; i < len; i++) Serial.printf("%02X ", payload[i]);
    Serial.println();
#endif

    // Sortie sur le fil : header + escape(logical + CRC)
    serial_->write(DUCO_FRAME_HEADER0);
    serial_->write(DUCO_FRAME_HEADER1);

    auto writeEscaped = [&](uint8_t b) {
        serial_->write(b);
        if (b == DUCO_FRAME_HEADER0) serial_->write(DUCO_ESCAPE_BYTE);
    };

    for (size_t i = 0; i < llen; i++) writeEscaped(logical[i]);
    writeEscaped((uint8_t)(crc & 0xFF));
    writeEscaped((uint8_t)(crc >> 8));

    serial_->flush();
}

// ---------------------------------------------------------------------------
// Réception d'une trame complète, avec dé-escape et validation CRC
// outBuf reçoit LEN | FN | ID | payload (sans header, sans CRC)
// ---------------------------------------------------------------------------
bool Duco::receiveFrame(uint8_t* outBuf, size_t bufSize, size_t& outLen,
                        uint32_t timeoutMs) {
    uint32_t start = millis();
    enum State { WAIT_H0, WAIT_H1, IN_FRAME } state = WAIT_H0;

    uint8_t  decoded[256];
    size_t   declen = 0;
    bool     lastWasAA = false;

    while (millis() - start < timeoutMs) {
        while (serial_->available()) {
            uint8_t b = serial_->read();

            if (state == WAIT_H0) {
                if (b == DUCO_FRAME_HEADER0) state = WAIT_H1;
            } else if (state == WAIT_H1) {
                if (b == DUCO_FRAME_HEADER1) {
                    state = IN_FRAME;
                    declen = 0;
                    lastWasAA = false;
                } else if (b == DUCO_FRAME_HEADER0) {
                    state = WAIT_H1;
                } else {
                    state = WAIT_H0;
                }
            } else { // IN_FRAME
                // dé-escape : si dernier était 0xAA et celui-ci est 0x01, on jette
                if (lastWasAA && b == DUCO_ESCAPE_BYTE) {
                    lastWasAA = false;
                    continue;
                }
                // Si on a un 0xAA suivi d'un 0x55, c'est un nouveau header
                if (lastWasAA && b == DUCO_FRAME_HEADER1) {
                    // Restart frame
                    declen = 0;
                    lastWasAA = false;
                    continue;
                }
                if (declen >= sizeof(decoded)) {
                    state = WAIT_H0; // trop long, abandon
                    continue;
                }
                if (lastWasAA) {
                    decoded[declen++] = DUCO_FRAME_HEADER0;
                    lastWasAA = false;
                }
                if (b == DUCO_FRAME_HEADER0) {
                    lastWasAA = true;
                    continue;
                }
                decoded[declen++] = b;

                // Trame complète ? On peut vérifier dès que declen >= LEN+3
                // (LEN au début, puis FN+ID+payload = LEN bytes, puis CRC 2 bytes)
                if (declen >= 3) {
                    uint8_t expectedLen = decoded[0]; // LEN = FN+ID+payload
                    if (declen == (size_t)expectedLen + 1 + 2) {
                        // declen = LEN_byte + FN+ID+payload + CRC2
                        uint16_t crcCalc = crc16Modbus(decoded, declen - 2);
                        uint16_t crcRecv = decoded[declen - 2]
                                        | (decoded[declen - 1] << 8);
                        if (crcCalc == crcRecv) {
                            // OK ! On copie LEN+FN+ID+payload (sans CRC)
                            size_t payloadLen = declen - 2;
                            if (payloadLen > bufSize) return false;
                            memcpy(outBuf, decoded, payloadLen);
                            outLen = payloadLen;
                            lastValidMs = millis();
                            online = true;
#if DUCO_DEBUG_FRAMES
                            Serial.printf("[DUCO RX] fn=0x%02X id=0x%02X len=%u payload=",
                                          outBuf[1], outBuf[2], (unsigned)(outLen >= 3 ? outLen - 3 : 0));
                            for (size_t i = 3; i < outLen; i++) Serial.printf("%02X ", outBuf[i]);
                            Serial.println();
#endif
                            return true;
                        }
                        // CRC fail -> on abandonne et on attend le prochain header
                        state = WAIT_H0;
                    }
                }
            }
        }
        delay(1);
    }
    return false;
}

// ---------------------------------------------------------------------------
// Attend l'ACK puis la REPLY de la box pour la dernière trame envoyée
// ---------------------------------------------------------------------------
bool Duco::waitAckThenReply(uint8_t expectedFn, uint8_t expectedId,
                            uint8_t* outBuf, size_t bufSize,
                            size_t& outLen, uint32_t timeoutMs) {
    uint8_t buf[256];
    size_t  len;
    uint32_t start = millis();

    // 1) Ack
    bool ackSeen = false;
    while (millis() - start < timeoutMs / 2) {
        if (!receiveFrame(buf, sizeof(buf), len, 200)) continue;
        // buf = LEN | FN | ID | payload
        if (len >= 3 && buf[1] == (expectedFn + 1) && buf[2] == expectedId) {
            ackSeen = true;
            break;
        }
    }
    if (!ackSeen) {
#if DUCO_DEBUG_PROTOCOL
        Serial.printf("[DUCO] ACK timeout fn=0x%02X id=0x%02X\n", expectedFn, expectedId);
#endif
        return false;
    }

    // 2) Reply
    while (millis() - start < timeoutMs) {
        if (!receiveFrame(buf, sizeof(buf), len, 200)) continue;
        if (len >= 3 && buf[1] == (expectedFn + 2) && buf[2] == expectedId) {
            if (len > bufSize) return false;
            memcpy(outBuf, buf, len);
            outLen = len;
            return true;
        }
    }
#if DUCO_DEBUG_PROTOCOL
    Serial.printf("[DUCO] REPLY timeout fn=0x%02X id=0x%02X\n", expectedFn + 2, expectedId);
#endif
    return false;
}

// ===========================================================================
// API publique
// ===========================================================================
void Duco::begin(HardwareSerial& serial) {
    serial_ = &serial;
}

void Duco::loop() {
    // Marque offline si rien depuis 60 s
    if (online && (millis() - lastValidMs) > 60000UL) online = false;
    // Sniff passif : décode les trames spontanées de la box
    // (la box parle en permanence avec ses nœuds, on peut juste écouter)
    uint8_t buf[256];
    size_t  len;
    if (receiveFrame(buf, sizeof(buf), len, 5)) {
        // Trame spontanée : on peut interpréter certains messages broadcast.
        // Pour l'instant on se contente de marquer "online" (fait dans receiveFrame).
    }
}

// ----- MODE ----------------------------------------------------------------
bool Duco::readMode() {
    if (!serial_) return false;
    uint8_t payload[2] = { 0x02, 0x01 };   // sub 0x02 = read, node 0x01 = BOX
    uint8_t lastId = msgId_ + 1;
    if (lastId == 0xAA) lastId++;
    sendFrame(DUCO_FN_MODE_REQ, payload, sizeof(payload));

    uint8_t reply[64]; size_t rlen;
    if (!waitAckThenReply(DUCO_FN_MODE_REQ, lastId, reply, sizeof(reply), rlen, 1500))
        return false;
    // reply = LEN | 0x0E | ID | payload...
    // byte 0 du payload (= reply[3]) = mode pour node BOX
    if (rlen < 4) return false;
    currentMode = reply[3];
    if (rlen >= 6) flowLevel = reply[5];
    if (rlen >= 17) {
        modeTimeRemainingSec = (uint16_t)(reply[16] << 8) | reply[15];
    }
    return true;
}

bool Duco::writeMode(uint8_t mode) {
    if (!serial_) return false;
    if (!isValidMode(mode)) return false;
    uint8_t payload[3] = { 0x04, 0x01, mode };   // sub 0x04 0x01 = write
    uint8_t lastId = msgId_ + 1;
    if (lastId == 0xAA) lastId++;
    sendFrame(DUCO_FN_MODE_REQ, payload, sizeof(payload));

    uint8_t reply[64]; size_t rlen;
    if (!waitAckThenReply(DUCO_FN_MODE_REQ, lastId, reply, sizeof(reply), rlen, 1500))
        return false;
    currentMode = mode;
    return true;
}

// ----- CONSIGNE CONFORT ----------------------------------------------------
// Trace kokx (lecture) :
//   Board: aa 55 05 24 ID 00 12 0a CRC CRC
//   Box ack -> Box reply: 09 26 ID 01 12 0a <val> 00 00 00 CRC CRC
//                                            ^ val = T * 10
// Trace kokx (écriture) :
//   Board: aa 55 09 24 ID 01 12 0a <val> 00 00 00 CRC CRC
bool Duco::readComfortTemp() {
    if (!serial_) return false;
    uint8_t payload[3] = { 0x00, 0x12, 0x0A };
    uint8_t lastId = msgId_ + 1;
    if (lastId == 0xAA) lastId++;
    sendFrame(DUCO_FN_REG_REQ, payload, sizeof(payload));

    uint8_t reply[64]; size_t rlen;
    if (!waitAckThenReply(DUCO_FN_REG_REQ, lastId, reply, sizeof(reply), rlen, 1500))
        return false;
    // reply = LEN | 0x26 | ID | 01 12 0A <val> 00 00 00
    if (rlen < 7) return false;
    uint8_t val = reply[6];   // byte 4 du payload effectif
    comfortTemp = (float)val / 10.0f;
    return true;
}

bool Duco::writeComfortTemp(float celsius) {
    if (!serial_) return false;
    if (celsius < 10.0f || celsius > 30.0f) return false;
    uint8_t val = (uint8_t)(celsius * 10.0f + 0.5f);
    uint8_t payload[7] = { 0x01, 0x12, 0x0A, val, 0x00, 0x00, 0x00 };
    uint8_t lastId = msgId_ + 1;
    if (lastId == 0xAA) lastId++;
    sendFrame(DUCO_FN_REG_REQ, payload, sizeof(payload));

    uint8_t reply[64]; size_t rlen;
    if (!waitAckThenReply(DUCO_FN_REG_REQ, lastId, reply, sizeof(reply), rlen, 1500))
        return false;
    comfortTemp = celsius;
    return true;
}

bool Duco::readRegister(uint8_t high, uint8_t low, uint8_t* reply,
                        size_t replySize, size_t& replyLen,
                        uint32_t timeoutMs) {
    if (!serial_ || !reply || replySize == 0) return false;

    uint8_t payload[3] = { 0x00, high, low };
    uint8_t lastId = msgId_ + 1;
    if (lastId == 0xAA) lastId++;
    sendFrame(DUCO_FN_REG_REQ, payload, sizeof(payload));

    return waitAckThenReply(DUCO_FN_REG_REQ, lastId,
                            reply, replySize, replyLen, timeoutMs);
}

bool Duco::writeRegister8(uint8_t high, uint8_t low, uint8_t value,
                          uint8_t* reply, size_t replySize, size_t& replyLen,
                          uint32_t timeoutMs) {
    if (!serial_ || !reply || replySize == 0) return false;

    uint8_t payload[7] = { 0x01, high, low, value, 0x00, 0x00, 0x00 };
    uint8_t lastId = msgId_ + 1;
    if (lastId == 0xAA) lastId++;
    sendFrame(DUCO_FN_REG_REQ, payload, sizeof(payload));

    return waitAckThenReply(DUCO_FN_REG_REQ, lastId,
                            reply, replySize, replyLen, timeoutMs);
}

bool Duco::readSerialNumber() {
    if (!serial_) return false;

    uint8_t payload[5] = { 0x01, 0x01, 0x00, 0x1A, 0x10 };
    uint8_t lastId = msgId_ + 1;
    if (lastId == 0xAA) lastId++;
    sendFrame(0x10, payload, sizeof(payload));

    uint8_t reply[96];
    size_t rlen = 0;
    uint32_t start = millis();
    bool replySeen = false;
    while (millis() - start < 1500) {
        if (!receiveFrame(reply, sizeof(reply), rlen, 200)) continue;
        if (rlen >= 6 && reply[1] == 0x12 && reply[2] == lastId) {
            replySeen = true;
            break;
        }
    }
    if (!replySeen) return false;

    String serial = "";
    for (size_t i = 5; i < rlen; i++) {
        if (reply[i] == 0x00) break;
        if (reply[i] >= 0x20 && reply[i] <= 0x7E) serial += (char)reply[i];
    }
    if (serial.length() == 0) return false;
    serialNumber = serial;
    return true;
}

bool Duco::readFilterRemaining() {
    uint8_t reply[64];
    size_t rlen = 0;
    if (!readRegister(0x30, 0x09, reply, sizeof(reply), rlen, 1500))
        return false;

    if (rlen < 7) return false;
    filterRemainingDays = reply[6];
    return true;
}

// ----- TEMPERATURES INTERNES (BOX) -----------------------------------------
// Même mécanisme que le composant ESPHome kokx :
//   requête: 0x24 | 00 <type> 09
//   réponse: 0x26 | ... <val_lo> <val_hi> ...
//   type 0=ODA, 1=SUP, 2=ETA, 3=EHA ; valeur en dixièmes de °C.
bool Duco::readBoxTemps() {
    bool ok = true;
    auto readOne = [&](uint8_t type, float& dest, const char* label) {
        if (!serial_) { ok = false; return; }
        uint8_t payload[3] = { 0x00, type, 0x09 };
        uint8_t lastId = msgId_ + 1;
        if (lastId == 0xAA) lastId++;
        sendFrame(DUCO_FN_REG_REQ, payload, sizeof(payload));

        uint8_t reply[64]; size_t rlen;
        if (!waitAckThenReply(DUCO_FN_REG_REQ, lastId,
                              reply, sizeof(reply), rlen, 1500)) {
            ok = false;
            return;
        }
        if (!decodeTenthsTemperature(reply, rlen, dest)) {
#if DUCO_DEBUG_PROTOCOL
            Serial.printf("[DUCO] temp %s invalid reply len=%u\n", label, (unsigned)rlen);
#endif
            ok = false;
        }
    };

    readOne(DUCO_TEMP_TYPE_ODA, tempOda, "ODA");
    delay(50);
    readOne(DUCO_TEMP_TYPE_SUP, tempSup, "SUP");
    delay(50);
    readOne(DUCO_TEMP_TYPE_ETA, tempEta, "ETA");
    delay(50);
    readOne(DUCO_TEMP_TYPE_EHA, tempEha, "EHA");

    return ok;
}

// ----- BYPASS ---------------------------------------------------------------
bool Duco::readBypass() {
    if (!serial_) return false;

    uint8_t payload[3] = { 0x00, 0x10, 0x09 };
    uint8_t lastId = msgId_ + 1;
    if (lastId == 0xAA) lastId++;
    sendFrame(DUCO_FN_REG_REQ, payload, sizeof(payload));

    uint8_t reply[64]; size_t rlen;
    if (!waitAckThenReply(DUCO_FN_REG_REQ, lastId, reply, sizeof(reply), rlen, 1500))
        return false;

    // reply = LEN | 0x26 | ID | payload ; kokx lit message.data[3].
    if (rlen < 7 || reply[6] > 100) return false;
    bypassPosition = reply[6];
    bypassOpen = bypassPosition > 0;
    return true;
}

bool Duco::readBypassMode() {
    uint8_t reply[64];
    size_t rlen = 0;
    if (!readRegister(0x10, 0x0A, reply, sizeof(reply), rlen, 1500))
        return false;

    if (rlen < 7 || reply[3] != 0x01 || reply[6] > DUCO_BYPASS_MODE_OPEN)
        return false;

    bypassMode = reply[6];
    return true;
}

bool Duco::writeBypassMode(uint8_t mode) {
    if (mode > DUCO_BYPASS_MODE_OPEN) return false;

    uint8_t reply[64];
    size_t rlen = 0;
    if (!writeRegister8(0x10, 0x0A, mode, reply, sizeof(reply), rlen, 1500))
        return false;

    bypassMode = mode;
    return true;
}
