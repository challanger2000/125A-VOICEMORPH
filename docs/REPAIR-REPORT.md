# Reparaturbericht — 125A VoiceMorph, 14.10.2026

## Grundlage und Status

Arbeitszweig: `develop/v1.0.0`, Ausgangscommit `341fc8bd1a3d4a42e20ed945420a352ffe10708d`, Klangreferenz Windows-Build **38622391579**, Version **0.1.0**. Dieser Bericht beschreibt die Reparaturen gegenüber dem Ausgangscommit. Für die angeforderte Lieferung des installierbaren Windows-VST3 wird der reparierte Stand auf dem Entwicklungszweig gebaut. Die bestehenden VST3-Klassen-IDs, Parameter-IDs und das aktuelle State-v8-Format bleiben erhalten. Alte v1–v7-States können strukturell weiterhin gelesen werden; klangliche Rückwärtskompatibilität ist gemäß Nutzerauftrag weder implementiert noch zugesichert.

**Die nachgewiesenen und hier beschriebenen Codefehler sind korrigiert. Eine Freigabe als Windows-/Studio-One-Release ist noch nicht belegt.** Die lokale Maschine kann ein echtes Linux-VST3 ohne GUI bauen und laden, aber weder Windows-VSTGUI noch Studio One ausführen. Neue Windows-Test- und Workflow-Änderungen sind deshalb als vorbereitet, nicht als bestanden zu bewerten.

## Korrekturen

