#include "SMLChannel.h"
#include "SMLModule.h"
#include "NetworkModule.h"
#include <sml/sml_file.h>

#define CorrectionOfCycleTimeMS 50

const uint16_t SML_CRC_TABLE[256] PROGMEM =
    {0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF, 0x8C48,
     0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7, 0x1081, 0x0108,
     0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E, 0x9CC9, 0x8D40, 0xBFDB,
     0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876, 0x2102, 0x308B, 0x0210, 0x1399,
     0x6726, 0x76AF, 0x4434, 0x55BD, 0xAD4A, 0xBCC3, 0x8E58, 0x9FD1, 0xEB6E,
     0xFAE7, 0xC87C, 0xD9F5, 0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E,
     0x54B5, 0x453C, 0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD,
     0xC974, 0x4204, 0x538D, 0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB,
     0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3, 0x5285,
     0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A, 0xDECD, 0xCF44,
     0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72, 0x6306, 0x728F, 0x4014,
     0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9, 0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5,
     0xA96A, 0xB8E3, 0x8A78, 0x9BF1, 0x7387, 0x620E, 0x5095, 0x411C, 0x35A3,
     0x242A, 0x16B1, 0x0738, 0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862,
     0x9AF9, 0x8B70, 0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E,
     0xF0B7, 0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF,
     0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036, 0x18C1,
     0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E, 0xA50A, 0xB483,
     0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5, 0x2942, 0x38CB, 0x0A50,
     0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD, 0xB58B, 0xA402, 0x9699, 0x8710,
     0xF3AF, 0xE226, 0xD0BD, 0xC134, 0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7,
     0x6E6E, 0x5CF5, 0x4D7C, 0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1,
     0xA33A, 0xB2B3, 0x4A44, 0x5BCD, 0x6956, 0x78DF, 0x0C60, 0x1DE9, 0x2F72,
     0x3EFB, 0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232,
     0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A, 0xE70E,
     0xF687, 0xC41C, 0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1, 0x6B46, 0x7ACF,
     0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9, 0xF78F, 0xE606, 0xD49D,
     0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330, 0x7BC7, 0x6A4E, 0x58D5, 0x495C,
     0x3DE3, 0x2C6A, 0x1EF1, 0x0F78};

// Die ersten vier Bytes sind die Escape-Marke, alle acht die maskierte 1B-Folge
const uint8_t SML_ESCAPE[8] = {0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B};
// Marke plus Argumentgruppe; SML_START + 4 ist die Argumentgruppe für sich
const uint8_t SML_START[8] = {0x1B, 0x1B, 0x1B, 0x1B, 0x01, 0x01, 0x01, 0x01};
// erstes Byte der Argumentgruppe, die eine Marke als Rahmenende ausweist
const uint8_t SML_ESCAPE_END = 0x1A;

SMLChannel::SMLChannel(uint8_t index)
    : _asciiParser((char *)_buffer, OPENKNX_SML_BUFFER)
{
    _channelIndex = index;
}

HardwareSerial *SMLChannel::getSerial()
{
    return _serial;
}

bool SMLChannel::isAscii()
{
    return ParamSML_cProtocol == 1;
}

// Wird von main.cpp nach openknx.setup() aufgerufen, die Parameter sind hier also gültig.
// Ein inaktiver Kanal öffnet seine Schnittstelle nicht.
void SMLChannel::setSerial(HardwareSerial *serial)
{
    _serial = serial;

    if (_serial == nullptr) return;

    if (!isActive())
    {
        logDebugP("Kanal inaktiv, serielle Schnittstelle bleibt geschlossen");
        return;
    }

    const uint32_t baud = SMLHelpers::baudFromParam(ParamSML_cBaudrate);
    const SMLHelpers::SerialFormat fmt = SMLHelpers::formatFromParams(ParamSML_cProtocol, ParamSML_cDataFormat);

    getSerial()->begin(baud, fmt == SMLHelpers::SerialFormat::F7E1 ? SERIAL_7E1 : SERIAL_8N1);
    _serialOpen = true;

    char info[24] = {};
    SMLHelpers::formatSerialInfo(info, sizeof(info), ParamSML_cProtocol, baud, fmt);
    logInfoP("Serielle Schnittstelle geöffnet: %s", info);
}

const std::string SMLChannel::name()
{
    return "SML";
}

bool SMLChannel::isActive()
{
    return ParamSML_cType > 0 && !ParamSML_cSuspended;
}

bool SMLChannel::receiveStatus()
{
    return _lastReceivedStatus;
}

const std::string SMLChannel::diagnoseInfo()
{
    if (!isActive())
        return "INACTIVE";

    if (!_lastReceivedStatus)
        return "NODATA";

    char flags[5] = {};
    uint8_t i = 0;
    if (_features.power) flags[i++] = 'P';
    if (_features.voltage) flags[i++] = 'V';
    if (_features.current) flags[i++] = 'A';
    if (_features.frequency) flags[i++] = 'F';

    char buffer[14] = {};
    if (i > 0)
        snprintf(buffer, sizeof(buffer), "%sR T%u %s", _features.bidirectional ? "2" : "1", _features.maxTariff, flags);
    else
        snprintf(buffer, sizeof(buffer), "%sR T%u", _features.bidirectional ? "2" : "1", _features.maxTariff);

    return std::string(buffer);
}

// Protokoll, Baudrate und Datenformat für die Konsole. Das Diagnose-KO ist ein DPT 16
// mit nur 14 Zeichen, deshalb steht das hier getrennt von diagnoseInfo().
const std::string SMLChannel::serialInfo()
{
    if (!isActive())
        return "";

    char buffer[24] = {};
    SMLHelpers::formatSerialInfo(buffer, sizeof(buffer), ParamSML_cProtocol,
                                 SMLHelpers::baudFromParam(ParamSML_cBaudrate),
                                 SMLHelpers::formatFromParams(ParamSML_cProtocol, ParamSML_cDataFormat));
    return std::string(buffer);
}

void SMLChannel::setup(bool configured)
{
#ifdef ARDUINO_ARCH_RP2040
    mutex_init(&_mutex);
#elif defined(ARDUINO_ARCH_ESP32)
    _mutex = xSemaphoreCreateMutex();
#endif

    _led = openknx.ledFunctions.get(201 + _channelIndex);
    openknxSMLModule.ledHelper(_led, false, _lastReceivedByte);
}

void SMLChannel::loop(bool configured)
{
    // Kein frühes return: auch ein Kanal ohne Schnittstelle muss seinen Status auswerten,
    // damit "Zähler empfängt" nach dem Start einmalig 0 meldet.
    if (!_noSerialLogged && _serial == nullptr && isActive())
    {
        logWarningP("Kanal aktiv, aber keine serielle Schnittstelle vorhanden");
        _noSerialLogged = true;
    }

#if !defined(OPENKNX_DUALCORE)
    if (_serialOpen)
    {
        while (getSerial()->available())
        {
            writeBuffer(getSerial()->read());
        }
    }
#endif

    processFile();

    loopStatus();
}

