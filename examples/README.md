# Examples — bancs de test capteur

Sketches Arduino IDE isolés, un par capteur. Utilisés comme **bancs de test**
pour valider un capteur seul avant intégration dans le firmware principal.

| Sketch           | Capteur          | Rôle                                  |
| ---------------- | ---------------- | ------------------------------------- |
| `BH1750/`        | BH1750           | luminosité (lux)                      |
| `BMP-E_280/`     | BME280 / BMP280  | température, humidité, pression       |
| `KY-037/`        | HW-484 / KY-037  | micro analogique (bruit dB)           |
| `SCD30/`         | SCD30            | CO2, température, humidité            |
| `sensor_fusion/` | tous             | banc complet de fusion + recal micro  |

## Usage

Ouvre le `.ino` correspondant dans l'Arduino IDE, sélectionne la board
`ESP32 Dev Module`, installe les libs listées en tête du fichier, puis flash.
Le moniteur série (`115200`) affiche les valeurs brutes.

> Ces sketches **ne sont pas compilés par la CI**. Ils servent de référence
> terrain et de guide de câblage. Le code de production vit dans `src/` + `lib/`.