| Priorität | Nachgewiesenes Problem | Korrektur / Fundstelle | Nachweis |
|---|---|---|---|
| Hoch | Ein ausgedienter IR-Stage wurde erst beim nächsten Ladeauftrag freigegeben; ein bereits wartender Austausch konnte dauerhaft hängen. | `src/IRStageHandoff.h`, `vst/processor.cpp`: periodische Freigabe durch Worker, getrennte pending/retired-Lebensdauer. Keine Freigabe im Audio-Thread. | `tests/test_ir_handoff.cpp`: konkurrierende Veröffentlichungen und schließlich hörbarer letzter Auftrag; ASan/UBSan. |
| Hoch | State-/Sampleraten-Wiederherstellung konnte mit noch nicht vorbereiteten IRs starten. | `vst/processor.cpp`: vollständig vorbereitete, gemeinsam übernommene Parameter-/IR-Transaktion; synchroner Aufbau im inaktiven SDK-Setup; fehlerhafte IR-Payloads werden vor Übernahme abgelehnt. | `tests/test_vm_host.cpp`: tatsächliches VST3, Recall vor/nach Setup, identische Audioausgabe bei unterschiedlichen Blockgrößen, 32-/64-Bit, beschädigte States, Ratewechsel. |
| Hoch | Neue IRs starteten kalt ohne hörsicheren Übergang. | `src/IRStage.h`: vorgewärmter Zielpfad, 10-ms-Überblendung; bisheriger Pfad läuft weiter. Predelay-Wechsel überblenden zwei Abgriffe. | Identischer IR-Austausch ohne den früheren 128-Sample-Aussetzer; Load/Clear-Übergänge in `tests/test_audit_repairs.cpp`. Kein allgemeiner subjektiver Klickfreiheitstest. |
| Hoch | Kontinuierliche Host-Automation wurde zwischen Punkten gehalten. | `src/ParameterPointCursor.h`, `vst/processor.cpp`: stückweise lineare VST3-Rampen; binäre Schalter bleiben diskret; End-Offset ohne zusätzliche Extrapolation. | Ein-/Mehrpunkt-Rampen und diskrete Schalter in Reparaturtest. SDK-Validator und echter Host-Queue-Test ergänzen, ersetzen aber keinen Studio-One-Automationstest. |
| Hoch | GRIT hatte beim Übergang von OFF zu einem sehr kleinen Wert einen Pegelsprung. | `src/VoiceDSP.h`: kontinuierlicher Übergang innerhalb des ersten Prozents; bisherige Kennlinie ab 1 % exakt erhalten. | Nahe-OFF-Stetigkeit und exakter Vergleich zur bisherigen Formel. |
| Hoch | IR-Sampleratenkonvertierung änderte durch veränderte Tap-Dichte den Transferpegel. | `src/IRResampler.h`: Skalierung mit inverser Sampleratenrelation. Matching-Rate-IRs unverändert, keine neue Peak-Normalisierung von MIC/PREAMP. | DC-Summe eines inneren Impulses bei 44,1/48/96 kHz. Begrenzte Sinc-Randabschneidung bleibt; kein Anspruch auf perfekte Transfergleichheit jeder IR an jedem Frequenzpunkt. |
| Mittel | INPUT-/OUTPUT-, IR-MIX-/LEVEL-Änderungen sprangen unmittelbar. | `src/SmoothValue.h`, `src/VoiceDSP.h`, `src/FixedLatencyIRChain.h`, `src/IRStage.h`: 5-ms-Rampen, Startup-/Restore-Werte unmittelbar initialisiert. LOW CUT überblendet im ersten Prozent aus OFF, statt bereits beim kleinsten Wert den 35-Hz-Filter hart zuzuschalten. | Rampen-/Endwerttest; Referenzvergleich bei stationären Parametern. |
| Mittel | INPUT-Anzeige/Clip bezog sich auf das Signal vor INPUT-Trim. | `vst/processor.cpp`: Messung nach tatsächlich angewandtem, geglättetem Trim; Output weiterhin nach finaler Verarbeitung. | Geladenes Modul: positiver Trim löst passenden Input-Peak/Clip aus. |
| Mittel | Physikalische Parameterwerte waren gegenüber Host-Konvertierung inkonsistent. | `src/VoiceParameterText.h`, `vst/controller.cpp`: explizite normalisiert↔physikalisch-Konvertierungen für dB, Prozent, Semitöne, Hz und ms. | Rundlaufprüfungen; echte Controller-Abfragen im Modultest. |
| Mittel | Große IRs waren nur nach Dateigröße begrenzt; endliche große Taps konnten beim FFT-Aufbau überlaufen. | `src/IRMemoryBudget.h`, `src/WaveIR.h`, `src/FFTStereoIR.h`, `src/IRFileBuilder.h`, `src/PartitionedIR.h`: 30-s-/Decoded-/Stage-Speicherbudget vor Aufbau, Prüfung auf endliche FFT-Spektren, Exceptions am SDK-/Worker-Rand abgefangen. | Budget-Grenzen und überlaufende FFT als abgelehnter Eingang. Kein vollständiger OOM-Fault-Injection-Test. |
| Mittel | Paketumbenennung konnte äußeren Bundle-Namen und inneren Modulnamen trennen; QA konnte einen älteren Build prüfen. | `vst/CMakeLists.txt`, `tools/check-vst3-package.py`, beide Windows-Workflows: offizieller Name direkt im SDK-Paketziel; finaler installierbarer Pfad für Validator/Modultest/Tester; aktuelle Commit-Provenienz. | Lokales echtes Linux-Paket geprüft; Windows-Workflow noch auszuführen. |
| Mittel | Sechs Release-Testziele verloren ihre assert-Prüfungen durch NDEBUG. | `CMakeLists.txt`: Assertions für alle lokalen Testziele aktiviert; Reparaturtest nutzt zusätzlich unabhängige CHECKs. | Eigener erwarteter Negativtest beweist, dass die Release-Prüfung einen Fehler zurückweist. |
| Niedrig | Laufender GUI-IR-Auftrag konnte bei Ratewechsel verloren gehen. | `vst/processor.cpp`: angefragten Pfad separat halten und nicht abgeschlossenen Auftrag nach inaktivem Setup neu einreihen. | Modultest für CLEAR unmittelbar vor Ratewechsel und für fertige IRs; keine deterministische GUI-/Worker-Race-Injektion. |
| Niedrig | Fehlende Silent-Flags und Denormal-Schutz. | `vst/processor.cpp`, `src/RealtimeGuard.h`, `src/VoiceDSP.h`: tatsächliche Nullausgänge markieren, Host-FP-Flags nach Callback wiederherstellen, winzige IIR-Zustände löschen. | SDK-Validator; stationärer Referenzvergleich. |

## Tatsächlich ausgeführte Prüfungen

- **35/35 CTest-Prüfungen** im Release-Build mit geladenem Linux-VST3: 34 positive Prüfungen plus ein absichtlich fehlgeschlagener, durch CTest erwarteter Negativtest.
- **47/47 Steinberg-Validator-Prüfungen**, SDK 3.8.1, exakt gepinnt auf `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` mit passenden Submodulen. Validator-PASS ist kein Qualitätsurteil über Stimme oder GUI.
- IR-Handoff und Reparatur-DSP-Regressions unter **AddressSanitizer und UndefinedBehaviorSanitizer**, ohne Diagnosen. Das ist kein ThreadSanitizer-Nachweis der gesamten Anwendung.
- Tatsächliches Paket `build/host/VST3/Release/125A VoiceMorph.vst3`: Architekturmodul, Bundle-Name, Lizenzdateien und Klassen-Metadaten geprüft. Dieses ausdrücklich kopflose Linux-Testpaket ist kein Windows-Lieferartefakt.
- Reproduzierbarer Klangvergleich: `python tools/compare-reference-sound.py`. Eine unveränderte synthetische Stereo-Fixture wird gegen den Ausgangscommit und die reparierten Header kompiliert; je zwei Sekunden bei 44,1 und 48 kHz sind bytegleich. Enthält HOLLYWOOD-nahe Voice-Werte, B1, drei passende IRs, Formant −1 st, Pitch 0, GRIT OFF. Das beweist diesen konkreten stationären Pfad; keine allgemeine Gleichheit bei Automation, SRC, GRIT nahe OFF oder beliebigen Stimmen.
- Separater CPU-/C++-Allokationsprobe bei 44,1/48 kHz und Blocks 16–512 mit drei dichten 700-ms-IRs, Pitch/Formant/B1 sowie bewegten Voice-Reglern. Keine C++-Heap-Allokationen; Zeiten sind Diagnosewerte eines geteilten Rechners, keine DAW-Zusage. Einzelne Deadline-Überschreitungen müssen als solche gelesen werden.
- Workflow-YAML syntaktisch geparst, `git diff --check` sauber. PowerShell und Windows-GUI-Test wurden lokal nicht ausgeführt.