void SMLChannel::loopStatus()
{
    const bool receiving = SMLHelpers::isReceiving(millis(), _lastReceivedFile, SMLHelpers::timeoutMsFromParam(ParamSML_cReceiveTimeout));

    if (receiving != _lastReceivedStatus)
    {
        _lastReceivedStatus = receiving;
        if (_led != nullptr) openknxSMLModule.ledHelper(_led, receiving, _lastReceivedByte);
    }

    // Bei jedem Durchlauf: valueCompare() sendet nur bei Änderung, beim ersten Aufruf (KO noch
    // nicht initialisiert) aber immer. So meldet auch ein Kanal ohne Daten einmalig 0.
    if (isActive() && knx.configured() && openknx.afterStartupDelay())
        KoSML_cReceiving.valueCompare(_lastReceivedStatus, DPT_Bool);
}

#if defined(OPENKNX_DUALCORE)
void SMLChannel::setup1(bool configured)
{
}

void SMLChannel::loop1(bool configured)
{
    if (!_serialOpen) return;

    while (getSerial()->available())
    {
        writeBuffer(getSerial()->read());
    }
}
#endif

void SMLChannel::processInputKo(GroupObject &ko)
{
}

void SMLChannel::writeBuffer(uint8_t byte)
{
    _lastReceivedByte = millis();
    openknxSMLModule._lastReceivedByte = millis();

    if (isAscii())
        writeBufferAscii(byte);
    else
        writeBufferSml(byte);
}

void SMLChannel::writeBufferSml(uint8_t byte)
{
    // Kein Telegrammende innerhalb des Puffers gefunden, der Inhalt ist damit unbrauchbar
    // und wird komplett verworfen. Anschließend wird wieder auf eine Startsequenz gewartet.
    // Eine vollständige Startsequenz kann nicht verloren gehen, die hätte unten bereits
    // resynchronisiert. Da die Telegramme im Sekundentakt kommen, kostet das höchstens eines.
    if (_bufferPos >= OPENKNX_SML_BUFFER)
    {
        logErrorP("No end sequence within %u bytes, buffer dropped", _bufferPos);
        _bufferPos = 0;
        _capture = false;
    }

    _buffer[_bufferPos] = byte;
    _bufferPos++;

    if (_capture)
    {
        bool frameComplete = false;

        // Resync-Netz: ein neuer Rahmenanfang wird auch außerhalb des 4-Byte-Rasters
        // erkannt. Sonst müsste nach verlorenen Bytes bis zum Pufferüberlauf gewartet
        // werden, weil der Rest des Rahmens dann gegen das Raster verschoben ist.
        // Die Argumentgruppe einer Escape-Marke ist ausgenommen, sonst würde eine
        // maskierte 1B-Folge gefolgt von 01 01 01 01 als Rahmenanfang gelesen.
        if (_bufferPos >= 16 && (uint16_t)(_bufferPos - 8) != _escapeArgPos &&
            memcmp(_buffer + _bufferPos - 8, SML_START, 8) == 0)
        {
            if (openknxSMLModule.debug()) logWarningP("Start with newly found start sequence in running message");
            beginCapture(_bufferPos - 8);
        }
        // Das Transportformat ist durchgängig 4-Byte-ausgerichtet, dafür existieren die
        // Füllbytes. Escape-Marken und ihre Argumente liegen daher immer auf dem Raster,
        // und nur dort darf geprüft werden. Ein ungerasterter Vergleich würde die zweite
        // Hälfte einer maskierten 1B-Folge für ein Rahmenende halten.
        else if ((_bufferPos % 4) == 0)
        {
            const uint8_t *group = _buffer + _bufferPos - 4;

            if (_escapePending)
            {
                _escapePending = false;

                if (memcmp(group, SML_ESCAPE, 4) == 0)
                {
                    // maskierte 1B-Folge in den Nutzdaten, removeEscaping() löst sie später auf
                    _escapeArgPos = _bufferPos - 4;
                }
                else if (memcmp(group, SML_START + 4, 4) == 0)
                {
                    if (openknxSMLModule.debug()) logWarningP("Start with newly found start sequence in running message");
                    beginCapture(_bufferPos - 8);
                }
                else if (group[0] == SML_ESCAPE_END)
                {
                    frameComplete = true;
                }
                else
                {
                    logErrorP("Invalid escape sequence, buffer dropped");
                    _bufferPos = 0;
                    _capture = false;
                }
            }
            else if (memcmp(group, SML_ESCAPE, 4) == 0)
            {
                _escapePending = true;
            }
        }

        if (frameComplete)
        {
            _capture = false;

            // read checksum and fillbyte
            const uint8_t *checksum = _buffer + _bufferPos - 2;
            const uint16_t crc_received = (checksum[1] << 8) | checksum[0];
            const uint8_t fillBytes = _buffer[_bufferPos - 3];

            // calc verify crc
            uint16_t crc = 0xFFFF; // reset
            for (size_t i = 0; i < _bufferPos - 2; i++)
                crc = crc16(_buffer[i], crc);
            crc ^= 0xFFFF;

            if (crc == crc_received)
            {
                // fillBytes stammt aus dem Telegramm (laut Spec 0..3) und wird gleich von
                // _bufferPos abgezogen. Ohne Prüfung würde _bufferPos (uint16_t) unterlaufen
                // und die Folgeschritte weit über _buffer hinaus lesen und schreiben.
                if (fillBytes > 3 || _bufferPos <= 16 + fillBytes)
                {
                    logErrorP("Invalid sml file (length %u, fill bytes %u)", _bufferPos, fillBytes);
                    moveBuffer(_bufferPos);
                    return;
                }

                // preapre next step
                moveBuffer(8);           // remove start sequence
                _bufferPos -= 8;         // remove end sequence
                _bufferPos -= fillBytes; // remove fill bytes
                removeEscaping();        // remove escaping

                // process
                lockBuffer();
                if (_smlBuffer != NULL)
                {
                    // vorheriges, noch nicht verarbeitetes Telegramm verwerfen
                    sml_buffer_free(_smlBuffer);
                    _smlBuffer = NULL;
                }
                _smlBuffer = sml_buffer_init(_bufferPos);
                if (_smlBuffer != NULL)
                {
                    memcpy(_smlBuffer->buffer, _buffer, _bufferPos);
                }
                else
                {
                    logErrorP("sml_buffer_init failed (out of memory)");
                }
                unlockBuffer();
            }
            else
            {
                // Die Länge zeigt, ob das Telegramm verkürzt ankam. Ein Hexdump des Puffers
                // dauert deutlich länger als der serielle Empfangspuffer überbrücken kann
                // und würde damit das nächste Telegramm gleich mit beschädigen.
                logErrorP("Invalid sml file dropped (checksum %04X != %04X, %u bytes)", crc, crc_received, _bufferPos);
            }
            moveBuffer(_bufferPos);
        }
    }
    else
    {
        if (_bufferPos >= 8)
        {
            if (memcmp(_buffer, SML_START, 8) == 0)
            {
                // logInfoP("START");
                beginCapture(0);
            }
            else
            {
                if (openknxSMLModule.debug())
                {
                    logErrorP("Unexpected chars");
                }
                // Unabhängig vom Debug-Modus und nur einmal pro Start: der Kanal ist
                // vermutlich auf SML eingestellt, der Zähler sendet aber OBIS-ASCII.
                if (!_asciiHintLogged && SMLHelpers::looksLikeObisAscii(_buffer, _bufferPos))
                {
                    logWarningP("Sieht nach OBIS-ASCII aus, Protokoll in der ETS umstellen");
                    _asciiHintLogged = true;
                }
                moveBuffer(1);
            }
        }
    }
}

