# Bypass manuel

## Etat valide

Le pilotage manuel du bypass est maintenant implemente dans le firmware custom.

- `0x1009` : position physique du bypass, lecture seule, `0..100 %`
- `0x100A` : mode bypass, lecture/ecriture
  - `0` = `AUTO`
  - `1` = `SHUT` / fermeture forcee
  - `2` = `OPEN` / ouverture forcee

Le registre `0x1009` a ete teste en ecriture avec une valeur intermediaire :
la box l'ignore. On l'affiche donc uniquement comme valeur de retour.

## Commandes exposees

Interface web :

- bouton `Auto`
- bouton `Ouvrir`
- bouton `Fermer`
- affichage de l'ouverture reelle en pourcentage

HTTP :

- `POST /api/set_bypass_mode?value=0`
- `POST /api/set_bypass_mode?value=1`
- `POST /api/set_bypass_mode?value=2`
- `GET /api/state` expose `bypass`, `bypass_position` et `bypass_mode`

MQTT :

- commande : `duco-vmc/set/bypass_mode`
- etats : `duco-vmc/state/bypass`, `duco-vmc/state/bypass_position`,
  `duco-vmc/state/bypass_mode`

## Limites Duco cote ecriture

La Duco impose une limite importante :

- maximum 200 ecritures par jour au total
- minimum 2 secondes entre deux ecritures

Le firmware applique ces limites pour toutes les ecritures Duco :

- changement de mode ventilation
- changement de consigne confort
- changement de mode bypass

Les lectures et les polls automatiques ne consomment pas ce quota.
Le compteur journalier est stocke en NVS sur l'ESP32 et expose dans l'interface,
l'API et MQTT.

## Historique reverse engineering

Les captures ont montre que le changement depuis le display genere un evenement
`fn=0x2C payload=07 0A`, puis les lectures de registres ont confirme que le
mode bypass utile est `0x100A`.

Anciennes valeurs observees :

- `OPEN` : `0x100A = 2`, `0x1009 = 100`
- `SHUT` : `0x100A = 1`, `0x1009 = 0`
- `AUTO` : `0x100A = 0`, position selon logique interne Duco

Les routes sniffer/debug ont ete supprimees du firmware maintenant que le mapping
est valide.
