# T-ESP-AuroraHome

Firmware ESP32 — collecte multi-capteurs (CO2, temp, humidité, pression, luminosité, bruit) et publication MQTT vers un hub Orange Pi.

## Structure du dépôt

```text
.
├── platformio.ini    # build PlatformIO (env: esp32dev)
├── src/              # main.cpp
├── lib/              # libs internes (capteurs, fusion, net, telemetry)
├── include/          # config globale
├── test/             # tests Unity (native + embedded)
└── examples/         # bancs de test Arduino IDE isolés par capteur
```

## Build & flash

```bash
pio run -e esp32dev            # compile
pio run -e esp32dev -t upload  # flash
pio device monitor             # serial
```

## Architecture P.O.C

```mermaid
flowchart LR

%% ===== IOT / ESP32 =====
subgraph IOT[IOT - ESP32 C++ via PlatformIO]
    A1[Capteurs: temperature, humidite, CO2, luminosite, bruit]
    A2[ESP32 - lecture capteurs + JSON + MQTT publish]
end

%% ===== ORANGE PI =====
subgraph ORANGE[Orange Pi 3 LTS - MQTT + NextJS + SQLite]
    B1[Broker MQTT - Mosquitto]
    subgraph BACK[Backend NextJS - API Routes]
        B2[Service de collecte - subscribe MQTT]
        B3[SQLite - stockage historique et dernieres valeurs]
        C1[API REST - latest, history, recommendations]
    end
    subgraph FRONT[Frontend NextJS - Dashboard]
        C2[Dashboard web - temps reel et graphes]
        D1[Recommandations simples - regles IF]
    end
end

subgraph USER[Utilisateur]
    U[Consultation du dashboard et des recommandations]
end

A1 --> A2
A2 -->|MQTT publish| B1
B1 -->|MQTT subscribe| B2
B2 --> B3
B3 --> C1
C1 --> C2
C1 --> D1
C2 --> U
D1 --> U
```

## Flux M.V.P. (provisioning)

```mermaid
flowchart TD

A1[Orange Pi démarre en mode AP]
A2[SSID généré: AuroraHome_XXXXXX<br/>XXXXXX = 6 derniers chars MAC]
A3[Mot de passe attendu = XXXXXX_AuroraHome]
A1 --> A2 --> A3

B1[ESP32 scanne les reseaux]
B2[ESP32 detecte AuroraHome_XXXXXX]
B3[ESP32 extrait XXXXXX]
B4[ESP32 construit le mdp XXXXXX_AuroraHome]
B5[ESP32 se connecte en auto a l'AP]
A3 --> B5
B1 --> B2 --> B3 --> B4 --> B5

C1[Orange Pi lance broker MQTT]
C2[ESP32 s'abonne au topic de configuration]
C3[Orange Pi publie: demande relais credentials]
B5 --> C1 --> C2 --> C3

D1[User se connecte a AuroraHome_XXXXXX]
D2[User accede a la page de config]
D3[User entre SSID + mot de passe de son WiFi personnel]
D4[Orange Pi publie les credentials via MQTT]
C3 --> D1 --> D2 --> D3 --> D4

E1[ESP32 recoit les credentials]
E2[ESP32 confirme reception via MQTT]
D4 --> E1 --> E2

F1[Orange Pi quitte mode AP]
F2[ESP32 quitte mode AP]
F3[Les deux se connectent au WiFi du user en DHCP]
E2 --> F1 --> F3
E2 --> F2 --> F3

G1[Orange Pi annonce sa presence en mDNS<br/>ex: aurorahome.local]
G2[ESP32 s'abonne de nouveau au broker MQTT sur reseau local]
F3 --> G1 --> G2

H1[ESP32 publie ses valeurs capteurs]
H2[Orange Pi stocke en SQLite]
H3[Dashboard local dispo sur http://aurorahome.local]
G2 --> H1 --> H2 --> H3
```