void SMLChannel::writeBufferAscii(uint8_t byte)
{
    // Sendet der Zähler SML, kommt im ASCII-Betrieb nie ein gültiges Telegramm zustande
    if (!_smlInAsciiHintLogged && _smlDetector.feed(byte))
    {
        logWarningP("Zähler sendet SML, Protokoll in der ETS auf SML umstellen");
        _smlInAsciiHintLogged = true;
    }

    const ObisAsciiParser::Result result = _asciiParser.feed(byte);
    if (result == ObisAsciiParser::Result::None) return;

    if (result == ObisAsciiParser::Result::Dropped)
    {
        switch (_asciiParser.lastDropReason())
        {
            case ObisAsciiParser::DropReason::Overflow:
                logErrorP("No end of OBIS-ASCII telegram within %u bytes, telegram dropped", OPENKNX_SML_BUFFER);
                logAsciiDrop("Pufferüberlauf");
                break;
            case ObisAsciiParser::DropReason::Garbage:
                logAsciiDrop("Steuerzeichen");
                break;
            case ObisAsciiParser::DropReason::Resync:
                if (openknxSMLModule.debug()) logWarningP("OBIS-ASCII telegram dropped (new telegram started)");
                return; // Resync heißt: es kommen Telegramme an, das zählt nicht als Fehler
            case ObisAsciiParser::DropReason::BccMismatch:
                if (openknxSMLModule.debug()) logWarningP("OBIS-ASCII telegram dropped (BCC mismatch)");
                break;
            case ObisAsciiParser::DropReason::Framing:
                if (openknxSMLModule.debug()) logWarningP("OBIS-ASCII telegram dropped (framing error)");
                break;
            default:
                break;
        }

        // Unabhängig vom Debug-Modus und nur einmal pro Start: Es kommen Bytes an, aber nie ein gültiges
        // Telegramm. Fast immer passen Baudrate oder Datenformat nicht zum Zähler.
        if (_asciiBadDrops < UINT16_MAX) _asciiBadDrops++;
        if (!_asciiSettingsHintLogged && _asciiBadDrops >= 3)
        {
            _asciiSettingsHintLogged = true;
            char info[24] = {};
            SMLHelpers::formatSerialInfo(info, sizeof(info), ParamSML_cProtocol,
                                         SMLHelpers::baudFromParam(ParamSML_cBaudrate),
                                         SMLHelpers::formatFromParams(ParamSML_cProtocol, ParamSML_cDataFormat));
            if (_asciiParser.lastDropReason() == ObisAsciiParser::DropReason::Overflow)
                logWarningP("Daten lesbar, aber kein Telegrammende ('!') innerhalb von %u Bytes: Telegramm zu lang für OPENKNX_SML_BUFFER?", OPENKNX_SML_BUFFER);
            else
                logWarningP("Daten kommen an, aber kein gültiges OBIS-ASCII-Telegramm (%s): Baudrate und Datenformat prüfen, Logarex LK13: 9600 Baud, 7E1. Details mit \"sml debug\"", info);
        }
        return;
    }

    _asciiBadDrops = 0;

    // Complete: Kopie an processFile() übergeben, _buffer gehört gleich wieder dem Parser
    const size_t length = _asciiParser.length();

    lockBuffer();
    if (_asciiTelegram != nullptr)
    {
        // vorheriges, noch nicht verarbeitetes Telegramm verwerfen
        free(_asciiTelegram);
        _asciiTelegram = nullptr;
        _asciiTelegramLen = 0;
    }
    _asciiTelegram = (char *)malloc(length);
    if (_asciiTelegram != nullptr)
    {
        memcpy(_asciiTelegram, _asciiParser.data(), length);
        _asciiTelegramLen = (uint16_t)length;
    }
    else
    {
        logErrorP("OBIS-ASCII telegram dropped (out of memory)");
    }
    unlockBuffer();
}

// Merkt sich den Zeitpunkt eines gültigen Telegramms. Sendet der Zähler seltener, als der
// Empfangs-Timeout erlaubt, springen Status-LED und "Zähler empfängt" zwischen den Telegrammen
// auf aus/rot. Das wird einmal pro Start gemeldet, mit dem gemessenen Abstand.
void SMLChannel::noteValidTelegram()
{
    const uint32_t now = millis();
    if (_lastReceivedFile != 0)
    {
        const uint32_t interval = now - _lastReceivedFile;
        const uint32_t timeout = SMLHelpers::timeoutMsFromParam(ParamSML_cReceiveTimeout);
        if (openknxSMLModule.debug()) logInfoP("Abstand zum letzten Telegramm: %u ms", interval);
        // Lücken über 5 Minuten sind Ausfälle (Lesekopf ab, Neustart), kein Sendeabstand
        if (!_intervalHintLogged && interval >= timeout && interval < 300000)
        {
            logWarningP("Zähler sendet nur alle %u s, Empfangs-Timeout ist %u s: Timeout in der ETS größer einstellen",
                        (unsigned)((interval + 999) / 1000), (unsigned)(timeout / 1000));
            _intervalHintLogged = true;
        }
    }
    _lastReceivedFile = now;
}

// Debug: zeigt, was vor dem Verwerfen im Puffer stand. Lesbare Zeilen sprechen für ein zu langes
// Telegramm, Zeichensalat für falsche Baudrate bzw. falsches Datenformat. Höchstens alle 2 s ein Dump.
void SMLChannel::logAsciiDrop(const char *reason)
{
    if (!openknxSMLModule.debug()) return;

    if (_asciiLastDump != 0 && !delayCheck(_asciiLastDump, 2000))
    {
        if (_asciiSuppressedDumps < UINT16_MAX) _asciiSuppressedDumps++;
        return;
    }
    _asciiLastDump = millis();

    const uint8_t *data = (const uint8_t *)_asciiParser.data();
    const size_t length = _asciiParser.droppedLength();
    const size_t head = length < 32 ? length : 32;
    char dump[4 * 32 + 3];

    logInfoP("OBIS-ASCII verworfen (%s) nach %u Bytes, auslösendes Byte 0x%02X, %u weitere seit dem letzten Dump",
             reason, (unsigned)length, _asciiParser.droppedByte(), _asciiSuppressedDumps);
    _asciiSuppressedDumps = 0;
    logIndentUp();
    SMLHelpers::formatDump(dump, sizeof(dump), data, head);
    logInfoP("Anfang: %s", dump);
    if (length > 32)
    {
        SMLHelpers::formatDump(dump, sizeof(dump), data + length - 32, 32);
        logInfoP("Ende:   %s", dump);
    }
    logIndentDown();
}

