# Liste de courses

## Strict minimum (≈ 8 €)

| Composant | Quantité | Prix indicatif | Notes |
|---|---|---|---|
| ESP32 DevKit WROOM-32 (USB-C de préférence) | 1 | 4-6 € (AliExpress) / 8-10 € (FR/EU) | Pas d'ESP8266 : le composant Duco kokx est testé sur ESP32 uniquement, et on a besoin d'un UART hardware libre |
| Câble USB pour flasher | 1 | déjà dans tes tiroirs en général | USB-A → USB-C ou micro-USB selon ta carte |
| Fils Dupont F-F (jumpers) | 4 | 2-3 € (le pack) | Pour tests sans soudure |

## Recommandé (encore ≈ 5 €)

| Composant | Utilité |
|---|---|
| Multimètre basique | Identifier GND / VCC / TX / RX sur le connecteur 12 broches (sans c'est très risqué) |
| Alimentation USB 5 V dédiée + câble | Plus simple et plus sûr que de tirer l'alim depuis le PCB de la box |
| Boîtier ABS 35×60 mm ou impression 3D | Pour caser l'ESP32 dans la box proprement |
| Soudure + fer (si tu n'as pas) | Pour la phase finale, après tests concluants en Dupont |

## Optionnel (pour aller plus loin)

| Composant | Utilité |
|---|---|
| Logic analyzer USB type Saleae clone | Sniffer le bus avant écriture si tu veux vérifier le brochage par toi-même (le brochage exact est déjà documenté par kokx, donc pas indispensable) |
| Adaptateur USB-TTL 3,3 V (FTDI / CP2102) | Si ton ESP32 a un souci de flash, utile en backup |

## À NE PAS prendre

- **ESP8266 (Wemos D1, NodeMCU 8266)** — fonctionnera peut-être en bricolant
  mais le composant ESPHome de kokx n'est validé que sur ESP32.
- **CC1101 868 MHz** — c'est pour la bidouille RF (technique 2), pas la voie
  UART.
- **Level shifter 5V↔3.3V** — inutile, le bus Duco est en TTL 3,3 V natif.

## Liens d'achat (exemples non-affiliés)

- ESP32 DevKit WROOM-32 USB-C : rechercher "ESP32 DevKit C V4 USB-C" sur
  AliExpress, Amazon ou Mouser.
- Pack jumpers F-F : "Dupont jumper wires female female 20cm".
- Multimètre 10-20 € : n'importe quel modèle Uni-T ou Aneng fait l'affaire.

## Vérification avant de souder

Avant tout achat, vérifie chez toi :

1. Tu as bien une **DucoBox Energy Comfort 350** (et pas la version *Plus* qui
   peut différer).
2. La box est **accessible** (pas dans un placard impossible à ouvrir).
3. La box n'a **pas déjà une Connectivity Board** branchée sur le connecteur
   12 broches. Si oui, tu peux l'utiliser directement via Modbus TCP et tu n'as
   pas besoin de cette bidouille.
