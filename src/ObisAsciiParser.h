#pragma once
// Parser für OBIS-ASCII-Telegramme (IEC 62056-21 Mode D / "Logarex"-Stil) von Stromzählern.
// Hardwareunabhängig: kein Arduino, keine Exceptions, keine Heap-Allokation.
#include <cstddef>
#include <cstdint>

#ifndef OPENKNX_SML_BUFFER
    #define OPENKNX_SML_BUFFER 1024
#endif

namespace ObisAscii
{
    // Maximale Länge des Wertes (Text zwischen '(' und '*' bzw. ')') ohne NUL.
    constexpr size_t MaxValueLen = 40;
    // Maximale Länge der Einheit ohne NUL.
    constexpr size_t MaxUnitLen = 8;
    // Maximale Länge einer Zeile (ohne CR/LF); längere Zeilen sind Invalid.
    constexpr size_t MaxLineLen = 96;

    // Art einer Zeile.
    //  Empty   - leere Zeile (Länge 0)
    //  Ident   - beginnt mit '/' (Identifikationszeile)
    //  End     - genau "!" (Telegrammende)
    //  Data    - gültige OBIS-Datenzeile
    //  Invalid - alles andere
    enum class LineKind : uint8_t
    {
        Empty,
        Ident,
        End,
        Data,
        Invalid
    };

    // Ergebnis einer geparsten Datenzeile.
    struct Entry
    {
        // OBIS-Gruppen A-B:C.D.E*F, jeweils 0..255. Kurzform C.D.E: a=1, b=0.
        // Fehlt F, ist f=255. Bei Buchstabencode in Gruppe C ist c=0.
        uint8_t a, b, c, d, e, f;
        // Buchstabe in Gruppe C (so wie gesendet, z. B. 'C' bei "C.90.2"), sonst '\0'.
        char letter;
        // Wert (Text zwischen '(' und '*' bzw. ')'), NUL-terminiert.
        char value[MaxValueLen + 1];
        // Einheit (Text hinter dem LETZTEN '*' im Wert), NUL-terminiert, sonst leer.
        char unit[MaxUnitLen + 1];
        // true, wenn der Wert die Form [+-]?Ziffern(.Ziffern)? hat.
        bool numeric;
        // Zahlenwert, falls numeric; sonst 0.
        double number;
    };

    // Parst eine einzelne Zeile OHNE CR/LF.
    //  len 0 -> Empty; beginnt mit '/' -> Ident; genau "!" -> End.
    //  Datenformen: A-B:C.D.E*F(wert*einheit), A-B:C.D.E(wert), Kurzform C.D.E(...) und C.D.E*F(...).
    //  Gruppen > 255, fehlendes '(' oder ')', zu lange Werte/Einheiten/Zeilen sowie
    //  Buchstaben außerhalb von Gruppe C -> Invalid.
    //  Text nach der ersten ')' wird ignoriert (z. B. eine zweite (...)-Gruppe).
    //  Nur bei Data wird `out` befüllt.
    LineKind parseLine(const char *line, size_t len, Entry &out);

    // Zusammenfassung eines geparsten Telegramms.
    struct Summary
    {
        // Ident-Zeile ohne führendes '/', NUL-terminiert, auf MaxLineLen gekürzt.
        char ident[MaxLineLen + 1];
        // Anzahl nicht-leerer Zeilen.
        uint16_t lines;
        // Anzahl Data-Zeilen.
        uint16_t entries;
        // Anzahl Invalid-Zeilen.
        uint16_t dropped;
    };

    // Wird für jede nicht-leere Zeile aufgerufen. `entry` ist nur bei kind==Data nicht null.
    typedef void (*LineCallback)(void *ctx, const char *line, size_t len, LineKind kind, const Entry *entry);

    // Zerlegt ein komplettes Telegramm an '\n' (ein abschließendes '\r' wird je Zeile entfernt).
    // Leere Zeilen werden übersprungen (kein Callback, nicht gezählt).
    // Eine fehlerhafte Zeile verwirft den Rest des Telegramms nicht. `cb` darf nullptr sein.
    Summary parseTelegram(const char *text, size_t len, LineCallback cb, void *ctx);

    // Rechnet kWh -> Wh und kW -> W um (Groß-/Kleinschreibung egal). Alle anderen Einheiten
    // bleiben unverändert. outUnit wird immer NUL-terminiert und auf outUnitSize-1 gekürzt.
    double normalizeUnit(double value, const char *unit, char *outUnit, size_t outUnitSize);

    // Entfernt führende '0', solange die Restlänge > 14 ist, und kopiert das Ergebnis
    // (NUL-terminiert, auf outSize-1 gekürzt). Rückgabe: Anzahl geschriebener Zeichen.
    size_t trimMeterId(const char *in, size_t len, char *out, size_t outSize);
} // namespace ObisAscii

