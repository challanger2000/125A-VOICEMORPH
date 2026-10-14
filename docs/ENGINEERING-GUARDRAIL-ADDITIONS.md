# Ergänzungen zu zentralen Engineering-Guardrails

Diese aus den VoiceMorph-Reparaturen abgeleiteten Regeln ergänzen die gelesenen zentralen Vorgaben. Diese Datei ist ein konkreter Vorschlag für `125A-Engineering/ENGINEERING-GUARDRAILS.md`; das zentrale Repository wurde hier nicht verändert.

1. VST3-Namen im SDK-Paketziel festlegen. Keine Reparatur des Namens ausschließlich durch spätere Umbenennung des äußeren Bundles. Architekturmodul und Bundle gemeinsam prüfen.
2. QA des endgültigen installierbaren Pakets mit Commit-/Binary-Provenienz ausführen. Alte Artefakte und alte PASS-Berichte sind kein Nachweis für den aktuellen Quellstand. Leere oder wiederverwendete Reportverzeichnisse dürfen keinen PASS erzeugen.
3. Assertions in allen Release-Testzielen explizit aktivieren. Ein erwarteter Negativtest muss bestätigen, dass die Prüfungen tatsächlich auslösen. Produktionsziele behalten ihre normalen Release-Flags.
4. Retired-Objekte auch ohne weiteren Benutzerauftrag auf dem Control-/Worker-Thread freigeben. Audio-Veröffentlichung, Verbrauch und Freigabe besitzen klar getrennte Eigentümerschaft.
5. Projektzustände samt IRs vollständig validieren und vorbereiten, bevor sie als gemeinsame Transaktion angenommen werden. Inaktive Setup-/Sampleratenwechsel dürfen weder kalte erste Offline-Blöcke noch verlorene laufende Benutzeraufträge erzeugen.
6. IR-Sampleratenkonvertierung muss Transferpegel berücksichtigen. Keine pauschale Peak-Normalisierung von Farb-IRs als Ersatz für korrekte Tap-Dichte-Skalierung. Grenzen eines endlichen SRC-Kernels dokumentieren.
7. GUI-, Validator-, DSP- und echte Host-Nachweise getrennt benennen. Ein vorbereiteter Windows-Test ist kein bestandenes Windows-Ergebnis. Ein synthetischer Referenzvergleich ersetzt kein pegelgleiches Hören mit der Nutzerstimme.

8. SDK 3.8.1 ersetzt bei Visual-Studio-Ressourcenkopien den Paketnamen durch `$(TargetFileName)` ohne VERBATIM. Bei Namen mit Leerzeichen kann das Kopierziel in Argumente zerfallen. Eigene dependency-basierte CMake-Ressourcenkopien müssen die vollständigen Pfade mit VERBATIM erhalten; Ressourcen müssen vor SDK-Metadaten und Validator verfügbar sein. Nachgewiesen in Windows-Run 38635144881.

9. Bei CTest-Aufrufen mit SDK-Target-Paketpfaden deren enthaltene Konfigurationsausdrücke über TARGET_GENEX_EVAL auswerten. TARGET_PROPERTY allein lässt verschachtelte `$<CONFIG>`-Ausdrücke unverändert; das funktioniert zufällig bei statischen Linux-Pfaden, aber nicht bei Visual-Studio-Multikonfiguration. Nachgewiesen in Windows-Run 38637581721 und einem isolierten CMake-Reproduzierer.
