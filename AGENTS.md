# AGENTS.md

Fork of InnuendoPi/MQTTDevice32pIO (`origin` = Adamp457 fork, `upstream` = InnuendoPi).
ESP32/ESP8266 PlatformIO firmware that connects GPIO actors, sensors and a GGM
IDS2 induction hob to CraftBeerPi 4 over MQTT.

## Fork changes

- **CBPi heartbeat watchdog** (`src/HEARTBEAT.cpp`, `include/HEARTBEAT.h`, docs in
  `docs/heartbeat.md`). It's the on-device consumer of the heartbeat contract in
  `../cbpi4-heartbeat-watchdog/README.md`. Change it only together with that contract.
  The hooks into upstream code are deliberately small, so upstream merges stay easy:
  `Actor::Update`/`handlemqtt`, `induction::updatePower`/`handlemqtt`/`hbOff`,
  `mqttcallback`, `EM_MQTTSUBSCRIBE`, `loop`, and the `hb_timeout` config key.
- The watchdog never publishes a topic or payload taken from the beat. It only
  publishes a fixed off payload to the device's own configured topics. Keep it that way.

## Build

`pio run --environment ESP32_IDF5` (also `ESP32_IDF4`, `ESP8266`). The web UI is a
pre-gzipped blob in `include/index_htm.h`, and there's no source HTML in the repo,
so new settings go in `config.txt` rather than the UI.

## Process

Code, tests and docs freely. Ask (one line) before flashing a device that drives a
real heater or pump, or before anything that touches the production broker.
