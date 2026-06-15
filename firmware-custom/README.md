# Firmware custom Duco VMC - ESP32 sans ESPHome

Firmware Arduino/PlatformIO autonome pour domotiser une DucoBox Energy Comfort
via un ESP32 connecte au bus UART interne. Il fournit une interface web locale,
une API HTTP, du MQTT et une regulation automatique de free-cooling.

- Wi-Fi en mode station, mDNS `duco-vmc.local`
- OTA Arduino pour les uploads suivants
- MQTT publish/subscribe compatible Jeedom jMQTT
- Interface web embarquee sur le port 80
- UART2 vers la DucoBox avec le protocole reverse-engineere kokx

## Fichiers principaux

```text
firmware-custom/
|-- platformio.ini
|-- src/
|   |-- config.h      Wi-Fi, MQTT, mot de passe OTA
|   |-- main.cpp      Wi-Fi, OTA, MQTT, HTTP, limites d'ecriture
|   |-- duco.h        API protocole Duco
|   |-- duco.cpp      implementation UART Duco
|   `-- web_ui.h      interface HTML/CSS/JS embarquee
`-- data/
```

## Build et upload

USB initial :

```bash
cd firmware-custom
pio run -t upload
pio device monitor
```

OTA :

```bash
pio run -e esp32dev_ota -t upload
```

L'environnement OTA lit le mot de passe directement depuis `src/config.h`.
Avant de flasher, remplacer les valeurs `CHANGE_ME_*` dans `src/config.h`.

## Interface web

L'interface web locale est accessible sans authentification HTTP.

Elle est pensee comme un tableau de bord de pilotage local, utile meme sans
Jeedom/Home Assistant. La page affiche :

- etat bus online/offline
- mode ventilation courant et commande de changement
- consigne confort et commande de changement
- temperatures ODA/SUP/ETA/EHA + delta ETA-ODA
- bypass ouvert/ferme
- ouverture physique bypass en pourcentage
- mode bypass `Auto` / `Ouvrir` / `Fermer`
- numero de serie box, filtre restant et temps restant du mode courant
- regulation auto free-cooling avec meteo du lendemain
- reglages persistants de regulation auto : delta, seuils de temperature,
  cooldown, pause manuelle, meteo
- quota d'ecritures Duco du jour

Refresh automatique toutes les 5 secondes.

## Bypass

Mapping valide :

- `0x1009` : position physique bypass, lecture seule, `0..100 %`
- `0x100A` : mode bypass, lecture/ecriture
  - `0` = auto
  - `1` = fermeture forcee
  - `2` = ouverture forcee

La position `0x1009` est affichee mais non pilotable : la box ignore les
ecritures intermediaires sur ce registre.

## Limites d'ecriture

La Duco impose :

- 200 ecritures par jour maximum
- 2 secondes minimum entre deux ecritures

Le firmware applique ces limites sur toutes les ecritures HTTP et MQTT :

- mode ventilation
- consigne confort
- mode bypass

Les lectures et polls automatiques ne consomment pas ce quota. Le compteur est
stocke en NVS sur l'ESP32 et remis a zero au changement de jour via NTP.

## Regulation auto free-cooling

La regulation auto utilise :

- interieur : temperature ETA
- exterieur : temperature ODA
- consigne confort Duco
- prevision Open-Meteo pour Châlons-en-Champagne

Comportement principal :

- si demain est chaud et que l'exterieur est assez plus frais que l'interieur,
  le bypass passe en ouverture forcee et la ventilation en Puissance 2 ou 3 ;
  l'ouverture forcee ne demarre que si l'exterieur est au moins 3.0 degC plus
  froid que l'interieur
- si l'exterieur est trop chaud pendant un besoin de froid, le bypass est ferme
- si le besoin de froid existe mais que l'exterieur n'est pas au moins 3.0 degC
  plus froid que l'interieur, le bypass est force ferme
- en hiver ou hors besoin de froid, la regulation evite de perdre la chaleur
- toute commande manuelle mode/consigne/bypass met l'auto en pause 2 h

La regulation est desactivee par defaut au premier flash, puis son etat est
stocke en NVS.

Le diagnostic de regulation est expose dans l'interface web, l'API et MQTT :
etat actif/repos, pause manuelle, prevision du lendemain et raison de la
decision courante.

## API HTTP

