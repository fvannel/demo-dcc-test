# Décodeur DCC — RP2040

Socle C++17 pour un décodeur ferroviaire DCC d’accessoires sur RP2040. Le cœur de réception
est indépendant du Pico SDK et fonctionne sans allocation dynamique.

L’[architecture logicielle](docs/architecture.md) décrit les modules, les flux,
les règles de sûreté et les prochaines étapes.

## État du projet

Implémenté et testé sur PC : reconnaissance des demi-bits, assemblage des paquets
de 3 à 6 octets, contrôle XOR, récupération après erreur et compteurs de diagnostic.

Fourni pour le RP2040 : adaptateur de capture par interruptions GPIO, file bornée,
détection de perte et boucle de réception. Ce backend nécessite encore une
compilation avec le Pico SDK et une validation sur carte.

Le filtrage d’adresse, les instructions d’accessoires, les CV, les
sorties et la sécurité de puissance restent à implémenter. Le firmware actuel
compte les paquets valides et ne commande aucune charge.

## Tests sur PC

Prérequis : GNU Make et un compilateur C++17.

```sh
cd /workspace/demo-dcc-test
make test
```

Alternative avec CMake ≥ 3.13 :

```sh
cmake -S . -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

## Construction pour RP2040

Prérequis supplémentaires : toolchain Arm GNU (`arm-none-eabi-gcc/g++`),
CMake, outil de build et Raspberry Pi Pico SDK avec ses sous-modules.
Installer le SDK hors des sources, sélectionner une version officielle et
conserver son identifiant de commit pour rendre la construction reproductible.
Le SDK et la toolchain embarquée ne sont pas fournis dans ce dépôt.

```sh
export PICO_SDK_PATH=/chemin/vers/pico-sdk
cmake -S . -B build/pico -DBUILD_RP2040=ON -DPICO_BOARD=pico
cmake --build build/pico --parallel 2
```

Le résultat attendu est `build/pico/dcc_firmware.uf2`. Utiliser `PICO_BOARD=pico`
pour une carte Raspberry Pi Pico ; adapter la carte et les broches pour un
matériel personnalisé. L’entrée provisoire est **GP2**, définie dans
`src/platform/rp2040/main.cpp`.

**Ne jamais connecter la voie DCC à un GPIO.** L’entrée exige un étage de
détection et de protection adapté, avec une sortie logique de 0 à 3,3 V.
Le RP2040 ne tolère pas le 5 V. Le schéma de puissance et les valeurs des
composants devront être définis avant tout essai sur voie.
