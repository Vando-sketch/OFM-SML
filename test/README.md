# Host-Unit-Tests

Tests für hardwareunabhängigen Code (z. B. `src/ObisAsciiParser.cpp`), der auf dem PC
läuft statt auf dem Mikrocontroller. Eingesetzt wird PlatformIO (`native`) mit Unity.
`SMLChannel.cpp` und `SMLModule.cpp` werden bewusst **nie** kompiliert (Arduino/OpenKNX/libsml).

## Voraussetzungen

- Python 3 und PlatformIO (`pip install platformio`, am besten in einem venv)
- C++-Compiler (`g++`/`clang++`); Unity lädt PlatformIO beim ersten Lauf selbst nach

## Tests ausführen

```sh
test/run.sh
```

Entspricht `cd test/native && pio test -e native -v`. Ist `pio` nicht im `PATH`,
`PIO=/pfad/zu/pio test/run.sh` setzen. Ausgabe: Ergebnis pro Test und eine
Zusammenfassung wie `N Tests M Failures`; bei Fehlern ist der Exit-Code ungleich 0.

## Tests hinzufügen

1. Neues Verzeichnis `test/native/test/test_<name>/` mit einer `test_main.cpp` anlegen
   (siehe `test_sanity` als Vorlage: `setUp`/`tearDown`, `RUN_TEST(...)`, `main`).
2. Mit `#include "ObisAsciiParser.h"` den Code aus `src/` einbinden (`-I../../src` ist gesetzt).
3. `test/run.sh` starten. Jedes `test_*`-Verzeichnis ist eine eigene Test-Suite.

`src/ObisAsciiParser.cpp` wird automatisch mitkompiliert, sobald die Datei existiert
(`build_src_filter` in `test/native/platformio.ini`). Weitere Quelldateien ohne
Arduino-Abhängigkeit dort ergänzen, z. B. `+<Weitere.cpp>`.
