# Diagnostic du bypass — à faire AVANT d'acheter quoi que ce soit

Cette checklist se fait depuis le menu display de la box, sans matériel
supplémentaire. Le but est de comprendre POURQUOI le bypass ne s'ouvre pas
avant de se lancer dans la bidouille.

## Rappel logique bypass

Le bypass s'ouvre uniquement si les **3 conditions cumulatives** sont vraies :

1. T-ETA (air extrait) > T-confort (consigne)
2. T-ODA (air extérieur) > 10 °C
3. T-ODA < T-ETA (l'extérieur est réellement plus frais que l'intérieur)

## Étape 1 — Lire les 4 températures internes

Dans le menu display (bouton de la box), naviguer dans le menu de mesures. Tu
dois pouvoir lire :

- **T-ODA** — Outdoor Air, l'air qui entre depuis dehors
- **T-SUP** — Supply Air, l'air soufflé dans les pièces de vie
- **T-ETA** — Extract Air, l'air repris des pièces humides (≈ température
  intérieure moyenne)
- **T-EHA** — Exhaust Air, l'air rejeté à l'extérieur

Note-les **une nuit où le problème se manifeste** (26 °C intérieur, bypass
fermé). Compare-les à un thermomètre extérieur que tu connais et au
thermomètre du salon.

### Interprétation

| Constat | Diagnostic |
|---|---|
| ODA > ETA | Bypass volontairement fermé, comportement normal. Solution : ouvrir les fenêtres ou ajouter une clim, le bypass ne fait pas de miracle |
| ODA < ETA et ODA > 10 et ETA > 20, mais bypass fermé | **Anomalie**, passer à l'étape 2 |
| ODA très différente du thermomètre extérieur réel | Capteur ODA mal placé ou faussé, passer à l'étape 4 |
| ETA très différente de la température salon | Capteur ETA dans une zone chaude (proche cuisine/SDB), à prendre en compte |

## Étape 2 — Vérifier les codes erreur

Toujours dans le menu display, chercher la liste des erreurs actives. Codes à
surveiller :

- **3.1.x, 4.1.x, 6.1.x, 30.1.x** — Erreurs de sondes de température
  (sonde non lue ou connexion coupée)
- **Erreurs liées au servomoteur du bypass** — voir manuel d'erreurs Duco
  (référencé dans le README)

Si une de ces erreurs est présente : c'est un problème matériel, pas de
configuration. Contacter l'installateur ou Duco.

## Étape 3 — Vérifier le côté L/R du bypass

La box contient **deux trappes bypass** (gauche et droite). Une seule est
active selon le réglage fait à l'installation. Si l'installateur a coché le
mauvais côté, la trappe pilotée est figée fermée.

Dans le menu installateur (code requis, souvent 9876 ou 1234, sinon demander
à l'installateur) :

- Chercher le réglage "Bypass position" ou similaire
- Vérifier qu'il correspond bien au côté physiquement présent (visible à
  l'œil dans la box, après dépose du capot)

## Étape 4 — Forcer le bypass manuellement

Toujours dans le menu installateur, il y a un mode test/service qui permet
de **commander le bypass à la main** (ouvert / fermé).

Procédure :

1. Commander le bypass à "ouvert"
2. Écouter le servomoteur — il doit faire un petit bruit pendant ~10 s
3. Inspecter visuellement la trappe (capot ouvert) : elle doit avoir bougé
4. Commander à "fermé" et vérifier qu'elle revient

Si le servo ne bouge pas ou bourdonne : la trappe est mécaniquement bloquée
(poussière, grippage après hiver). Solution : nettoyer + lubrifier sec.

Si le servo bouge mais que le bypass ne fait pas son effet : problème côté
logique ou capteurs, passer à l'étape 5.

## Étape 5 — Vérifier le zonage et la consigne

Si la box est configurée en bi-zone (jour/nuit ou rdc/étage), il y a **deux
consignes confort** (zone 1 et zone 2). La consigne "20" que tu as réglée
est peut-être celle d'une seule zone.

Dans le menu, vérifier :

- Y a-t-il une zone 2 ? (oui si tu as deux clapets motorisés type ZSV)
- Quelle est la consigne de chaque zone ?
- Le bypass utilise la consigne de la zone correspondante à l'extraction

## Étape 6 — Recalibrer les sondes (si écart)

Si T-ODA lue diffère de >2 °C d'un thermomètre indépendant placé près de la
prise d'air, il faut soit :

- Recalibrer dans le menu (option "Sensor offset" ou équivalent)
- Faire intervenir l'installateur pour vérifier le câblage/sonde

## Conclusion du diagnostic

- **Si l'étape 1 montre ODA > ETA** : c'est physique, aucune bidouille ne
  résoudra. Inutile d'aller plus loin sur la voie domotique pour ça.
- **Si une étape 2-6 révèle un défaut** : c'est de la maintenance Duco,
  inutile d'aller plus loin sur la voie domotique tant que ce n'est pas
  réparé.
- **Si tout est OK mais que tu veux quand même un pilotage fin** (par
  exemple forcer un free-cooling agressif sur fenêtre horaire) : on passe
  au branchement (`03-BRANCHEMENT.md`).
