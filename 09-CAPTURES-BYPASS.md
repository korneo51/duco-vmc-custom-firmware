# Captures bypass

Journal court des observations faites pendant le reverse engineering du bypass
manuel.

## 2026-05-28

Capture utilisateur : `SHUT -> OPEN -> AUTO -> SHUT`.

Trame atypique observee :

```text
243886 *RX len=2 fn=0x2C id=0x1A payload=07 0A
```

Analyse provisoire :

- Le reste de la capture est du trafic cyclique normal `0x20`, `0x1C`, `0x08`.
- Une seule trame atypique apparait pour trois changements de bypass.
- `0x2C 07 0A` est donc un candidat evenement/notification, mais ce n'est pas
  encore une commande directe `OPEN/AUTO/SHUT`.

Prochaine capture recommandee avec le firmware courant :

1. Demarrer le sniffer.
2. Mettre le marqueur a `before-open`, cliquer `Marquer maintenant`, puis
   passer le bypass a `OPEN`.
3. Mettre le marqueur a `before-auto`, cliquer `Marquer maintenant`, puis
   passer le bypass a `AUTO`.
4. Mettre le marqueur a `before-shut`, cliquer `Marquer maintenant`, puis
   passer le bypass a `SHUT`.
5. Copier seulement le bloc atypique de l'interface web.

## Capture marquee

Sequence utilisateur :

```text
159406 SNIFFER start duration_ms=180000
171329 MARK before-open
175578 *RX len=2 fn=0x2C id=0x1F payload=07 0A  event_candidate=0x070A
191116 MARK before-shut
207414 MARK before-auto
211658 *RX len=2 fn=0x2C id=0x21 payload=07 0A  event_candidate=0x070A
217468 SNIFFER stop
```

Analyse :

- `0x2C 07 0A` apparait apres `before-open`.
- Pas de trame atypique visible apres `before-shut`.
- `0x2C 07 0A` apparait apres `before-auto`.
- Le payload reste identique, donc il ne code probablement pas directement
  `OPEN/AUTO/SHUT`.

Action firmware :

- Ajout d'une lecture debug du registre candidat `0x070A`.
- Endpoint HTTP : `GET /api/debug/read_reg?reg=070A`.
- UI : bouton `Lire registre candidat 0x070A`.
- Aucun endpoint d'ecriture n'est ajoute tant que la valeur n'est pas comprise.
- Ajout d'un verrou logiciel autour des acces UART actifs pour eviter les
  collisions entre poll, MQTT et requetes HTTP.

Lectures de controle apres OTA :

```text
070A -> payload=00 07 0A F4 DE 00 00, candidate_u16_le=57076
1009 -> payload=01 10 09 00 00 00 00, candidate_u8=0
120A -> payload=01 12 0A BE 00 00 00, candidate_u8=190
```

Interpretation :

- `0x1009` reste bien le registre d'etat bypass deja connu.
- `0x120A` reste bien la consigne confort (`0xBE` = 190 = 19.0 degC).
- `0x070A` repond, mais `F4 DE` ne ressemble pas a un enum simple
  `OPEN/AUTO/SHUT`.

Prochain test utile : lire `0x070A` dans chacun des trois etats bypass depuis
le bouton UI, sans sniffer actif, pour voir si `F4 DE` change.

## Lecture `0x070A` par etat

Lectures utilisateur :

```text
OPEN -> payload=00 07 0A FF 96 00 00, u16_le=38655
SHUT -> payload=00 07 0A E9 42 00 00, u16_le=17129
AUTO -> payload=00 07 0A DA 06 00 00, u16_le=1754
```

Lecture repetee sans changement d'etat :

```text
070A -> D8 66
070A -> D9 9A
070A -> D8 EE
070A -> D9 12
070A -> D9 56
```

Conclusion : `0x070A` est bien lisible, mais sa valeur varie meme sans changement
d'etat bypass. Ce registre ne peut donc pas etre utilise tel quel comme commande
ou etat stable `OPEN/AUTO/SHUT`.

Action firmware :

- Ajout endpoint `GET /api/debug/scan_regs?from=0700&to=0710`.
- Ajout bouton UI `Scanner 0x0700-0x0710`.
- Le scan est limite a 65 registres maximum et reste en lecture seule.

## Scan `0x0700-0x0710` par etat

Comparaison utilisateur :

- `0x0700`, `0x0701`, `0x0702`, `0x0703` restent constants.
- `0x0708` vaut `1` en `OPEN` et `SHUT`, mais a eu un timeout en `AUTO`.
- `0x0704` a `0x0710` changent tous comme des valeurs dynamiques.

Conclusion : la plage `0x0700-0x0710` ne contient pas de candidat stable pour
le mode bypass manuel.

Scan local de plages proches :

- `0x1009` = etat physique du bypass deja connu (`0` ou `100`).
- `0x100A` = petite valeur discrete observee a `2` lorsque le bypass physique
  est ouvert.
- `0x100B` = petite valeur discrete observee a `0`.
- `0x1109`, `0x120A` correspondent a des consignes connues/zone, pas au bypass.

Action firmware :

- Ajout bouton UI `Scanner 0x1000-0x1020`.

Prochain test utile : scanner `0x1000-0x1020` en `OPEN`, `SHUT`, puis `AUTO`.
La cible principale a comparer est `0x100A`.

## Scan `0x1000-0x1020` par etat

Comparaison utilisateur :

```text
OPEN -> 0x1009=100, 0x100A=2, 0x100B=0
SHUT -> 0x1009=0,   0x100A=1, 0x100B=timeout
AUTO -> 0x1009=0,   0x100A=0, 0x100B=0
```

Conclusion :

- `0x1009` est l'etat physique du bypass, deja valide (`0` ferme, `100` ouvert).
- `0x100A` encode le mode bypass manuel :
  - `0` = `AUTO`
  - `1` = `SHUT`
  - `2` = `OPEN`

Action firmware :

- Ajout `Duco::readBypassMode()` et `Duco::writeBypassMode()`.
- Ajout endpoint HTTP `POST /api/set_bypass_mode?value=0|1|2`.
- Ajout topics MQTT :
  - `duco-vmc/state/bypass_mode`
  - `duco-vmc/set/bypass_mode`
- Ajout boutons UI `Auto`, `Ouvrir`, `Fermer` dans `Pilotage bypass`.

## Position d'ouverture

Le registre `0x1009` est expose comme position physique du bypass :

- lecture : `bypass_position` dans `/api/state`
- MQTT etat : `duco-vmc/state/bypass_position`
- ecriture HTTP : non supportee, retourne `501`
- ecriture MQTT : ignoree

Test : ecrire `50` sur `0x1009` repond `OK` au niveau protocole, mais la box
retourne ensuite `100`. Ce registre est donc considere comme lecture seule pour
le moment. Le mode bypass `0x100A` reste separe : `AUTO`, `SHUT`, `OPEN`.

Correctif firmware :

- Le pilotage `0x100A` relit maintenant `0x100A` et `0x1009` juste apres la
  commande pour eviter un etat web stale.
- L'interface n'affiche plus de slider de position, seulement la valeur lue.
