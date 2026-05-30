# Duco VMC — Bidouille UART + Jeedom

Projet : reprendre la main sur la **DucoBox Energy Comfort 350** sans acheter la
Connectivity Board officielle (~250 €), pour la piloter depuis Jeedom et
notamment forcer le bypass nocturne en été.

Coût total estimé : **8 à 15 €** (ESP32 + 4 fils Dupont).

## Le problème de départ

- Consigne confort réglée à 20 °C
- Il fait 26 °C à l'intérieur la nuit
- Le bypass ne s'ouvre pas alors qu'il est en ON

Logique du bypass Duco (3 conditions cumulatives) :

1. **T-ETA > T-confort** (air extrait > consigne)
2. **T-ODA > 10 °C** (air extérieur au-dessus du seuil bas — non modifiable)
3. **T-ODA < T-ETA** (air extérieur réellement plus froid que l'intérieur)

Si l'une des trois manque, le bypass reste fermé. La seconde est souvent
satisfaite. La troisième est piégeuse : si la nuit reste tiède en été ou si le
capteur ODA est mal placé / faussé, on tombe dans ce cas.

## Plan de bataille

1. **D'abord diagnostiquer** sans rien acheter — `02-DIAGNOSTIC.md`
2. Si la box fonctionne correctement mais qu'on veut piloter finement →
   bidouille UART (technique 1 : on tape directement sur le connecteur interne
   12 broches prévu pour la Connectivity Board officielle, mais avec un ESP32 à
   5 €).
3. Intégrer ESPHome → MQTT → Jeedom.
4. Écrire un scénario Jeedom de free-cooling nocturne qui force la consigne à
   15 °C en pleine nuit pour garantir l'ouverture du bypass.

## Lecture dans l'ordre

| Fichier | Quand |
|---|---|
| `01-MATERIEL.md` | Avant d'acheter |
| `02-DIAGNOSTIC.md` | Avant de bidouiller, à faire sur la box telle quelle |
| `03-BRANCHEMENT.md` | Une fois le matos reçu, pour repérer les broches |
| `04-INSTALLATION-ESPHOME.md` | Pour flasher l'ESP32 |
| `duco.yaml` | Le firmware ESPHome prêt à coller (à éditer juste pour les secrets) |
| `secrets.yaml.example` | Modèle des mots de passe à mettre à côté de `duco.yaml` |
| `firmware-custom/` | Firmware C++ autonome PlatformIO, sans ESPHome |
| `07-TESTS-FIRMWARE-CUSTOM.md` | Procédure de tests UART avec le firmware custom |
| `08-BYPASS-MANUEL.md` | État des recherches sur le pilotage manuel du bypass |
| `05-JEEDOM.md` | Pour connecter à Jeedom et créer le scénario |

## Sources de référence

- `kokx/duco-analysis` — reverse engineering complet du bus interne :
  https://github.com/kokx/duco-analysis
- PR ESPHome #7993 — composant Duco officiel (encore en review) :
  https://github.com/esphome/esphome/pull/7993
- Information sheet Modbus TCP Duco L2003592-F (mapping modes officiel) :
  https://www.duco.eu/Wes/CDN/1/Attachments/information-sheet-Modbus-TCP-(en)_638884516909638369.pdf
- Doc ESPHome UART : https://esphome.io/components/uart/

## Avertissement

Toute intervention sur le PCB de la VMC annule la garantie Duco/Daikin. Si la
box est sous garantie, choisir plutôt la Connectivity Board officielle. Couper
le 230 V au disjoncteur avant toute ouverture du capot.
