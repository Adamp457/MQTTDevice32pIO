# CBPi heartbeat watchdog

Fork feature (not upstream). Switches every actor and the induction hob off if
CraftBeerPi dies, hangs or stops while they are on. It needs no Home Assistant.

**This is not a safety system.** Fit hardwired over-temperature and dry-fire
cutouts on every element.

## Why

The MQTTDevice acts on CBPi's **retained** actor commands. If CBPi crashes or
hangs with a heater on, the device keeps the last `on` and the broker keeps
serving it. A clean CBPi stop doesn't help either, because CBPi 4.7.7 doesn't
switch MQTT actors off when it shuts down.

## Requirements

Install the [cbpi4-heartbeat-watchdog](https://github.com/Adamp457/cbpi4-heartbeat-watchdog)
plugin in CBPi. It publishes `cbpi/heartbeat` once a second, and a final
`stopped` beat on a clean shutdown. The device only reads the beat's `state`.
It ignores the beat's actor list, because it already knows its own topics.

## Enable

There is no web UI field. Set `hb_timeout` (seconds) in the `misc` section of
`config.txt` with the file explorer (`/edit`), then reboot:

```json
"misc": [{ ..., "hb_timeout": 15, ... }]
```

`0` or a missing value turns the watchdog off (upstream behaviour). The minimum
is 5 s. Saving settings in the web UI keeps the value.

`/reqMisc` reports `hb_timeout` and `hb_tripped`.

## Behaviour

| Situation | What the device does |
|---|---|
| After boot, before the first beat | Accepts commands but holds all outputs off. Holding lasts one timeout at most. |
| Beat arrives | Outputs follow CBPi. |
| No beat for `hb_timeout`, or a `stopped` beat | **Trip.** Switches every actor and the hob off locally. Clears their MQTT/sensor-error restore state. Publishes `{"state": "off", "power": 0}` **retained** to each actor's and the hob's own topic. Sounds the error alarm if a buzzer is configured. |
| While tripped | Refuses every `on`, and answers it with another retained off. This covers a stale retained `on` redelivered on reconnect. |
| Beat returns and every off has been published | Trip clears. Everything stays off until CBPi sends a new command. |

Losing WiFi or the broker also trips the watchdog, because the beats stop. If
the broker can't be reached, the retained offs are published once it's back.

Topics and payloads are never taken from the heartbeat. A forged beat can only
keep the watchdog quiet; it can't switch anything on. Use broker ACLs on
`cbpi/heartbeat` all the same.

## Limits

- **One CBPi per broker.** The topic `cbpi/heartbeat` is fixed.
- The watchdog switches **all** actors off, including pumps and any that don't
  have "switchable" set.
- With the watchdog on but the CBPi plugin missing, the device stays off.
- If CBPi is alive at boot, a retained `on` takes up to one beat (about 1 s) to
  apply.
- A hung kettle logic inside a healthy CBPi keeps the beat going, so the
  watchdog can't see it.
