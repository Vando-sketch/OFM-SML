#pragma once
#include "OpenKNX.h"
#include "sml/sml_list.h"
#include "sml/sml_value.h"
#include "ObisAsciiParser.h"
#include "SMLHelpers.h"
#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
#include "OpenKNX/Format/JSON/Writer.h"
#endif
#ifdef ARDUINO_ARCH_RP2040
    #include "pico/sync.h"
#endif
#ifdef ARDUINO_ARCH_ESP32
    #include "freertos/semphr.h"
#endif

// Nimmt genau ein Telegramm zwischen Start- und Endsequenz auf. Übliche Zähler liegen
// bei rund 370 Byte, ein Dreiphasenzähler mit allen Messwerten deutlich darüber. Läuft
// der Puffer über, meldet der Kanal das als "No end sequence within ... bytes".
#ifndef OPENKNX_SML_BUFFER
    #define OPENKNX_SML_BUFFER 1024
#endif

// Wird nicht mehr ausgewertet: der Empfangs-Timeout ist pro Kanal in der ETS einstellbar
// (ParamSML_cReceiveTimeout). Das Define bleibt nur, damit bestehende Projekte weiter bauen.
#ifndef OPENKNX_SML_STALE_TIMEOUT
    #define OPENKNX_SML_STALE_TIMEOUT 5000
#endif

// Platzhalter für "keine Escape-Argumentgruppe gesehen", liegt außerhalb jeder Position
#define SML_NO_ESCAPE_ARG 0xFFFF

class SMLChannel : public OpenKNX::Channel
{

  protected:
    // Schützt die Übergabe von _smlBuffer bzw. _asciiTelegram zwischen writeBuffer() und
    // processFile(), die im Dualcore-Betrieb auf unterschiedlichen Kernen laufen.
#ifdef ARDUINO_ARCH_RP2040
    mutex_t _mutex;
#elif defined(ARDUINO_ARCH_ESP32)
    SemaphoreHandle_t _mutex = nullptr;
#endif
    uint16_t _bufferPos = 0;
    uint8_t _buffer[OPENKNX_SML_BUFFER] = {};
    // uint8_t *_currentFile = nullptr;
    // uint16_t _currentFileSize = 0;
    HardwareSerial *_serial = nullptr;
    sml_buffer *_smlBuffer = NULL;

    // OBIS-ASCII: Der Parser rahmt Telegramme direkt in _buffer (im ASCII-Betrieb ungenutzt
    // für SML, daher kein zusätzlicher RAM). Ein fertiges Telegramm wird als Kopie per
    // Mutex an processFile() übergeben, genau wie _smlBuffer.
    ObisAsciiParser _asciiParser;
    char *_asciiTelegram = nullptr;
    uint16_t _asciiTelegramLen = 0;

    bool _serialOpen = false;       // Schnittstelle wurde in setSerial() geöffnet
    bool _noSerialLogged = false;   // Warnung "keine Schnittstelle" nur einmal
    bool _asciiHintLogged = false;  // ASCII-Hinweis im SML-Betrieb nur einmal pro Start

    // Diagnose im ASCII-Betrieb, wenn Bytes ankommen, aber kein gültiges Telegramm
    SMLHelpers::SmlStartDetector _smlDetector;
    bool _smlInAsciiHintLogged = false;    // "Zähler sendet SML" nur einmal pro Start
    bool _asciiSettingsHintLogged = false; // Hinweis Baudrate/Datenformat nur einmal pro Start
    uint16_t _asciiBadDrops = 0;           // verworfene Telegramme seit dem letzten gültigen
    uint32_t _asciiLastDump = 0;           // Zeitpunkt des letzten Debug-Dumps
    uint16_t _asciiSuppressedDumps = 0;    // seitdem unterdrückte Dumps

    int64_t _sentCounterIn = 0;
    int64_t _sentCounterInT1 = 0;
    int64_t _sentCounterInT2 = 0;
    int64_t _sentCounterOut = 0;
    int64_t _sentCounterOutT1 = 0;
    int64_t _sentCounterOutT2 = 0;
    uint32_t _sentCounterInTime = 0;
    uint32_t _sentCounterInT1Time = 0;
    uint32_t _sentCounterInT2Time = 0;
    uint32_t _sentCounterOutTime = 0;
    uint32_t _sentCounterOutT1Time = 0;
    uint32_t _sentCounterOutT2Time = 0;
    bool _capture = false;
    // Escape-Zustand der Rahmenerkennung: _escapePending ist gesetzt, sobald eine
    // Escape-Marke gelesen wurde und die nächste 4-Byte-Gruppe deren Argument ist.
    // _escapeArgPos merkt sich die zuletzt als Argument verbrauchte Gruppe.
    bool _escapePending = false;
    uint16_t _escapeArgPos = SML_NO_ESCAPE_ARG;
    double _sentDataPower = 0;
    double _sentDataPowerL1 = 0;
    double _sentDataPowerL2 = 0;
    double _sentDataPowerL3 = 0;
    uint32_t _sentDataPowerTime = 0;
    uint32_t _sentDataPowerL1Time = 0;
    uint32_t _sentDataPowerL2Time = 0;
    uint32_t _sentDataPowerL3Time = 0;

