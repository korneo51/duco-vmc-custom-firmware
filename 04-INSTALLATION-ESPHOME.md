# Installation ESPHome et flash de l'ESP32

## Vue d'ensemble

ESPHome est un firmware qui se configure en YAML. Tu décris ce que tu veux,
ESPHome compile et flashe automatiquement. Pas besoin d'écrire du C.

Le composant Duco utilisé ici n'est **pas encore mergé** dans ESPHome stable
(PR `esphome/esphome#7993` en review). On utilise donc le composant comme
"external_component" depuis le fork de kokx.

## Option A — Installation locale avec Python (recommandé pour démarrer)

### Prérequis

- Python 3.9+ installé
- pip à jour

### Install

Dans un terminal (PowerShell ou CMD) :

```
pip install esphome
esphome version
```

### Flash initial (USB)

1. Brancher l'ESP32 en USB sur le PC.
2. Vérifier que Windows reconnaît le port COM (Gestionnaire de
   périphériques → Ports COM). Si non reconnu, installer le driver
   CP2102 ou CH340 selon le chipset USB de la carte.
3. Dans le dossier `C:\Users\korne\Documents\Duco VMC` :

```
esphome run duco.yaml
```

ESPHome compile (première fois = ~5 min), demande le port COM, flashe,
puis ouvre les logs en direct.

### Flash suivants (OTA via Wi-Fi)

Une fois la première version flashée et l'ESP32 connecté au Wi-Fi, les
flashs suivants se font sans fil :

```
esphome run duco.yaml
```

ESPHome détecte automatiquement l'IP et flashe en OTA.

## Option B — Dashboard ESPHome via Docker (plus visuel)

Pour ceux qui préfèrent une interface graphique :

```
docker run --rm -v "${PWD}:/config" -p 6052:6052 -it ghcr.io/esphome/esphome
```

Puis ouvrir http://localhost:6052 dans le navigateur. Le dashboard liste les
YAML du dossier, permet de flasher, de voir les logs, de gérer les secrets.

## Option C — Add-on Home Assistant (si déjà installé)

Si tu utilises déjà Home Assistant à côté de Jeedom, ESPHome est un add-on
disponible. Mais comme tu es sur Jeedom, ce n'est pas le chemin le plus
naturel.

## Préparer les secrets

Avant le premier flash, créer le fichier `secrets.yaml` à côté de `duco.yaml`
(à partir du `secrets.yaml.example` fourni) avec :

- Ton SSID Wi-Fi
- Ton mot de passe Wi-Fi
- Une clé API ESPHome (n'importe quelle chaîne longue aléatoire, c'est pour
  sécuriser l'API native ESPHome)
- Un mot de passe OTA (pareil, choisi par toi)
- Le mot de passe MQTT (à choisir, sera utilisé côté broker et côté Jeedom)

## Premier démarrage : valider le brochage

Au premier boot, ouvre les logs ESPHome :

```
esphome logs duco.yaml
```

Tu dois voir, dans l'ordre :

1. Démarrage WiFi, IP attribuée
2. UART2 initialisé à 57600 bauds
3. Bus Duco actif → liste des nœuds détectés type :

```
[duco:206]: Discovered nodes:
[duco:208]: Node 1: type 17 (BOX)
[duco:208]: Node 2: type 8 (UCBAT)
[duco:208]: Node 3: type 12 (UCCO2)
...
```

Si tu vois la découverte des nœuds : le brochage est bon, le bus parle.

Si tu vois "UART RX timeout" ou aucune découverte de nœud :

- Vérifier l'inversion TX/RX (la moitié des erreurs)
- Vérifier la masse commune
- Vérifier le baud rate (doit être 57600)
- Vérifier que la box est sous tension et fonctionne (LED bouton allumée)

## Adapter le YAML aux nœuds découverts

Une fois la découverte OK, note les adresses des nœuds qui t'intéressent
(CO₂, humidité, etc.). Édite `duco.yaml` pour ajouter des entrées dans la
section `sensor:` aux bonnes adresses. Voir les commentaires dans le YAML.

Reflash après modif :

```
esphome run duco.yaml
```

## Vérifier les capacités d'écriture

Une fois en place, depuis l'interface ESPHome (web local sur l'IP de l'ESP32) :

1. Changer le `Duco Mode` (select) → vérifier que le mode change sur la box
   (LED du bouton)
2. Changer la `Duco Consigne Confort` (number) → vérifier dans le menu
   display que la consigne a bien changé

Si les deux marchent : on peut passer à Jeedom (`05-JEEDOM.md`).