// Beginnt die Aufzeichnung mit der Startsequenz an Position 0. Damit liegt der Rahmen
// wieder am Pufferanfang und das 4-Byte-Raster der Escape-Erkennung stimmt.
void SMLChannel::beginCapture(uint16_t start)
{
    if (start > 0) moveBuffer(start);

    _capture = true;
    _escapePending = false;
    _escapeArgPos = SML_NO_ESCAPE_ARG;
}

bool SMLChannel::moveBuffer(uint16_t move)
{
    if (move > _bufferPos)
    {
        logErrorP("moveBuffer ERROR %i / %i", move, _bufferPos);
        return false;
    }
    if (_bufferPos - move > 0)
    {
        memmove(_buffer, _buffer + move, _bufferPos - move);
    }
    _bufferPos -= move;

    return true;
}

void SMLChannel::removeEscaping()
{
    // letzte gültige Vergleichsposition ist _bufferPos - 8, dort müssen noch 8 Byte passen.
    // _bufferPos schrumpft bei jedem Treffer, die Bedingung wird daher je Runde neu geprüft.
    for (size_t i = 0; i + 8 <= _bufferPos;)
    {
        if (memcmp(_buffer + i, SML_ESCAPE, 8) == 0)
        {
            memmove(_buffer + i + 4, _buffer + i + 8, _bufferPos - i - 8);
            _bufferPos -= 4;
            i += 4;
        }
        else
        {
            i++;
        }
    }
}

uint16_t SMLChannel::crc16(uint8_t byte, uint16_t crc)
{
    return pgm_read_word_near(&SML_CRC_TABLE[(byte ^ crc) & 0xff]) ^ (crc >> 8 & 0xff);
}

void SMLChannel::lockBuffer()
{
#ifdef ARDUINO_ARCH_RP2040
    mutex_enter_blocking(&_mutex);
#elif defined(ARDUINO_ARCH_ESP32)
    xSemaphoreTake(_mutex, portMAX_DELAY);
#endif
}

// Nicht blockierend: schlägt der Zugriff fehl, wird die Verarbeitung einfach
// im nächsten Durchlauf erneut versucht.
bool SMLChannel::tryLockBuffer()
{
#ifdef ARDUINO_ARCH_RP2040
    return mutex_try_enter(&_mutex, NULL);
#elif defined(ARDUINO_ARCH_ESP32)
    return xSemaphoreTake(_mutex, 0) == pdTRUE;
#else
    return true;
#endif
}

void SMLChannel::unlockBuffer()
{
#ifdef ARDUINO_ARCH_RP2040
    mutex_exit(&_mutex);
#elif defined(ARDUINO_ARCH_ESP32)
    xSemaphoreGive(_mutex);
#endif
}

void SMLChannel::processFile()
{
    if (_asciiTelegram != nullptr)
    {
        processFileAscii();
        return;
    }

    if (_smlBuffer == NULL) return;

    if (!tryLockBuffer()) return;

    sml_buffer *currentBuffer = _smlBuffer;
    _smlBuffer = NULL;

    unlockBuffer();

    if (openknxSMLModule.debug())
    {
        logInfoP("Valid sml file found (%i size)", currentBuffer->buffer_len);
        logIndentUp();
        logHexDebugP(currentBuffer->buffer, currentBuffer->buffer_len);
    }

    noteValidTelegram();
    _features = {};
    _statusSentThisFile = false;
#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
    mqttBegin();
#endif

    sml_file *file = (sml_file *)malloc(sizeof(sml_file));
    if (file == NULL)
    {
        logErrorP("malloc(sml_file) failed (out of memory)");
        sml_buffer_free(currentBuffer);
        return;
    }
    *file = (sml_file){.messages = NULL, .messages_len = 0, .buf = NULL};
    file->buf = currentBuffer;

    // parsing all messages
    sml_message *msg;
    for (; currentBuffer->cursor < currentBuffer->buffer_len;)
    {

        if (sml_buf_get_current_byte(currentBuffer) == SML_MESSAGE_END)
        {
            // reading trailing zeroed bytes
            sml_buf_update_bytes_read(currentBuffer, 1);
            continue;
        }

        msg = sml_message_parse(currentBuffer);

        if (sml_buf_has_errors(currentBuffer))
        {
            logErrorP("Could not read the whole file!");
            break;
        }

        if (msg) sml_file_add_message(file, msg);
    }

    for (int i = 0; i < file->messages_len; i++)
    {
        sml_message *message = file->messages[i];
        if (*message->message_body->tag == SML_MESSAGE_GET_LIST_RESPONSE)
        {
            sml_list *entry;
            sml_get_list_response *body;
            body = (sml_get_list_response *)message->message_body->data;
            for (entry = body->val_list; entry != NULL; entry = entry->next)
            {
                if (!entry->value)
                { // do not crash on null value
                    continue;
                }
                processDataPoint(entry);
            }
        }
    }

    // free the malloc'd memory
    sml_file_free(file);
    // logHexInfoP(_buffer, messageLen);

    if (openknxSMLModule.debug())
    {
        logIndentDown();
    }

#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
    mqttPublish();
#endif

    // clearCurrentFile();

    return;
}

void SMLChannel::processFileAscii()
{
    if (!tryLockBuffer()) return;

    char *telegram = _asciiTelegram;
    const uint16_t length = _asciiTelegramLen;
    _asciiTelegram = nullptr;
    _asciiTelegramLen = 0;

    unlockBuffer();

    if (telegram == nullptr) return;

    if (openknxSMLModule.debug())
    {
        logInfoP("Valid OBIS-ASCII telegram (%u bytes)", length);
        logIndentUp();
    }

    noteValidTelegram();
    _features = {};
    _statusSentThisFile = false;
#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
    mqttBegin();
#endif

    const ObisAscii::Summary summary = ObisAscii::parseTelegram(telegram, length, &SMLChannel::asciiLineTrampoline, this);

    if (openknxSMLModule.debug())
    {
        logInfoP("Ident: %s", summary.ident);
        logInfoP("%u Zeilen, %u Werte, %u verworfen", summary.lines, summary.entries, summary.dropped);
        logIndentDown();
    }

    free(telegram);

#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
    mqttPublish();
#endif
}

void SMLChannel::asciiLineTrampoline(void *ctx, const char *line, size_t len, ObisAscii::LineKind kind, const ObisAscii::Entry *entry)
{
    static_cast<SMLChannel *>(ctx)->processAsciiLine(line, len, kind, entry);
}

