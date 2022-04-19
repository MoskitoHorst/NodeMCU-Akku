# NodeMCU-Akku
Solar Akku Steuerung mit ESP32, AC gekoppelt

Grundlast-Akku

Aufgabe:
  Phase 1: Den Überschuss der PV in einen Akku laden, nachts die Grundlast aus dem Akku decken.
  Phase 2: Notstromversorgung per Steckdose, ggf. durch Einspeisung in das dann abgekoppelte Hausnetz
  Phase 3: Während eines Stromausfalls nachladen.
  
Material:
  Akku
  Ladenetzteil, Spannung und Strom steuerbar
  Grid-Tie Inverter, Leistung steuerbar
  Off Grid Wechselrichter
  ESP-32 NodeMCU
  Diverse Relais, Stromsensoren
  
Projektstand:
  Ladestrom + Grundlast sind fest eingestellt.
  Über eine Webseite des ESP32 können die Statuswerte angezeigt werden:
    - Akkuspannung
    - Akkustrom
    - Ladestrom DC
    - Entladestrom DC
  Schalten von:
    - DC Grid-Tie Inverter
    - AC Grid-Tie Inverter
    - DC Ladegerät
    - AC Ladegerät
    - Hauptrelais

Nächste Aufgaben
  Automatisches Ein-/Ausschalten von Ladegerät und Inverter
