125A VoiceMorph — Windows x64 / Studio One

1. Das ZIP entpacken.
2. Den gesamten Ordner „125A VoiceMorph.vst3“ nach
   C:\Program Files\Common Files\VST3\
   kopieren. Der Zielpfad lautet anschließend:
   C:\Program Files\Common Files\VST3\125A VoiceMorph.vst3\
3. Studio One neu starten und den Plugin-Scan ausführen. Danach
   „125A VoiceMorph“ als Audioeffekt auf der Sprachspur laden.

Der .vst3-Ordner enthält das eigentliche Modul und die GUI-Ressourcen.
Die Datei in Contents\x86_64-win\ allein reicht nicht aus.

Dieses Paket ist ein Entwicklungsbuild des reparierten Plugins.
Commit, Sampleraten-Klangreferenz und SHA256 des Windows-Moduls stehen
in BUILD-PROVENANCE.json. Den Klang zunächst mit der eigenen Stimme und
mit den eigenen IRs prüfen; die CI-Prüfungen ersetzen kein Hören in der DAW.