// Ordnet eine ASCII-Zeile den bestehenden processDataPoint()-Zweigen zu. Es gibt kein Statuswort
// im ASCII-Telegramm, KoSML_cMeterStatus wird hier nie berührt.
void SMLChannel::processAsciiLine(const char *line, size_t len, ObisAscii::LineKind kind, const ObisAscii::Entry *entry)
{
    const bool debug = openknxSMLModule.debug();

    if (debug) logInfoP("> %.*s", (int)len, line);

    if (kind != ObisAscii::LineKind::Data || entry == nullptr) return;
    if (!isActive()) return;

    // Buchstabencodes (z. B. "C.90.2") haben keine Entsprechung bei den SML-Werten
    if (entry->letter != '\0')
    {
        if (debug) logInfoP("%c.%u.%u: %s", entry->letter, entry->d, entry->e, entry->value);
        return;
    }

    if (entry->a != 1) return; // nur Stromzähler erlaubt

    char obis[10] = {};
    snprintf(obis, 9, "%02d.%02d.%02d", entry->c, entry->d, entry->e);

    // Vorwerte (F != 255, z. B. 1.8.0*96 … *100) würden sonst den aktuellen Zählerstand überschreiben
    if (!ObisAscii::isCurrentValue(*entry))
    {
        if (debug) logInfoP("%s*%u: %s %s (Vorwert, ignoriert)", obis, entry->f, entry->value, entry->unit);
        return;
    }

    if (entry->c == 96 && entry->d == 1 && entry->e == 0) // Zähler ID
    {
        char identifier[15] = {};
        ObisAscii::trimMeterId(entry->value, strlen(entry->value), identifier, sizeof(identifier));
        publishIdentifier(obis, identifier);
    }
    else if (entry->numeric)
    {
        char unit[ObisAscii::MaxUnitLen + 1] = {};
        const double value = ObisAscii::normalizeUnit(entry->number, entry->unit, unit, sizeof(unit));
        processDataPoint(obis, entry->a, entry->b, entry->c, entry->d, entry->e, entry->f, (double)value, "");
    }
    else if (debug)
    {
        logInfoP("%s: %s", obis, entry->value);
    }
}

#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
void SMLChannel::mqttBegin()
{
    _mqttJson.reset();
    _mqttJson.beginObject();
    _mqttHasData = false;
}

void SMLChannel::mqttAppend(const char *key, float value)
{
    _mqttJson.field(key, value, 3);
    _mqttHasData = true;
}

void SMLChannel::mqttPublish()
{
    if (!_mqttHasData || !openknxNetwork.mqtt.connected()) return;

    _mqttJson.endObject();

    char topic[32];
    if (!_mqttIdentifier.empty())
        snprintf(topic, sizeof(topic), "sml/%s", _mqttIdentifier.c_str());
    else
        snprintf(topic, sizeof(topic), "sml/%c", 'a' + _channelIndex);

    openknxNetwork.mqtt.publishP(topic, _mqttJson.str(), /*qos=*/0, /*retain=*/false);
}
#endif

void SMLChannel::processDataPoint(sml_list_entry *entry)
{
    if (!isActive()) return;

    if (!entry->obj_name || entry->obj_name->len < 6)
    {
        logErrorP("Invalid OBIS field (too short)");
        return;
    }

    const uint8_t a = entry->obj_name->str[0];
    const uint8_t b = entry->obj_name->str[1];
    const uint8_t c = entry->obj_name->str[2];
    const uint8_t d = entry->obj_name->str[3];
    const uint8_t e = entry->obj_name->str[4];
    const uint8_t f = entry->obj_name->str[5];

    char obis[10] = {};
    snprintf(obis, 9, "%02d.%02d.%02d", c, d, e);

    // " (Status: 0x" (12) + 8 Hexstellen + ")" + NUL = 22, etwas Reserve
    char statusSuffix[24] = "";
    if (entry->status)
    {
        uint32_t rawStatus = 0;
        switch (entry->status->type & SML_LENGTH_FIELD)
        {
            case 1: rawStatus = *entry->status->data.status8; break;
            case 2: rawStatus = *entry->status->data.status16; break;
            case 4: rawStatus = *entry->status->data.status32; break;
            case 8: rawStatus = (uint32_t)*entry->status->data.status64; break;
        }

        if (openknxSMLModule.debug()) snprintf(statusSuffix, sizeof(statusSuffix), " (Status: 0x%X)", rawStatus);

        // Pro Telegramm gewinnt der erste Eintrag mit Statusfeld, weitere werden ignoriert.
        if (!_statusSentThisFile)
        {
#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
            _mqttJson.field("status", rawStatus);
            _mqttHasData = true;
#endif
            _statusSentThisFile = true;

            if (knx.configured() && openknx.afterStartupDelay() && ParamSML_cShowMeterStatus)
                KoSML_cMeterStatus.valueCompare(rawStatus, DPT_Value_4_Ucount);
        }
    }

    if (a != 1) return; // nur Stromzähler erlaubt

    if (((entry->value->type & SML_TYPE_FIELD) == SML_TYPE_INTEGER) ||
        ((entry->value->type & SML_TYPE_FIELD) == SML_TYPE_UNSIGNED))
    {

        double converted = sml_value_to_double(entry->value);

        int scaler = (entry->scaler) ? *entry->scaler : 0;
        int prec = -scaler;
        if (prec < 0) prec = 0;
        converted = converted * pow(10, scaler);

        processDataPoint(obis, a, b, c, d, e, f, (double)converted, statusSuffix);
    }
    else if (entry->value->type == SML_TYPE_OCTET_STRING)
    {
        processDataPoint(obis, a, b, c, d, e, f, (char *)entry->value->data.bytes->str, entry->value->data.bytes->len);
    }
    else if (entry->value->type == SML_TYPE_BOOLEAN)
    {
        processDataPoint(obis, a, b, c, d, e, f, (bool)entry->value->data.boolean, statusSuffix);
    }
}

void SMLChannel::processDataPoint(char *obis, const uint8_t &a, const uint8_t &b, const uint8_t &c, const uint8_t &d, const uint8_t &e, const uint8_t &f, bool value, const char *statusSuffix)
{
    if (openknxSMLModule.debug())
        logInfoP("%s: %s%s", obis, (value ? "true" : "false"), statusSuffix);
}

