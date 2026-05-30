# Branchement de l'ESP32 sur le connecteur 12 broches

## Sécurité

**COUPER LE 230 V AU DISJONCTEUR** avant d'ouvrir le capot de la VMC. La box a
une alimentation secteur dans le boîtier. Ne jamais travailler sous tension.

Vérifier qu'aucun voyant n'est allumé après coupure et attendre 30 s pour la
décharge des condensateurs.

## Ouverture de la box

1. Couper le 230 V au disjoncteur dédié de la VMC.
2. Vérifier l'absence de tension avec un multimètre (mode V~) sur les bornes
   secteur d'entrée.
3. Retirer le capot avant (vis ou clips selon génération).
4. Identifier le PCB principal — c'est la carte avec le bouton/écran et les
   connecteurs de capteurs.

## Localisation du connecteur 12 broches

Le connecteur est celui **prévu pour la Connectivity Board officielle**. Il se
trouve généralement sur le bord du PCB, libre s'il n'y a pas de carte
optionnelle déjà installée.

Pour le repérer avec certitude :

- Regarder une vidéo d'installation de la Connectivity Board sur **duco.tv**
  (chercher "Installation Connectivity Board DucoBox Energy Comfort")
- Le connecteur est marqué dans la sérigraphie du PCB
- C'est un connecteur à 12 contacts, généralement de type header 2x6 ou 1x12

Photo annotée du brochage de référence : `kokx/duco-analysis/duco-pinout.jpg`
(repo GitHub).

## Identification des 4 broches utiles

Sur les 12 broches, on n'utilise que 4 :

| Rôle | Comment l'identifier |
|---|---|
| **GND** (masse) | Continuité avec la vis de fixation du PCB ou la masse d'un gros condensateur, multimètre en mode continuité (bip) |
| **VCC** (alim) | 5 V ou 3,3 V continu par rapport à GND, multimètre en mode V= |
| **TX (Box → carte)** | Trafic UART permanent (la box interroge en boucle ses nœuds). Au repos = niveau haut (= VCC), avec bursts |
| **RX (carte → Box)** | Plus calme au repos. Niveau haut au repos aussi |

### Méthode multimètre (sans logic analyzer)

Étapes — **box sous tension, capot ouvert, attention aux 230 V** :

1. **GND** — multimètre en mode continuité. Sonde noire sur la vis de fixation
   du PCB (qui est à la masse du châssis). Tester chaque broche du connecteur :
   celle qui bipe est GND.

2. **VCC** — multimètre en V= 20 V. Sonde noire sur GND identifié, sonde rouge
   sur chaque broche restante. Celle qui donne un 3,3 V ou 5 V stable est VCC.
   Noter la valeur.

3. **TX / RX** — multimètre en V= 20 V. Sur les broches qui restent, lire la
   tension :
   - Une broche oscillera entre VCC et ~0 V (multimètre voit une valeur
     instable, souvent entre 2-3 V en moyenne) → c'est une ligne UART
   - Pour distinguer TX de RX, sans oscillo, le plus simple est d'essayer un
     branchement et de vérifier au log ESPHome (voir étape suivante)

### Méthode logic analyzer (plus propre)

Si tu as un logic analyzer USB :

1. Brancher les sondes sur les 4 broches candidates (après identification GND
   et VCC).
2. Saleae Logic → Auto-baud detect → 57600 bauds attendus, format 8N1
3. La ligne avec du trafic constant (la box interroge ses nœuds toutes les
   secondes) = TX (de la box vers la carte fille)
4. La ligne plus calme = RX

## Câblage ESP32

Une fois les 4 broches identifiées :

| Broche connecteur Duco | Broche ESP32 DevKit | Couleur fil suggérée |
|---|---|---|
| GND | GND (n'importe quel pin GND) | Noir |
| VCC 5 V (si mesuré 5 V) | VIN ou 5V | Rouge |
| VCC 3,3 V (si mesuré 3,3 V) | 3V3 | Rouge |
| TX (box → carte) | GPIO16 (RX2) | Jaune |
| RX (carte → box) | GPIO17 (TX2) | Vert |

**Pièges classiques :**

- **TX du box se branche sur RX de l'ESP32**, et vice-versa. C'est la moitié
  des galères de cette bidouille. Si rien ne remonte au flash, swap.
- Ne **PAS** brancher VIN si la box donne du 3,3 V : tu sous-alimentes la
  carte. Et ne pas brancher 3V3 si la box donne du 5 V : tu grilles l'ESP32.
- L'ESP32 fonctionne en 3,3 V sur ses GPIO. Le bus Duco est aussi en 3,3 V
  TTL. **Aucun level shifter nécessaire.**

## Connexion sans soudure pour les tests

Première étape : **ne rien souder**. Brancher les 4 fils Dupont F-F sur le
connecteur (côté box) et sur l'ESP32, et utiliser une alim USB externe pour
l'ESP32 (ne pas brancher VCC du connecteur pour commencer).

- Cela garantit que tu peux débrancher rapidement si quelque chose cloche
- Cela évite toute interaction parasite avec l'alim de la box
- Tu valides le YAML ESPHome (étape suivante), tu vois les logs, et seulement
  ensuite tu rends la solution permanente

## Permanence

Une fois que tout marche en Dupont :

1. Couper le 230 V.
2. Soit garder le connectique Dupont avec un petit ruban adhésif pour
   sécuriser, soit souder 4 fils directement sur les pastilles du connecteur.
3. Caser l'ESP32 dans la box (côté basse tension, à l'écart des 230 V) ou
   dans un boîtier externe avec un câble.
4. Alimenter l'ESP32 soit depuis VCC du connecteur si compatible, soit par un
   chargeur USB séparé (plus simple).
5. Refermer le capot.

## Vérification finale avant de remettre le 230 V

- Aucun fil dénudé ne touche le châssis
- L'ESP32 est isolé électriquement du PCB Duco (pas de court-circuit possible)
- Les fils ne passent pas à proximité des câbles 230 V (séparation > 1 cm)
- Le capot peut se refermer sans pincer les fils
