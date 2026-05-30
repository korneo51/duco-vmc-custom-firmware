# Checklist firmware custom courant

- [x] Sniffer UART retire du driver, de l'API HTTP et de l'interface web
- [x] Routes debug retirees de l'API HTTP
- [x] Interface web refaite autour des etats et commandes utiles
- [x] Limiteur d'ecritures ajoute : 200 ecritures/jour
- [x] Limiteur d'ecritures ajoute : 2 secondes minimum entre deux ecritures
- [x] Limiteur partage par HTTP et MQTT
- [x] Compteur d'ecritures persiste en NVS
- [x] Position bypass affichee via `0x1009`
- [x] Mode bypass pilote via `0x100A`
- [x] Interface refaite avec header bus, panneaux ventilation/bypass et schema SVG temperatures
- [x] Regulation auto free-cooling ajoutee avec meteo Open-Meteo Châlons-en-Champagne
- [x] Commandes HTTP/MQTT ajoutees pour activer l'auto et lever la pause manuelle
- [x] Verification API apres upload OTA sans consommer d'ecriture
- [x] MQTT optimise : etats publies seulement si le payload change
- [x] JSON MQTT complet francise sur `duco-vmc/state`
- [x] Commandes kokx pertinentes ajoutees : modes longs, serial, filtre restant, temps restant mode
- [x] PR upstream ouverte chez kokx pour partager la commande bypass `0x100A`
- [x] Repo public `korneo51/duco-vmc-custom-firmware` publie avec configuration anonymisee