Lokale Logs: `build/host/final-build.log`, `build/host/final-ctest.log`, `build/host/final-realtime-automation.log`. Diese erzeugten Dateien sind bewusst keine versionierten PASS-Artefakte.

## Windows-/Host-Nachweise vor Freigabe

1. Windows-x64-Workflow am reparierten Commit ausführen: GUI-Version kompilieren, Native-Editor-Test bei 100/150 %, Validator, tatsächliches Modulaudio und maßgeblicher 125A-Tester am finalen Paket. Der erweiterte GUI-Test leitet echte Editor-Edits an den Processor weiter und vergleicht den gespeicherten Zustand; er ist noch nicht ausgeführt.
2. Studio One 44,1/48 kHz: Installation/Scan, Öffnen/Schließen/Zoom, Maus/Text/Automation, PDC und Offline-Render, neue Projekte mit eingebetteten drei IRs speichern, Quelldateien verschieben und erneut laden. Es existieren laut Nutzer keine zu schützenden älteren Projekte.
3. Mit der Nutzerstimme und eigenen IRs pegelgleich hören; besonders GRIT nahe OFF, Sampleraten-konvertierte IRs, Kompression, Pitch/Formant, Übergänge und kleine Buffer bewerten. Referenzklang bleibt Ziel; absichtliche Fehlerkorrekturen verändern die bisher fehlerhaften Sonderfälle.

Bis diese Nachweise vorliegen, ist die technische Windows-/Studio-One-Releasefähigkeit **nicht bestätigt**. Bereits bestandene Linux-Tests dürfen nicht als Windows-GUI- oder 125A-Full-QA-PASS ausgegeben werden.

## Konkrete aktuelle Codefundstellen

- `vst/processor.cpp:53` — `void VMProcessor::irWorkerLoop`

- `vst/processor.cpp:251` — `VMProcessor::setupProcessing`

- `vst/processor.cpp:303` — `VMProcessor::adoptRestoreAudio`

- `vst/processor.cpp:665` — `VMProcessor::setState`

- `src/IRStage.h:118` — `void process(`

- `src/IRStageHandoff.h:46` — `StereoIRStage* acquireAudio`

- `src/ParameterPointCursor.h:43` — `double valueAt`

- `src/ParameterStateMailbox.h:28` — `std::uint64_t publish`

- `src/VoiceDSP.h:96` — `float processSample`

- `src/IRResampler.h:12` — `preserveTransfer`

- `src/IRMemoryBudget.h:10` — `estimateIRMemory`

- `src/PartitionedIR.h:77` — `for(const auto& bin`

- `vst/controller.cpp:354` — `normalizedParamToPlain`

- `vst/CMakeLists.txt:12` — `smtg_add_vst3plugin`

- `CMakeLists.txt:121` — `get_property(vm_test_targets`

- `tests/test_vm_editor_lifecycle.cpp:149` — `class GUIEditBridge`

## Nachtrag zur Windows-Lieferung

Die Windows-Ausführung hat zusätzlich einen SDK-3.8.1-/Visual-Studio-Fehler beim Ressourcen-Kopieren mit Leerzeichen im offiziellen Paketnamen nachgewiesen (Run 38635144881). `vst/CMakeLists.txt` verwendet nun dependency-basierte, mit VERBATIM quotierte Kopien an die vollständigen Bundle-Pfade. Keine nachträgliche Bundle-Umbenennung und keine Änderung der gepinnten SDK-Quellen. Die Windows-Lieferung enthält Installationsanleitung und Commit-/Modulhash-Provenienz. Der aktuelle Actions-Lauf entscheidet über den tatsächlichen Windows-Nachweis.
