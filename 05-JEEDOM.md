# Intégration Jeedom : MQTT + scénario free-cooling

## Architecture

```
[ESP32 dans la VMC]  ← UART →  [DucoBox]
       ↓
       Wi-Fi
       ↓
[Broker MQTT]  ←→  [Jeedom + plugin jMQTT]
                          ↓
                       Scénarios
```

ESPHome publie en MQTT. Jeedom consomme via le plugin jMQTT (auto-découverte).

## Pré-requis Jeedom

- Jeedom à jour
- Plugin **jMQTT** installé et activé (~6 € sur le market officiel)
- Broker MQTT accessible. Deux options :
  - **Mosquitto** standalone sur ton réseau (Docker, NAS, Raspberry Pi…)
  - **Broker intégré à jMQTT** (mode embedded, plus simple si tu démarres)

## Configuration du broker (mode embedded jMQTT)

1. Dans Jeedom : Plugins → Communication → jMQTT
2. Onglet "Configuration" → cocher "Démarrer le broker MQTT embarqué"
3. Définir :
   - Port : 1883 (défaut)
   - Authentification : oui (recommandé)
   - User : `duco`
   - Password : la valeur de `mqtt_password` dans `secrets.yaml`
4. Redémarrer le service jMQTT

## Activer MQTT dans le YAML ESPHome

Décommenter et adapter le bloc `mqtt:` à la fin de `duco.yaml` :

```yaml
mqtt:
  broker: 192.168.X.Y          # IP du Jeedom (ou du Mosquitto si externe)
  port: 1883
  username: duco
  password: !secret mqtt_password
  discovery: true
  topic_prefix: duco-vmc
```

Reflasher l'ESP32 :

```
esphome run duco.yaml
```

## Découverte côté Jeedom

Dans Jeedom :

1. Plugins → Communication → jMQTT
2. Onglet "Équipements" → "Découverte automatique" (Home Assistant Discovery)
3. Attendre quelques secondes : un nouvel équipement "Duco VMC" apparaît
4. Cliquer dessus → tu vois toutes les commandes :

**En lecture (info)** :

- Duco Temp ODA
- Duco Temp SUP
- Duco Temp ETA
- Duco Temp EHA
- Duco Bypass
- Duco Flow Level
- Duco Mode Time Remaining
- Duco Filter Remaining
- Duco Delta ETA-ODA
- Duco Rendement Echangeur
- Duco Serial Number
- Duco Mode (état)

**En écriture (action)** :

- Duco Mode (set) — accepte Auto, Manual 1/2/3, Permanent 1/2/3, Not at home
- Duco Consigne Confort (set) — accepte une valeur en °C

## Le scénario "free-cooling nocturne"

Objectif : la nuit, quand il fait trop chaud à l'intérieur ET qu'il fait
réellement plus frais dehors, forcer l'ouverture du bypass via une baisse
de la consigne confort, et pousser le débit au max.

### Création du scénario

Jeedom → Outils → Scénarios → Ajouter

- Nom : `Duco free-cooling nuit`
- Mode : Provoqué
- Déclencheurs : ajouter un déclencheur CRON `*/10 * * * *` (toutes les 10 min)
- Activé : oui
- Logger : oui (utile pour debug au début)

### Bloc principal (logique en pseudo-code)

```
SI heure entre 22:00 et 06:30
ET  #[Duco VMC][Duco][Duco Temp ETA]# > 22.0
ET  #[Duco VMC][Duco][Duco Temp ODA]# < (#[Duco VMC][Duco][Duco Temp ETA]# - 2)
ET  #[Duco VMC][Duco][Duco Temp ODA]# > 10.0
ALORS
    Action  : #[Duco VMC][Duco][Duco Consigne Confort (set)]#  = 15.0
    Action  : #[Duco VMC][Duco][Duco Mode (set)]#              = "Permanent 3"
    Variable: free_cool_actif = 1
SINON SI heure = 06:35
ET  free_cool_actif == 1
ALORS
    Action  : #[Duco VMC][Duco][Duco Consigne Confort (set)]#  = 21.5
    Action  : #[Duco VMC][Duco][Duco Mode (set)]#              = "Auto"
    Variable: free_cool_actif = 0
FIN SI
```

