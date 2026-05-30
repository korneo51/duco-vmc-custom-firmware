# Checklist de mise en route

À cocher dans l'ordre. Si une case bloque, ne pas passer à la suivante.

## Phase 1 — Diagnostic préalable (sans rien acheter)

- [ ] Lu les 4 températures (ODA, SUP, ETA, EHA) dans le menu display une nuit
      où le problème se manifeste
- [ ] Vérifié qu'il n'y a pas de code erreur actif dans la box
- [ ] Vérifié le côté L/R du bypass dans le menu installateur (correspond au
      côté physique présent)
- [ ] Testé l'ouverture/fermeture manuelle du bypass depuis le menu service
      (servo bouge mécaniquement)
- [ ] Vérifié les consignes confort de toutes les zones (zone 1 jour, zone 2
      nuit si bi-zone)
- [ ] Comparé T-ODA à un thermomètre extérieur indépendant (écart < 2 °C)

**Si tout est OK → bidouille pertinente, passer à la phase 2.**
**Sinon → corriger d'abord le défaut diagnostiqué.**

## Phase 2 — Achat du matériel

- [ ] ESP32 DevKit WROOM-32 USB-C commandé
- [ ] Câble USB compatible disponible
- [ ] Fils Dupont F-F (au moins 4) disponibles
- [ ] Multimètre disponible
- [ ] Alimentation USB 5 V dédiée disponible (recommandé)

## Phase 3 — Identification du connecteur

- [ ] Disjoncteur 230 V de la VMC coupé
- [ ] Capot de la VMC ouvert
- [ ] Connecteur 12 broches Connectivity Board localisé sur le PCB
- [ ] Photo du connecteur prise (pour référence)
- [ ] Connecteur **libre** (pas de Connectivity Board officielle déjà
      branchée — sinon utiliser directement Modbus TCP)

## Phase 4 — Brochage

- [ ] Sous tension, multimètre : GND identifiée par continuité
- [ ] Sous tension, multimètre : VCC identifiée (noter 3,3 V ou 5 V)
- [ ] Sous tension, multimètre : TX (ligne avec trafic actif) identifié
- [ ] Sous tension, multimètre : RX identifié par élimination
- [ ] Disjoncteur coupé avant câblage
- [ ] 4 fils Dupont branchés sur le connecteur Duco (sans soudure)
- [ ] 4 fils Dupont branchés sur l'ESP32 :
      - GND Duco → GND ESP32
      - VCC Duco → VIN ou 3V3 ESP32 selon mesure (OU non branché si alim USB)
      - TX Duco → GPIO16 (RX2) ESP32
      - RX Duco → GPIO17 (TX2) ESP32

## Phase 5 — Flash ESPHome

- [ ] ESPHome installé (pip ou Docker)
- [ ] `secrets.yaml` créé à partir de `secrets.yaml.example` et rempli
- [ ] ESP32 branché en USB sur le PC
- [ ] Port COM reconnu par Windows
- [ ] Première compilation et flash réussis (`esphome run duco.yaml`)
- [ ] Logs ESPHome ouverts (`esphome logs duco.yaml`)
- [ ] ESP32 connecté au Wi-Fi (IP visible dans les logs)

## Phase 5 bis — Flash firmware custom PlatformIO

- [ ] `firmware-custom/src/config.h` rempli avec Wi-Fi, MQTT et mots de passe
- [ ] Compilation réussie (`pio run`)
- [ ] Flash USB réussi (`pio run -t upload`)
- [ ] Moniteur série ouvert (`pio device monitor`)
- [ ] Logs Wi-Fi OK avec IP visible
- [ ] Logs UART OK : `UART2 @ 57600 bauds, RX=16 TX=17`
- [ ] Premier `[DUCO] poll` visible
- [ ] Checklist `07-TESTS-FIRMWARE-CUSTOM.md` suivie pour les tests UART

## Phase 6 — Validation bus Duco

- [ ] Disjoncteur 230 V réarmé, box redémarrée
- [ ] Logs ESPHome affichent "Discovered nodes" avec au moins Node 1 = BOX
- [ ] Logs ESPHome affichent les 4 températures (ODA/SUP/ETA/EHA) avec des
      valeurs cohérentes (proches de celles du menu display)
- [ ] État bypass remonté dans les logs
- [ ] Test lecture consigne confort (web interface ESPHome sur IP de l'ESP32)
- [ ] Test écriture consigne confort : changer de 20 à 19, vérifier sur menu
      display de la box
- [ ] Test écriture mode : passer en Permanent 3, vérifier sur menu display
- [ ] Remettre les valeurs d'origine

**Si tout est OK → installation pérenne et Jeedom, passer à la phase 7.**
**Sinon → relire les logs, vérifier l'inversion TX/RX, vérifier la masse.**

## Phase 7 — Installation pérenne

- [ ] Disjoncteur 230 V coupé
- [ ] ESP32 fixé dans un emplacement sûr (boîtier, ou dans la box côté basse
      tension, isolé du PCB)
- [ ] Câbles passés proprement, à distance des 230 V
- [ ] Alimentation USB raccordée (si externe)
- [ ] Capot de la VMC refermé sans pincer les fils
- [ ] Disjoncteur 230 V réarmé
- [ ] ESP32 reconnecté au Wi-Fi
- [ ] Logs OTA confirment qu'il tourne normalement

## Phase 8 — Jeedom

- [ ] Plugin jMQTT installé et activé
- [ ] Broker MQTT démarré (embedded ou Mosquitto externe)
- [ ] Bloc `mqtt:` ajouté à `duco.yaml`, ESP32 reflashé
- [ ] Découverte automatique jMQTT lancée
- [ ] Équipement "Duco VMC" visible dans Jeedom avec toutes les commandes
- [ ] Test lecture température dans Jeedom (valeurs cohérentes)
- [ ] Test écriture mode depuis Jeedom (vérifier sur la box)
- [ ] Scénario "Duco free-cooling nuit" créé selon `05-JEEDOM.md`
- [ ] Scénario testé en mode manuel
- [ ] Scénario activé en mode automatique
- [ ] Logger du scénario activé pendant les 3 premières nuits pour valider

## Phase 9 — Monitoring

- [ ] Dashboard Jeedom "VMC" créé
- [ ] Historisation activée sur les 4 températures + bypass + débit
- [ ] Scénario d'alerte filtre créé
- [ ] Scénario d'alerte VMC déconnectée créé
- [ ] Notes prises sur les nœuds détectés (CO₂, RH) pour ajouter les capteurs
      au YAML
