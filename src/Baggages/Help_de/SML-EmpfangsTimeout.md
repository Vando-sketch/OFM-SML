### Empfangs-Timeout

Zeit in Sekunden, nach der ein Kanal als "empfängt nicht" gilt, wenn bis dahin kein gültiges Telegramm angekommen ist. Danach wechselt die Kanal-Status-LED auf rot bzw. aus, und **Zähler empfängt** sendet 0. Der Standardwert von 5 Sekunden passt für Zähler, die etwa jede Sekunde senden. Sendet dein Zähler seltener, erhöhe den Wert, sonst springt der Status zwischen den Telegrammen hin und her.
