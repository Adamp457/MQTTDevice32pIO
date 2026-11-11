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

- **Power-share groups and shared-period PWM** (`include/PWMSCHEDULE.h`, called by
  `updateActorsPWM()` in `src/AKTOREN.cpp` every 20 ms from `loop()`; docs in
  `docs/power-groups.md`). Grouped actors publish their applied % to `<actor topic>/applied`. `Actor::Update()` now only switches off. All switching on
  goes through the scheduler, which writes offs before ons. Keep it that way, because
  that ordering is the interlock.
- **Broker password not served** (`src/WEB.cpp`, `src/CONFIGFILE.cpp`): `/reqMisc`
  has no authentication, so it returns an empty `pass`; a save with an empty
  password keeps the stored one only if host, port and user are unchanged (so the
  password can't be redirected to another broker), and the debug log masks it. Clearing a password
  now needs a config reset. Built for ESP32_IDF5 2026-11-11; not yet flashed.
  Still open: every config endpoint (`/setMisc` included) is unauthenticated, and
  the upstream `/edit` file browser can serve the config file, password included
  [Likely; not checked on a device]. Keep the panel on an isolated VLAN.
- **Editable web UI**: `web/index.htm` (minified, unpacked from upstream's blob) is the
  source. `tools/build_web.py` regenerates `include/index_htm.h` before every build.
  Never hand-edit the header. Upstream gitignores `web/` and `test/`; the fork tracks both.

## Build and test

`pio run --environment ESP32_IDF5` (also `ESP32_IDF4`, `ESP8266`).

Scheduler host test:
`g++ -std=c++11 -I include test/test_pwmschedule.cpp -o /tmp/t && /tmp/t`

Inline-JS syntax check after editing `web/index.htm`: extract the `<script>` body and
run `node --check` on it.

Language strings live in `data/*.json`, which are on LittleFS, not in the firmware.
After changing them, upload the filesystem (`pio run -t uploadfs`) or the files
through `/edit`.

## Fork traps

- **WebUpdate pulls upstream.** `src/WEBUPDATE.cpp` downloads firmware and tools from
  `InnuendoPi/MQTTDevice32pIO`. On a fork-flashed device it replaces this firmware
  and silently drops every fork feature. Don't use WebUpdate on these devices.
- `build/` and the `*.zip` files are upstream's prebuilt binaries, not this fork's.
  Flash with `pio run -t upload` / `-t uploadfs`.

## Process

Code, tests and docs freely. Ask (one line) before flashing a device that drives a
real heater or pump, or before anything that touches the production broker.