// Byteweiser Rahmenparser für OBIS-ASCII-Telegramme (von '/' bis '!', optional mit STX/ETX/BCC).
// Der Puffer wird vom Aufrufer gestellt; der Parser besitzt keinen eigenen Speicher.
//
// Regeln:
//  - Jedes Eingangsbyte wird zuerst mit & 0x7F maskiert (7E1-Daten als 8N1 gelesen).
//  - Idle: alle Bytes werden ignoriert, bis '/' kommt; dieses '/' wird gespeichert.
//  - Im Telegramm werden alle Bytes gespeichert (inkl. CR/LF) außer STX (0x02): STX wird nicht
//    gespeichert, markiert nur "BCC erwartet" und startet die BCC-Summe.
//  - Resync: ein '/' am Zeilenanfang (vorheriges gespeichertes Byte ist '\n') während eines
//    laufenden Telegramms -> Dropped/Resync; dieses '/' beginnt das neue Telegramm.
//  - Ende: ein '!' am Zeilenanfang wird gespeichert. Ohne STX -> sofort Complete (bccChecked()==false).
//  - Mit STX: nach '!' dürfen nur CR, LF und ETX (0x03) folgen (nicht gespeichert); das Byte nach ETX
//    ist das BCC = XOR aller (maskierten) Bytes nach STX bis einschließlich ETX.
//    Treffer -> Complete (bccChecked()==true), sonst Dropped/BccMismatch.
//    Anderes Byte zwischen '!' und ETX -> Dropped/Framing ('/' dort -> Dropped/Resync + Neustart).
//  - Overflow: würde ein Byte die Kapazität überschreiten -> Dropped/Overflow, zurück nach Idle.
//  - Garbage: ein Steuerzeichen im Telegramm (alles unter 0x20 außer CR, LF, TAB, STX sowie 0x7F, nach dem
//    Maskieren) -> sofort Dropped/Garbage. Typisch für falsche Baudrate oder einen SML-Zähler; ohne diese Regel
//    würde ein zufälliges '/' im Rauschen erst nach `capacity` Bytes per Overflow verworfen.
//  - Nach Dropped/Overflow bzw. Dropped/Garbage stehen die bis dahin gesammelten Bytes für einen Debug-Dump noch
//    in data() (Länge droppedLength()), bis das nächste '/' kommt.
//  - Ein abgebrochenes Telegramm (kein '!', kein neues '/') liefert nie Complete.
//  - Nach Complete/Dropped ist der Parser wieder Idle; data()/length() eines Complete bleiben
//    bis zum nächsten feed() gültig.
class ObisAsciiParser
{
  public:
    enum class Result : uint8_t
    {
        None,
        Complete,
        Dropped
    };
    enum class DropReason : uint8_t
    {
        None,
        Overflow,
        Resync,
        BccMismatch,
        Framing,
        Garbage
    };

    // storage/capacity: vom Aufrufer gestellter Puffer für das Telegramm.
    ObisAsciiParser(char *storage, size_t capacity);

    // Verarbeitet ein Byte. Result::Complete bzw. Dropped nur im Moment des Abschlusses.
    Result feed(uint8_t byte);

    // Telegramm von '/' bis '!' einschließlich (maskiert, ohne STX/ETX/BCC).
    const char *data() const;
    size_t length() const;

    // Grund des letzten Dropped (None nach reset()).
    DropReason lastDropReason() const;

    // Anzahl der Bytes, die beim letzten Dropped in data() standen (für den Debug-Dump).
    size_t droppedLength() const;

    // Unmaskiertes Byte, das das letzte Dropped/Garbage ausgelöst hat.
    uint8_t droppedByte() const;

    // true, wenn das letzte Complete per BCC geprüft wurde.
    bool bccChecked() const;

    // Zurück nach Idle, length 0, Drop-Grund None.
    void reset();

  private:
    enum class State : uint8_t
    {
        Idle,
        Body,
        Tail,
        Bcc
    };

    // Zustandsfunktionen (je Zustand eine, feed() verzweigt nur).
    Result feedBody(char c);
    Result feedTail(char c);
    Result feedBcc(char c);
    // Beginnt ein neues Telegramm mit '/'. false, wenn der Puffer keinen Platz dafür hat.
    bool beginTelegram();
    // Verwirft das laufende Telegramm: Grund merken, zurück nach Idle, Länge 0.
    Result drop(DropReason reason);
    // '/' am Zeilenanfang im laufenden Telegramm: Dropped/Resync, das '/' startet ein neues Telegramm.
    Result resync();
    // Telegramm vollständig: zurück nach Idle, Daten bleiben erhalten.
    Result complete(bool bccChecked);

    char *_storage;
    size_t _capacity;
    size_t _length;
    State _state;
    bool _stxSeen;
    uint8_t _bcc;
    DropReason _lastDrop;
    bool _bccChecked;
    size_t _droppedLength;
    uint8_t _rawByte;
    uint8_t _droppedByte;
};
