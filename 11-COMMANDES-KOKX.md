# Commandes Duco issues du repo kokx

Sources :

- `kokx/duco-analysis`
- `kokx/esphome`, branche `duco-component`, composant `esphome/components/duco`

Contribution upstream :

- PR kokx/esphome : https://github.com/kokx/esphome/pull/1
- Branche fork : `korneo51:add-duco-bypass-mode-select`

## Integrees au firmware custom

| Fonction | Trame / registre | Etat |
|---|---:|---|
| Mode ventilation | `fn=0x0C`, payload lecture `{0x02,0x01}`, ecriture `{0x04,0x01,mode}` | OK |
| Modes boost | `0x04..0x06`, `0x84..0x86`, `0xC4..0xC6` | OK |
| Consigne confort | `fn=0x24`, registre `0x120A` | OK |
| Temperatures box ODA/SUP/ETA/EHA | `fn=0x24`, registres `0x0009..0x0309` | OK |
| Bypass position | `fn=0x24`, registre `0x1009` | OK |
| Bypass mode manuel | `fn=0x24`, registre `0x100A` | OK, decouverte locale |
| Filtre restant | `fn=0x24`, registre `0x3009` | OK |
| Debit cible | `fn=0x0C`, reponse mode `data[2]` | OK |
| Temps restant mode | `fn=0x0C`, reponse mode `data[12..13]` | OK |
| Numero de serie | `fn=0x10`, payload `{0x01,0x01,0x00,0x1A,0x10}` | OK |

## Non integrees pour l'instant

| Fonction | Raison |
|---|---|
| CO2 par node | Necessite discovery/adresses de nodes fiables avant UI |
| Humidite par node | Necessite discovery/adresses de nodes fiables avant UI |
| Temperature par node externe | Necessite discovery/adresses de nodes fiables avant UI |
| Discovery nodes | Utile mais a exposer avec prudence pour ne pas charger le bus |
| Synchronisation heure Duco | Moins utile tant que le pilotage actuel fonctionne |

## Bypass

Le repo kokx expose la position bypass via `0x1009`.

Le pilotage manuel via `0x100A` n'a pas ete trouve comme commande dans le repo
kokx : il vient des captures/sniffs locaux du projet, puis validation terrain :

- `0` = auto
- `1` = fermeture forcee
- `2` = ouverture forcee

Un `select` ESPHome optionnel `bypass_mode` a ete propose upstream pour partager
ce mapping.

## Modes ventilation

Chez kokx, `MAN1`, `MAN2`, `MAN3` designent les niveaux de puissance. Les
suffixes `x2` et `x3` multiplient la duree par defaut de 15 minutes.

| Code | Libelle interface |
|---:|---|
| `0` | Auto |
| `4` | Boost P1 15 min |
| `5` | Boost P2 15 min |
| `6` | Boost P3 15 min |
| `7` | Absent |
| `8` | Puissance 1 |
| `9` | Puissance 2 |
| `10` | Puissance 3 |
| `132` / `0x84` | Boost P1 30 min |
| `133` / `0x85` | Boost P2 30 min |
| `134` / `0x86` | Boost P3 30 min |
| `196` / `0xC4` | Boost P1 45 min |
| `197` / `0xC5` | Boost P2 45 min |
| `198` / `0xC6` | Boost P3 45 min |