    double _sentDataCurrentL1 = 0;
    double _sentDataCurrentL2 = 0;
    double _sentDataCurrentL3 = 0;
    uint32_t _sentDataCurrentL1Time = 0;
    uint32_t _sentDataCurrentL2Time = 0;
    uint32_t _sentDataCurrentL3Time = 0;

    double _sentDataVoltageL1 = 0;
    double _sentDataVoltageL2 = 0;
    double _sentDataVoltageL3 = 0;
    uint32_t _sentDataVoltageL1Time = 0;
    uint32_t _sentDataVoltageL2Time = 0;
    uint32_t _sentDataVoltageL3Time = 0;

    double _sentDataFrequency = 0;
    uint32_t _sentDataFrequencyTime = 0;

    bool _lastReceivedStatus = false;
    uint32_t _lastReceivedByte = 0;
    uint32_t _lastReceivedFile = 0;
    OpenKNX::Led::FunctionGroup *_led = nullptr;

    // Innerhalb eines Telegramms gewinnt der erste OBIS-Eintrag mit Statusfeld, weitere
    // werden für das KO ignoriert (der SML-Spec zufolge kann grundsätzlich jeder Eintrag
    // ein eigenes, potenziell unterschiedliches Statusfeld tragen).
    bool _statusSentThisFile = false;

    struct
    {
        uint8_t maxTariff = 0; // 0-255, in der Praxis nur einstellig
        bool bidirectional : 1; // Einspeisung (2.8.0) vorhanden
        bool power : 1;
        bool voltage : 1;
        bool current : 1;
        bool frequency : 1;
    } _features = {};

    // double _sentDataCurrentL1 = 0;
    // double _sentDataCurrentL2 = 0;
    // double _sentDataCurrentL3 = 0;
    // double _sentDataVoltageL1 = 0;
    // double _sentDataVoltageL2 = 0;
    // double _sentDataVoltageL3 = 0;
    // double _sentDataFrequency = 0;

    void beginCapture(uint16_t start);
    bool moveBuffer(uint16_t length);
    uint16_t crc16(uint8_t byte, uint16_t crc);
    void removeEscaping();
    void lockBuffer();
    bool tryLockBuffer();
    void unlockBuffer();
    void processFile();
    void processFileAscii();
    void writeBufferSml(uint8_t byte);
    void writeBufferAscii(uint8_t byte);
    void logAsciiDrop(const char *reason);
    bool isAscii();
    void publishIdentifier(const char *obis, const char *identifier);

    static void asciiLineTrampoline(void *ctx, const char *line, size_t len, ObisAscii::LineKind kind, const ObisAscii::Entry *entry);
    void processAsciiLine(const char *line, size_t len, ObisAscii::LineKind kind, const ObisAscii::Entry *entry);

    void processDataPoint(sml_list_entry *entry);
    void processDataPoint(char *obis, const uint8_t &a, const uint8_t &b, const uint8_t &c, const uint8_t &d, const uint8_t &e, const uint8_t &f, boolean value, const char *statusSuffix);
    void processDataPoint(char *obis, const uint8_t &a, const uint8_t &b, const uint8_t &c, const uint8_t &d, const uint8_t &e, const uint8_t &f, double value, const char *statusSuffix);
    void processDataPoint(char *obis, const uint8_t &a, const uint8_t &b, const uint8_t &c, const uint8_t &d, const uint8_t &e, const uint8_t &f, char *value, uint8_t len);

#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
    // Ein JSON-Snapshot pro Telegramm (kein Retain: ohne Zeitstempel im Payload
    // wäre ein zwischengespeicherter, veralteter Wert beim Subscriber-Connect irreführend).
    OpenKNX::Format::JSON::Writer _mqttJson;
    bool _mqttHasData = false;
    std::string _mqttIdentifier;
    void mqttBegin();
    void mqttAppend(const char *key, float value);
    void mqttPublish();
#endif

  public:
    SMLChannel(uint8_t index);

    void setSerial(HardwareSerial *serial);
    HardwareSerial *getSerial();
    void setup(bool configured) override;
    void loop(bool configured) override;
    void loopStatus();
#if defined(OPENKNX_DUALCORE)
    void setup1(bool configured) override;
    void loop1(bool configured) override;
#endif
    void processInputKo(GroupObject &ko) override;
    const std::string name() override;
    void writeBuffer(uint8_t byte);
    bool isActive();
    // true, solange innerhalb des Empfangs-Timeouts (ETS) ein gültiges Telegramm ankam
    bool receiveStatus();
    // Kurzer Status für das Diagnose-KO (DPT 16, maximal 14 Zeichen)
    const std::string diagnoseInfo();
    // Protokoll, Baudrate und Datenformat, z. B. "ASCII 9600 7E1"; leer, wenn der Kanal inaktiv ist
    const std::string serialInfo();
};