### Bloc dans la syntaxe Jeedom (à reproduire dans l'éditeur visuel)

```
SI
    [valeur] #[Duco VMC][Duco][Duco Temp ETA]# [opérateur] > [valeur] 22
    ET
    [calcul] #[Duco VMC][Duco][Duco Delta ETA-ODA]# [op] > [valeur] 2
    ET
    [valeur] #[Duco VMC][Duco][Duco Temp ODA]# [op] > [valeur] 10
    ET
    [heure] entre 22:00 et 06:30
ALORS
    [action] #[Duco VMC][Duco][Duco Consigne Confort (set)]# avec valeur 15
    [action] #[Duco VMC][Duco][Duco Mode (set)]# avec valeur "Permanent 3"
    [variable] free_cool_actif = 1
SINON
    SI [variable] free_cool_actif == 1 ET [heure] >= 06:35
    ALORS
        [action] #[Duco VMC][Duco][Duco Consigne Confort (set)]# avec valeur 21.5
        [action] #[Duco VMC][Duco][Duco Mode (set)]# avec valeur "Auto"
        [variable] free_cool_actif = 0
    FIN SI
FIN SI
```

### Notes sur la logique

- Le délai de 10 min entre évaluations laisse le temps au bypass de réagir
- Le seuil `> 22 °C` à l'intérieur évite de gaspiller du free-cooling pour
  rien quand il fait déjà frais
- Le delta de 2 °C entre ODA et ETA évite les oscillations quand l'extérieur
  s'approche de l'intérieur
- Le seuil 10 °C ODA est redondant avec la logique interne de la box, mais
  c'est plus sûr de le doubler côté Jeedom
- À 06:35, on rend la main à la box en mode Auto (récupération de chaleur
  pour la journée)
- La variable `free_cool_actif` évite de réécrire la consigne à 21,5 °C en
  boucle quand on n'est pas en free-cooling

## Variantes possibles

### Free-cooling estival uniquement

Ajouter une condition sur le mois :

```
ET mois entre Mai et Septembre
```

### Sur prévisions météo

Si tu as un module météo dans Jeedom (DarkSky, OpenWeatherMap), ajouter une
condition sur la prévision de température mini de la nuit :

```
ET #[Meteo][Prevision][T_min_nuit]# < 18
```

Pour éviter de lancer le mode quand on sait que la nuit ne sera pas assez
fraîche.

### Pilotage par chambre

Si tu as des capteurs CO₂/RH par pièce (via le composant duco, ils
remontent automatiquement), tu peux déclencher des changements de mode
selon la pièce occupée. Ex : passer en Manuel 3 si CO₂ chambre > 1200 ppm.

## Dashboard Jeedom recommandé

Créer un dashboard "VMC" avec :

- Widgets thermomètres : ODA, SUP, ETA, EHA
- Widget binaire bypass : ouvert/fermé
- Widget jauge : Delta ETA-ODA (avec zones colorées)
- Widget jauge : Rendement échangeur (idéal 80-90 % bypass fermé)
- Sélecteur : Duco Mode (set)
- Slider 15-25 °C : Duco Consigne Confort (set)
- Historique 24 h des températures pour visualiser l'effet du bypass
  pendant la nuit

## Surveillance / alertes

Quelques scénarios bonus utiles :

- **Filtre à remplacer** : si `Duco Filter Remaining < 30` jours, notif
- **Bypass anormal** : si bypass=ouvert ET ODA>ETA, notif (anomalie)
- **VMC déconnectée** : si pas de message MQTT depuis 5 min, notif