void SMLChannel::processDataPoint(char *obis, const uint8_t &a, const uint8_t &b, const uint8_t &c, const uint8_t &d, const uint8_t &e, const uint8_t &f, double value, const char *statusSuffix)
{
    bool send = false;
    if (d == 8)
    {
        double counter = value;
        int64_t counterKwh = counter / 1000;
        int64_t counterWh = counter;
        if (openknxSMLModule.debug()) logInfoP("%s: %.3f kWh%s", obis, counter / 1000, statusSuffix);

        if (c == 1 && e > _features.maxTariff) _features.maxTariff = e;
        else if (c == 2 && e == 0) _features.bidirectional = true;

#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
        if (c == 1 && e == 0) mqttAppend("energy_in", counter / 1000.0f);
        else if (c == 1 && e == 1) mqttAppend("energy_in_t1", counter / 1000.0f);
        else if (c == 1 && e == 2) mqttAppend("energy_in_t2", counter / 1000.0f);
        else if (c == 2 && e == 0) mqttAppend("energy_out", counter / 1000.0f);
        else if (c == 2 && e == 1) mqttAppend("energy_out_t1", counter / 1000.0f);
        else if (c == 2 && e == 2) mqttAppend("energy_out_t2", counter / 1000.0f);
#endif

        // kWh- und Wh-Zählerstände sind unabhängig schaltbar, deshalb hier nur die Oder-Verknüpfung
        if (knx.configured() && openknx.afterStartupDelay() && ParamSML_cType && (ParamSML_cCounter || ParamSML_cCounterWh))
        {
            if (c == 1 && e == 0)
            {
                if (ParamSML_cCounter)
                {
                    if (ParamSML_cCounterChange && llabs(_sentCounterIn - counterKwh) >= ParamSML_cCounterChangeV)
                        send = true;

                    if (ParamSML_cCounterCyclic && (!_sentCounterInTime || delayCheck(_sentCounterInTime, (ParamSML_cCounterCyclicTimeMS - CorrectionOfCycleTimeMS))))
                        send = true;

                    if (send)
                    {
                        _sentCounterIn = counterKwh;
                        _sentCounterInTime = millis();
                        KoSML_cCounterF1In.value(counterKwh, DPT_ActiveEnergy_kWh);
                    }
                    else
                    {
                        KoSML_cCounterF1In.valueNoSend(counterKwh, DPT_ActiveEnergy_kWh);
                    }
                }

                if (ParamSML_cCounterWh)
                    KoSML_cCounterF2In.valueCompare(counterWh, DPT_ActiveEnergy);
            }
            else if (c == 1 && e == 1 && (ParamSML_cType == 2 || ParamSML_cType == 4))
            {
                if (ParamSML_cCounter)
                {
                    if (ParamSML_cCounterChange && llabs(_sentCounterInT1 - counterKwh) >= ParamSML_cCounterChangeV)
                        send = true;

                    if (ParamSML_cCounterCyclic && (!_sentCounterInT1Time || delayCheck(_sentCounterInT1Time, (ParamSML_cCounterCyclicTimeMS - CorrectionOfCycleTimeMS))))
                        send = true;

                    if (send)
                    {
                        _sentCounterInT1 = counterKwh;
                        _sentCounterInT1Time = millis();
                        KoSML_cCounterF1InT1.value(counterKwh, DPT_ActiveEnergy_kWh);
                    }
                    else
                    {
                        KoSML_cCounterF1InT1.valueNoSend(counterKwh, DPT_ActiveEnergy_kWh);
                    }
                }

                if (ParamSML_cCounterWh)
                    KoSML_cCounterF2InT1.valueCompare(counterWh, DPT_ActiveEnergy);
            }
            else if (c == 1 && e == 2 && (ParamSML_cType == 2 || ParamSML_cType == 4))
            {
                if (ParamSML_cCounter)
                {
                    if (ParamSML_cCounterChange && llabs(_sentCounterInT2 - counterKwh) >= ParamSML_cCounterChangeV)
                        send = true;

                    if (ParamSML_cCounterCyclic && (!_sentCounterInT2Time || delayCheck(_sentCounterInT2Time, (ParamSML_cCounterCyclicTimeMS - CorrectionOfCycleTimeMS))))
                        send = true;

                    if (send)
                    {
                        _sentCounterInT2 = counterKwh;
                        _sentCounterInT2Time = millis();
                        KoSML_cCounterF1InT2.value(counterKwh, DPT_ActiveEnergy_kWh);
                    }
                    else
                    {
                        KoSML_cCounterF1InT2.valueNoSend(counterKwh, DPT_ActiveEnergy_kWh);
                    }
                }

                if (ParamSML_cCounterWh)
                    KoSML_cCounterF2InT2.valueCompare(counterWh, DPT_ActiveEnergy);
            }
            else if (c == 2 && e == 0 && (ParamSML_cType == 3 || ParamSML_cType == 4))
            {
                if (ParamSML_cCounter)
                {
                    if (ParamSML_cCounterChange && llabs(_sentCounterOut - counterKwh) >= ParamSML_cCounterChangeV)
                        send = true;

                    if (ParamSML_cCounterCyclic && (!_sentCounterOutTime || delayCheck(_sentCounterOutTime, (ParamSML_cCounterCyclicTimeMS - CorrectionOfCycleTimeMS))))
                        send = true;

                    if (send)
                    {
                        _sentCounterOut = counterKwh;
                        _sentCounterOutTime = millis();
                        KoSML_cCounterF1Out.value(counterKwh, DPT_ActiveEnergy_kWh);
                    }
                    else
                    {
                        KoSML_cCounterF1Out.valueNoSend(counterKwh, DPT_ActiveEnergy_kWh);
                    }
                }

                if (ParamSML_cCounterWh)
                    KoSML_cCounterF2Out.valueCompare(counterWh, DPT_ActiveEnergy);
            }
            else if (c == 2 && e == 1 && ParamSML_cType == 4)
            {
                if (ParamSML_cCounter)
                {
                    if (ParamSML_cCounterChange && llabs(_sentCounterOutT1 - counterKwh) >= ParamSML_cCounterChangeV)
                        send = true;

                    if (ParamSML_cCounterCyclic && (!_sentCounterOutT1Time || delayCheck(_sentCounterOutT1Time, (ParamSML_cCounterCyclicTimeMS - CorrectionOfCycleTimeMS))))
                        send = true;

                    if (send)
                    {
                        _sentCounterOutT1 = counterKwh;
                        _sentCounterOutT1Time = millis();
                        KoSML_cCounterF1OutT1.value(counterKwh, DPT_ActiveEnergy_kWh);
                    }
                    else
                    {
                        KoSML_cCounterF1OutT1.valueNoSend(counterKwh, DPT_ActiveEnergy_kWh);
                    }
                }

                if (ParamSML_cCounterWh)
                    KoSML_cCounterF2OutT1.valueCompare(counterWh, DPT_ActiveEnergy);
            }
            else if (c == 2 && e == 2 && ParamSML_cType == 4)
            {
                if (ParamSML_cCounter)
                {
                    if (ParamSML_cCounterChange && llabs(_sentCounterOutT2 - counterKwh) >= ParamSML_cCounterChangeV)
                        send = true;

                    if (ParamSML_cCounterCyclic && (!_sentCounterOutT2Time || delayCheck(_sentCounterOutT2Time, (ParamSML_cCounterCyclicTimeMS - CorrectionOfCycleTimeMS))))
                        send = true;

                    if (send)
                    {
                        _sentCounterOutT2 = counterKwh;
                        _sentCounterOutT2Time = millis();
                        KoSML_cCounterF1OutT2.value(counterKwh, DPT_ActiveEnergy_kWh);
                    }
                    else
                    {
                        KoSML_cCounterF1OutT2.valueNoSend(counterKwh, DPT_ActiveEnergy_kWh);
                    }
                }

                if (ParamSML_cCounterWh)
                    KoSML_cCounterF2OutT2.valueCompare(counterWh, DPT_ActiveEnergy);
            }
        }
    }
    else if (d == 7 && (c == 16 || c == 36 || c == 56 || c == 76))
    {
        if (openknxSMLModule.debug()) logInfoP("%s: %i Watt%s", obis, (int)value, statusSuffix);

        _features.power = true;

#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
        if (c == 16) mqttAppend("power", value);
        else if (c == 36) mqttAppend("power_l1", value);
        else if (c == 56) mqttAppend("power_l2", value);
        else if (c == 76) mqttAppend("power_l3", value);
#endif

        if (knx.configured() && openknx.afterStartupDelay())
        {
            if (c == 16 && ParamSML_cPowerSum)
            {
                if (ParamSML_cPowerSumChange && fabs(_sentDataPower - value) >= ParamSML_cPowerSumChangeV)
                    send = true;

                if (ParamSML_cPowerSumCyclic && (!_sentDataPowerTime || delayCheck(_sentDataPowerTime, ParamSML_cPowerSumCyclicTimeMS - CorrectionOfCycleTimeMS)))
                    send = true;

                if (send)
                {
                    _sentDataPower = value;
                    _sentDataPowerTime = millis();
                    KoSML_cPower.value(value, DPT_Value_Power);
                }
                else
                {
                    KoSML_cPower.valueNoSend(value, DPT_Value_Power);
                }
            }
            else if (c == 36 && ParamSML_cPower)
            {
                if (ParamSML_cPowerChange && fabs(_sentDataPowerL1 - value) >= ParamSML_cPowerChangeV)
                    send = true;

                if (ParamSML_cPowerCyclic && (!_sentDataPowerL1Time || delayCheck(_sentDataPowerL1Time, ParamSML_cPowerCyclicTimeMS - CorrectionOfCycleTimeMS)))
                    send = true;

                if (send)
                {
                    _sentDataPowerL1 = value;
                    _sentDataPowerL1Time = millis();
                    KoSML_cPowerL1.value(value, DPT_Value_Power);
                }
                else
                {
                    KoSML_cPowerL1.valueNoSend(value, DPT_Value_Power);
                }
            }
            else if (c == 56 && ParamSML_cPower)
            {
                if (ParamSML_cPowerChange && fabs(_sentDataPowerL2 - value) >= ParamSML_cPowerChangeV)
                    send = true;

                if (ParamSML_cPowerCyclic && (!_sentDataPowerL2Time || delayCheck(_sentDataPowerL2Time, ParamSML_cPowerCyclicTimeMS - CorrectionOfCycleTimeMS)))
                    send = true;

                if (send)
                {
                    _sentDataPowerL2 = value;
                    _sentDataPowerL2Time = millis();
                    KoSML_cPowerL2.value(value, DPT_Value_Power);
                }
                else
                {
                    KoSML_cPowerL2.valueNoSend(value, DPT_Value_Power);
                }
            }
            else if (c == 76 && ParamSML_cPower)
            {
                if (ParamSML_cPowerChange && fabs(_sentDataPowerL3 - value) >= ParamSML_cPowerChangeV)
                    send = true;

                if (ParamSML_cPowerCyclic && (!_sentDataPowerL3Time || delayCheck(_sentDataPowerL3Time, ParamSML_cPowerCyclicTimeMS - CorrectionOfCycleTimeMS)))
                    send = true;

                if (send)
                {
                    _sentDataPowerL3 = value;
                    _sentDataPowerL3Time = millis();
                    KoSML_cPowerL3.value(value, DPT_Value_Power);
                }
                else
                {
                    KoSML_cPowerL3.valueNoSend(value, DPT_Value_Power);
                }
            }
        }
    }
    else if (d == 7 && (c == 31 || c == 51 || c == 71))
    {
        if (openknxSMLModule.debug()) logInfoP("%s: %.2f Ampere%s", obis, value, statusSuffix);

        _features.current = true;

#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
        if (c == 31) mqttAppend("current_l1", value);
        else if (c == 51) mqttAppend("current_l2", value);
        else if (c == 71) mqttAppend("current_l3", value);
#endif

        if (knx.configured() && openknx.afterStartupDelay() && ParamSML_cCurrent)
        {
            if (c == 31)
            {
                if (ParamSML_cCurrentChange && fabs(_sentDataCurrentL1 - value) >= (double)ParamSML_cCurrentChangeV / 10)
                    send = true;

                if (ParamSML_cCurrentCyclic && (!_sentDataCurrentL1Time || delayCheck(_sentDataCurrentL1Time, ParamSML_cCurrentCyclicTimeMS - CorrectionOfCycleTimeMS)))
                    send = true;

                if (send)
                {
                    _sentDataCurrentL1 = value;
                    _sentDataCurrentL1Time = millis();
                    KoSML_cCurrentL1.value(value, DPT_Value_Electric_Current);
                }
                else
                {
                    KoSML_cCurrentL1.valueNoSend(value, DPT_Value_Electric_Current);
                }
            }
            else if (c == 51)
            {
                if (ParamSML_cCurrentChange && fabs(_sentDataCurrentL2 - value) >= (double)ParamSML_cCurrentChangeV / 10)
                    send = true;

                if (ParamSML_cCurrentCyclic && (!_sentDataCurrentL2Time || delayCheck(_sentDataCurrentL2Time, ParamSML_cCurrentCyclicTimeMS - CorrectionOfCycleTimeMS)))
                    send = true;

                if (send)
                {
                    _sentDataCurrentL2 = value;
                    _sentDataCurrentL2Time = millis();
                    KoSML_cCurrentL2.value(value, DPT_Value_Electric_Current);
                }
                else
                {
                    KoSML_cCurrentL2.valueNoSend(value, DPT_Value_Electric_Current);
                }
            }
            else if (c == 71)
            {
                if (ParamSML_cCurrentChange && fabs(_sentDataCurrentL3 - value) >= (double)ParamSML_cCurrentChangeV / 10)
                    send = true;

                if (ParamSML_cCurrentCyclic && (!_sentDataCurrentL3Time || delayCheck(_sentDataCurrentL3Time, ParamSML_cCurrentCyclicTimeMS - CorrectionOfCycleTimeMS)))
                    send = true;

                if (send)
                {
                    _sentDataCurrentL3 = value;
                    _sentDataCurrentL3Time = millis();
                    KoSML_cCurrentL3.value(value, DPT_Value_Electric_Current);
                }
                else
                {
                    KoSML_cCurrentL3.valueNoSend(value, DPT_Value_Electric_Current);
                }
            }
        }
    }
    else if (d == 7 && (c == 32 || c == 52 || c == 72))
    {
        if (openknxSMLModule.debug()) logInfoP("%s: %.1f Volt%s", obis, value, statusSuffix);

        _features.voltage = true;

#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
        if (c == 32) mqttAppend("voltage_l1", value);
        else if (c == 52) mqttAppend("voltage_l2", value);
        else if (c == 72) mqttAppend("voltage_l3", value);
#endif

        if (knx.configured() && openknx.afterStartupDelay() && ParamSML_cVoltage)
        {
            if (c == 32)
            {
                if (ParamSML_cVoltageChange && fabs(_sentDataVoltageL1 - value) >= (double)ParamSML_cVoltageChangeV / 10)
                    send = true;

                if (ParamSML_cVoltageCyclic && (!_sentDataVoltageL1Time || delayCheck(_sentDataVoltageL1Time, (ParamSML_cVoltageCyclicTimeMS - CorrectionOfCycleTimeMS))))
                    send = true;

                if (send)
                {
                    _sentDataVoltageL1 = value;
                    _sentDataVoltageL1Time = millis();
                    KoSML_cVoltageL1.value(value, DPT_Value_Electric_Potential);
                }
                else
                {
                    KoSML_cVoltageL1.valueNoSend(value, DPT_Value_Electric_Potential);
                }
            }
            else if (c == 52)
            {
                if (ParamSML_cVoltageChange && fabs(_sentDataVoltageL2 - value) >= (double)ParamSML_cVoltageChangeV / 10)
                    send = true;

                if (ParamSML_cVoltageCyclic && (!_sentDataVoltageL2Time || delayCheck(_sentDataVoltageL2Time, (ParamSML_cVoltageCyclicTimeMS - CorrectionOfCycleTimeMS))))
                    send = true;

                if (send)
                {
                    _sentDataVoltageL2 = value;
                    _sentDataVoltageL2Time = millis();
                    KoSML_cVoltageL2.value(value, DPT_Value_Electric_Potential);
                }
                else
                {
                    KoSML_cVoltageL2.valueNoSend(value, DPT_Value_Electric_Potential);
                }
            }
            else if (c == 72)
            {
                if (ParamSML_cVoltageChange && fabs(_sentDataVoltageL3 - value) >= (double)ParamSML_cVoltageChangeV / 10)
                    send = true;

                if (ParamSML_cVoltageCyclic && (!_sentDataVoltageL3Time || delayCheck(_sentDataVoltageL3Time, (ParamSML_cVoltageCyclicTimeMS - CorrectionOfCycleTimeMS))))
                    send = true;

                if (send)
                {
                    _sentDataVoltageL3 = value;
                    _sentDataVoltageL3Time = millis();
                    KoSML_cVoltageL3.value(value, DPT_Value_Electric_Potential);
                }
                else
                {
                    KoSML_cVoltageL3.valueNoSend(value, DPT_Value_Electric_Potential);
                }
            }
        }
    }
    else if (c == 14 && d == 7)
    {
        if (openknxSMLModule.debug()) logInfoP("%s: %.1f Hertz%s", obis, value, statusSuffix);

        _features.frequency = true;

#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
        mqttAppend("frequency", value);
#endif

        if (knx.configured() && openknx.afterStartupDelay() && ParamSML_cFrequency)
        {
            if (ParamSML_cFrequencyChange && fabs(_sentDataFrequency - value) >= (double)ParamSML_cFrequencyChangeV / 10)
                send = true;

            if (ParamSML_cFrequencyCyclic && (!_sentDataFrequencyTime || delayCheck(_sentDataFrequencyTime, (ParamSML_cFrequencyCyclicTimeMS - CorrectionOfCycleTimeMS))))
                send = true;

            if (send)
            {
                _sentDataFrequency = value;
                _sentDataFrequencyTime = millis();
                KoSML_cFrequency.value(value, DPT_Value_Frequency);
            }
            else
            {
                KoSML_cFrequency.valueNoSend(value, DPT_Value_Frequency);
            }
        }
    }
    else if (c == 81 && d == 7)
    {
        uint16_t degree = value;
        if (openknxSMLModule.debug()) logInfoP("%s: %i °%s", obis, degree, statusSuffix);
    }
    else
    {
        if (openknxSMLModule.debug()) logInfoP("%s: %lf%s", obis, value, statusSuffix);
    }
}

