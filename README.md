# Duco VMC custom firmware

Firmware ESP32 autonome pour domotiser une DucoBox Energy Comfort via son bus
UART interne.

Le but du projet est de piloter la VMC depuis une installation domotique
locale, sans Connectivity Board officielle : lecture des temperatures, etat du
bypass, modes de ventilation, MQTT, API HTTP, interface web embarquee et
regulation automatique de free-cooling.

Le firmware principal est dans [`firmware-custom/`](firmware-custom/). Le
dossier contient aussi les notes de recherche et les traces de reverse
engineering qui ont permis de valider les commandes Duco utilisees.

## Fonctionnalites

- Interface web locale embarquee sur l'ESP32.
- API HTTP simple pour integration locale.
- MQTT compatible Jeedom, Home Assistant ou autre broker local.
- Publication MQTT optimisee : les valeurs ne sont publiees que si elles
  changent.
- JSON MQTT complet avec cles en francais.
- Lecture temperatures ODA/SUP/ETA/EHA.
- Lecture et pilotage des modes de ventilation Duco.
- Lecture de la position physique du bypass.
- Pilotage du mode bypass : auto, ferme force, ouvert force.
- Regulation automatique free-cooling avec prevision meteo Open-Meteo.
- Reglages de regulation modifiables dans l'interface web et stockes dans
  l'ESP32.
- OTA Arduino apres le premier flash USB.
- Limiteur d'ecritures Duco integre : 200 ecritures/jour et 2 secondes minimum
  entre deux ecritures.

## Interface web

L'interface web est accessible en local sur `http://duco-vmc.local` apres
connexion Wi-Fi de l'ESP32.

Elle permet de piloter la VMC sans passer par la domotique :

- etat du bus Duco et heure de derniere mise a jour ;
- schema des flux d'air avec les temperatures ;
- mode courant, consigne confort et commandes ventilation ;
- bypass avec etat physique, ouverture en pourcentage et commandes auto/ouvrir/
  fermer ;
- panneau de regulation automatique ;
- informations systeme : filtre, temps restant du mode courant, quota
  d'ecritures.

L'interface web n'a pas d'authentification HTTP. Elle est prevue pour rester sur
un reseau local de confiance.

## Regulation automatique

La regulation auto cherche a optimiser le confort d'ete sans gaspiller de
chaleur en hiver.

Elle utilise :

- la temperature interieure mesuree par la Duco (`ETA`) ;
- la temperature exterieure mesuree par la Duco (`ODA`) ;
- la consigne confort ;
- la prevision Open-Meteo du lendemain pour Chalons-en-Champagne.

Principe :

- si demain s'annonce chaud et que l'air exterieur est plus frais que
  l'interieur, le firmware force le bypass ouvert et augmente la ventilation
  pour charger le batiment en fraicheur ; l'ouverture forcee exige maintenant
  au moins 3.0 degC d'ecart entre interieur et exterieur ;
- si l'exterieur devient trop chaud, il ferme le bypass pour eviter de faire
  entrer de la chaleur ;
- si le delta interieur/exterieur est inferieur a 3.0 degC pendant un besoin de
  froid, il garde le bypass ferme ;
- en periode froide ou sans besoin de refroidissement, il laisse la Duco en mode
  auto ou ferme le bypass pour conserver la chaleur ;
- une commande manuelle de mode, consigne ou bypass met la regulation en pause
  pendant 2 heures.

La regulation n'ecrit jamais si la Duco est deja dans l'etat demande, et toutes
ses actions passent par le limiteur d'ecritures.

## MQTT et API

Le firmware expose :

- `GET /api/state` pour l'etat complet ;
- des routes `POST` pour piloter mode, consigne, bypass et regulation auto ;
- un topic MQTT `duco-vmc/state` avec l'etat complet en JSON francais ;
- des topics MQTT unitaires pour les integrations simples ;
- des topics `duco-vmc/set/...` pour les commandes.

Voir [`firmware-custom/README.md`](firmware-custom/README.md) pour la liste
complete des routes et topics.

## Installation rapide

1. Installer PlatformIO.
2. Ouvrir `firmware-custom/src/config.h`.
3. Remplacer les valeurs `CHANGE_ME_*` par les identifiants Wi-Fi, MQTT et OTA.
4. Flasher une premiere fois en USB :

```bash
cd firmware-custom
pio run -t upload
pio device monitor
```

5. Les mises a jour suivantes peuvent se faire en OTA :

```bash
pio run -e esp32dev_ota -t upload
```

## Materiel

- ESP32 DevKit.
- 4 fils Dupont.
- Connexion UART2 vers le connecteur interne de la DucoBox.

Le cout materiel est typiquement de 8 a 15 euros.

## Notes de securite

Toute intervention sur le PCB de la VMC peut annuler la garantie. Couper le
230 V au disjoncteur avant toute ouverture du capot.

Ne jamais publier un `config.h` contenant de vrais identifiants Wi-Fi, MQTT ou
OTA. Les valeurs du depot public sont des placeholders.

## Documentation du projet

| Fichier | Contenu |
|---|---|
| [`firmware-custom/`](firmware-custom/) | Firmware C++ autonome PlatformIO |
| [`firmware-custom/README.md`](firmware-custom/README.md) | Details firmware, API et MQTT |
| [`01-MATERIEL.md`](01-MATERIEL.md) | Materiel necessaire |
| [`03-BRANCHEMENT.md`](03-BRANCHEMENT.md) | Connexion UART sur la DucoBox |
| [`08-BYPASS-MANUEL.md`](08-BYPASS-MANUEL.md) | Commande bypass `0x100A` |
| [`11-COMMANDES-KOKX.md`](11-COMMANDES-KOKX.md) | Commandes reprises ou comparees au repo kokx |

## Sources

- [`kokx/duco-analysis`](https://github.com/kokx/duco-analysis)
- [`kokx/esphome`](https://github.com/kokx/esphome)
- [PR ESPHome Duco #7993](https://github.com/esphome/esphome/pull/7993)
