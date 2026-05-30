# Tests de communication avec le firmware custom

Objectif : valider progressivement que l'ESP32 parle bien avec la DucoBox sur
le bus UART interne, avant de piloter la VMC depuis Jeedom.

## Checklist rapide

- [ ] `firmware-custom/src/config.h` rempli avec le Wi-Fi réel
- [ ] ESP32 branché en USB au PC
- [ ] Firmware compilé avec `pio run`
- [ ] Firmware flashé avec `pio run -t upload`
- [ ] Moniteur série ouvert avec `pio device monitor`
- [ ] Logs Wi-Fi OK avec adresse IP
- [ ] Log UART visible : `UART2 @ 57600 bauds, RX=16 TX=17`
- [ ] Premier `[DUCO] poll` visible au démarrage
- [ ] Au moins une trame valide reçue ou `online=true` dans `/api/state`
- [ ] Lecture mode OK
- [ ] Lecture consigne confort OK
- [ ] Lecture températures testée, même si elle peut échouer au départ
- [ ] Premier test écriture uniquement après lectures OK

## Préparation

Éditer `firmware-custom/src/config.h` avant flash :

```cpp
#define WIFI_SSID       "..."
#define WIFI_PASSWORD   "..."
#define MQTT_HOST       "..."   // peut rester faux pour les premiers tests
#define MQTT_USER       ""      // vide si le broker MQTT n'a pas d'auth
#define MQTT_PASSWORD   ""      // vide si le broker MQTT n'a pas d'auth
#define WEB_USER        "admin"
#define WEB_PASSWORD    "..."
```

Pour les tests, garder :

```cpp
#define DUCO_DEBUG_PROTOCOL 1
#define DUCO_DEBUG_FRAMES   0
```

Activer `DUCO_DEBUG_FRAMES` seulement si les lectures échouent : le bus peut
générer beaucoup de logs.

## Test 1 — Firmware seul, sans VMC

But : vérifier que l'ESP32, le Wi-Fi, le serveur web et le build PlatformIO sont
OK avant de toucher à la VMC.

```powershell
cd "C:\Users\korne\Documents\Duco VMC\firmware-custom"
pio run
pio run -t upload
pio device monitor
```

Si l'upload échoue avec `Wrong boot mode detected (0x13)`, relancer la commande
et maintenir le bouton **BOOT** de l'ESP32 pendant les points `Connecting...`.
Relâcher dès que PlatformIO commence à écrire la flash.

Attendu :

```text
[WiFi] OK ip=...
[WiFi] ssid=... bssid=... rssi=... dBm
[WiFi] gateway=... subnet=... dns=...
[mDNS] http://duco-vmc.local
[DUCO] UART2 @ 57600 bauds, RX=16 TX=17
[WEB] started on port 80
[DUCO] poll
[DUCO] ACK timeout ...
```

Les timeouts sont normaux si l'ESP32 n'est pas encore branché à la DucoBox.

Si l'IP Wi-Fi n'est pas dans le même réseau que Jeedom/MQTT, par exemple ESP32
en `192.168.28.x` alors que Jeedom est en `192.168.10.x`, corriger le SSID, le
VLAN ou le DHCP côté routeur/point d'accès. Forcer une IP statique `192.168.10.x`
ne fonctionne que si le Wi-Fi est réellement ponté sur ce LAN.

Après un premier flash réussi et une connexion Wi-Fi OK, les uploads suivants
peuvent se faire en OTA :

```powershell
pio run -e esp32dev_ota -t upload
```

## Test 2 — Écoute passive du bus

But : valider RX, GND et baudrate sans écrire de commande dangereuse.

1. Couper le 230 V avant câblage.
2. Brancher seulement :
   - GND Duco -> GND ESP32
   - TX Duco -> GPIO16 RX2 ESP32
3. Alimenter l'ESP32 en USB.
4. Rétablir le 230 V de la VMC.
5. Ouvrir `pio device monitor`.

Attendu avec `DUCO_DEBUG_FRAMES=0` :

- `online` doit passer à `true` dans `http://IP_ESP32/api/state`
- le statut bus doit passer en ligne dans l'interface web

Si rien ne passe en ligne :

- vérifier masse commune
- vérifier que TX Duco est bien connecté sur GPIO16
- vérifier 57600 bauds
- activer `DUCO_DEBUG_FRAMES=1`, reflasher et regarder si des `[DUCO RX]`
  apparaissent

## Test 3 — Requêtes actives en lecture

But : valider aussi TX ESP32 -> RX Duco, sans modifier de réglage.

Brancher en plus :

- RX Duco -> GPIO17 TX2 ESP32

Attendu dans le moniteur série :

```text
[DUCO] poll
[DUCO] read mode OK mode=...
[DUCO] read comfort OK temp=...
```

Interprétation :

| Résultat | Diagnostic |
|---|---|
| `ACK timeout` partout | TX/RX probablement inversés ou RX Duco non connecté |
| `read mode OK`, `read comfort OK` | communication bidirectionnelle validée |
| mode/comfort OK mais températures FAIL | transport OK, registres température à ajuster |
| `online=true` mais lectures actives FAIL | RX passif OK, TX ESP32 vers Duco à vérifier |

## Test 4 — Interface web

Ouvrir :

```text
http://duco-vmc.local
```

ou l'IP affichée dans les logs Wi-Fi.

Vérifier :

- état bus en ligne
- mode courant cohérent avec la box
- consigne confort cohérente avec le menu Duco
- températures si disponibles

## Test 5 — Première écriture contrôlée

Ne faire cette étape qu'après `read mode OK` et `read comfort OK`.

1. Noter le mode et la consigne actuels depuis la box.
2. Depuis l'interface web, changer la consigne confort de `20.0` à `19.5`.
3. Vérifier sur le menu display Duco que la valeur change.
4. Remettre la valeur initiale.
5. Changer le mode vers `Manuel 1`, vérifier sur la box.
6. Remettre `Auto`.

Ne pas spammer les commandes : garder au moins 2 secondes entre deux écritures.

## Points connus à surveiller

- Le firmware custom compile et flashe via PlatformIO, mais ESPHome n'est pas
  installé en CLI sur ce PC.
- Les lectures `mode` et `consigne confort` suivent les traces du reverse
  engineering kokx.
- Les 4 températures dans `readBoxTemps()` utilisent les commandes du composant
  ESPHome kokx : types `0..3` sur le registre `0x09`.
- `bypassOpen` est lu via `0x10/0x09`; `flowLevel` vient de la réponse mode.

## Prochaine étape si les températures échouent

Activer temporairement :

```cpp
#define DUCO_DEBUG_FRAMES 1
```

Puis capturer 2 à 3 minutes de logs série pendant que la box tourne. Avec ces
trames, on pourra mapper les registres réels des températures/bypass ou aligner
le custom firmware sur le composant ESPHome de kokx.