void SMLChannel::processDataPoint(char *obis, const uint8_t &a, const uint8_t &b, const uint8_t &c, const uint8_t &d, const uint8_t &e, const uint8_t &f, char *value, uint8_t len)
{
    if (c == 96 && d == 50 && e == 1) // Hersteller
    {
        if (len < 3)
        {
            logErrorP("Invalid vendor field (len %u < 3)", len);
            return;
        }

        char vendor[4];
        sprintf(vendor, "%c%c%c", value[0], value[1], value[2]);

        if (openknxSMLModule.debug()) logInfoP("%s: %s", obis, vendor);
    }
    else if (c == 96 && d == 1 && e == 0) // Zähler ID
    {
        if (len < 10)
        {
            logErrorP("Invalid meter id field (len %u < 10)", len);
            return;
        }

        // Feste Breite nach DIN 43863-5: 1 Ziffer Sparte, 3 Zeichen Hersteller, 2 Ziffern
        // Fabrikationsblock, 8 Ziffern Fabrikationsnummer -> 14 Zeichen, wie DPT 16.001.
        // Die Feldbreiten sind Mindestbreiten: ein Zähler außerhalb der Spec (z. B. eine
        // Fabrikationsnummer > 99999999) erzeugt mehr Zeichen, snprintf schneidet dann ab.
        char identifier[15] = {};
        snprintf(identifier, sizeof(identifier), "%u%c%c%c%02u%08u",
                 (unsigned)(uint8_t)value[1],  // Sparte
                 value[2], value[3], value[4], // Hersteller
                 (unsigned)(uint8_t)value[5],  // Fabrikationsblock
                 (uint32_t)(uint8_t)value[6] << 24 | (uint32_t)(uint8_t)value[7] << 16 |
                     (uint32_t)(uint8_t)value[8] << 8 | (uint32_t)(uint8_t)value[9]); // Frabrikationsnummer

        publishIdentifier(obis, identifier);
    }
    else
    {
        if (openknxSMLModule.debug())
        {
            // value ist nicht nullterminiert, len passt als uint8_t immer in den Puffer.
            // Die Nullung stellt sicher, dass %s auf jeden Fall terminiert liest.
            char text[256] = {};
            memcpy(text, value, len);
            logInfoP("%s: %s", obis, text);
        }
    }
}

// Gemeinsam für SML und OBIS-ASCII: Zähler-ID ins Log, in das MQTT-Topic und auf das KO
void SMLChannel::publishIdentifier(const char *obis, const char *identifier)
{
    if (openknxSMLModule.debug()) logInfoP("%s: %s", obis, identifier);

#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
    _mqttIdentifier = identifier;
#endif

    if (knx.configured() && openknx.afterStartupDelay() && ParamSML_cIdentifikation)
    {
        if (KoSML_cIdentifier.initialized())
            KoSML_cIdentifier.valueNoSend(identifier, DPT_String_8859_1);
        else
            KoSML_cIdentifier.value(identifier, DPT_String_8859_1);
    }
}