- `GET /api/state`
- `POST /api/refresh_state`
- `POST /api/set_mode?value=<mode>`
- `POST /api/set_comfort?value=<celsius>`
- `POST /api/set_bypass_mode?value=0|1|2`
- `POST /api/set_auto_regulation?enabled=0|1`
- `POST /api/set_auto_config?...`
- `POST /api/clear_auto_hold`

`/api/set_auto_config` accepte les parametres optionnels suivants, sans
consommer d'ecriture Duco :

- `good_delta` : delta interieur/exterieur minimum pour ouvrir le bypass
- `strong_delta` : delta pour passer en Puissance 3
- `tomorrow_hot_margin` : demain chaud si max demain >= consigne + marge
- `need_cooling_margin` : besoin froid si interieur >= consigne + marge
- `min_interior` : securite temperature interieure minimale
- `min_outside` : securite temperature exterieure minimale
- `write_cooldown_min` : delai minimum entre deux decisions auto qui ecrivent
- `manual_hold_min` : duree de pause apres commande manuelle
- `weather_refresh_min` : intervalle de rafraichissement Open-Meteo
- `weather_lat`, `weather_lon` : coordonnees de prevision

Les anciennes routes sniffer/debug ont ete supprimees du firmware courant.

## Topics MQTT

Publies par l'ESP32 :

| Topic | Contenu |
|---|---|
| `duco-vmc/state` | JSON complet avec cles en francais |
| `duco-vmc/state/online` | `0` ou `1` |
| `duco-vmc/state/mode` | code mode |
| `duco-vmc/state/comfort` | temperature de consigne |
| `duco-vmc/state/bypass` | `0` ou `1` |
| `duco-vmc/state/bypass_position` | ouverture `0..100` |
| `duco-vmc/state/bypass_mode` | `0`, `1`, `2` |
| `duco-vmc/state/mode_time_remaining_sec` | temps restant du mode, secondes |
| `duco-vmc/state/filter_remaining_days` | jours restants filtre |
| `duco-vmc/state/serial` | numero de serie box |
| `duco-vmc/state/writes_today` | ecritures utilisees |
| `duco-vmc/state/writes_remaining` | ecritures restantes |
| `duco-vmc/state/auto_regulation_enabled` | `0` ou `1` |
| `duco-vmc/state/auto_regulation_active` | `0` ou `1` |
| `duco-vmc/state/auto_regulation_action` | action courante |
| `duco-vmc/state/auto_regulation_reason` | diagnostic court |
| `duco-vmc/state/forecast_tomorrow_max` | prevision max demain |
| `duco-vmc/state/forecast_tomorrow_min` | prevision min demain |
| `duco-vmc/state/forecast_ok` | `0` ou `1` |
| `duco-vmc/state/manual_override_active` | `0` ou `1` |
| `duco-vmc/state/oda` | temperature |
| `duco-vmc/state/sup` | temperature |
| `duco-vmc/state/eta` | temperature |
| `duco-vmc/state/eha` | temperature |

Les topics d'etat MQTT sont publies en retained uniquement quand leur payload
change. Cela limite le bruit MQTT tout en gardant les dernieres valeurs
disponibles cote broker.

Ecoutes par l'ESP32 :

| Topic | Payload |
|---|---|
| `duco-vmc/set/mode` | `0`, `4`, `5`, `6`, `7`, `8`, `9`, `10`, `132`, `133`, `134`, `196`, `197`, `198` |
| `duco-vmc/set/comfort` | ex. `19.0` |
| `duco-vmc/set/bypass_mode` | `0` auto, `1` fermer, `2` ouvrir |
| `duco-vmc/set/auto_regulation` | `0` ou `1` |
| `duco-vmc/set/auto_hold_clear` | payload libre |

## Fonctions validees

- lecture/ecriture du mode ventilation
- modes longs kokx `MAN1/2/3 x2` et `MAN1/2/3 x3`
- lecture/ecriture de la consigne confort
- lecture des temperatures ODA/SUP/ETA/EHA
- lecture du bypass
- lecture de la position bypass `0x1009`
- lecture/ecriture du mode bypass `0x100A`
- lecture du debit depuis la reponse mode
- lecture du numero de serie, du filtre restant et du temps restant de mode
- regulation auto free-cooling avec prevision meteo
- OTA apres premier flash

## Debug protocole

Les outils sniffer/debug web etaient utiles pour trouver le bypass manuel, mais
ne sont plus embarques dans le firmware de production. Si un nouveau mapping est
necessaire plus tard, repartir d'une branche dediee.
