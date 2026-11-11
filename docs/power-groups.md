# Power-share groups

Fork feature (not upstream). Two or more PWM actors split 100 % of one supply
between them and are never on at the same time, like Brewblox's Balancer and
Mutex. Typical use: an HLT and a boil element on one 15 A circuit
(about 3.45 kW at 230 V) that can't both run flat out.

CBPi needs no changes. Each kettle logic keeps asking for the power it wants,
and the device shares it out.

## Setup

1. Actor settings → **Power-share group**: give every actor on the shared
   circuit the same number (1–9). `0` means no group (upstream behaviour).
2. Settings → **Power-group dead time [ms]** (default 20, range 0–50): the gap
   between one member switching off and the next switching on. 20 ms is one
   50 Hz cycle, enough for a zero-cross SSR to finish switching off.
3. **actors dutycycle** sets the shared PWM period (default 5000 ms, minimum
   1000 ms).

## How power is shared

Each period, the group's requests are added up:

- **Total ≤ 100 %:** every member gets its request.
- **Total > 100 %:** every member is scaled by `100 / total`. For example,
  70 % + 60 % → 54 % + 46 %.

Each dead-time gap comes out of the period first, so with two members on, 2 ×
20 ms of every 5 s period goes unused (0.8 %). Members get back-to-back windows,
in actor-list order, with a dead-time gap after each window:

```
period 5 s:  HLT ████████░░░░░░░░░░   BK ░░░░░░░░█████████░
```

The actor list shows a scaled request as `ON: 70% → 54%`. `/reqActors` reports
`grp` and `apw` (the applied %).

## Showing applied power in CBPi

Every grouped actor publishes `{"apw": N}` to `<actor topic>/applied` each
actor tick (1 s). The message isn't retained. In CBPi, add a stock
**MQTTSensor** with Topic `<actor topic>/applied` and PayloadDictionary `apw`.
Put it on the kettle or fermenter card next to the actor. If it reads below the
actor's power, the device is throttling that actor.

## Guarantees and limits

- **No overlap within a group, by construction.** Windows never intersect, and
  the PWM loop writes every off before any on. A late PWM pass, e.g. while the
  IDS2 induction driver busy-waits about 0.2 s sending a command, can stretch a
  window. It can never start the next member early. `test/test_pwmschedule.cpp`
  sweeps every millisecond of the period to check this.
- **Resolution** is about 20 ms, i.e. 0.4 % of a 5 s period. That's up from
  20 % steps before, when actors only updated on the 1 s actor tick.
- **CBPi's actor shows the requested power**, not the scaled one. To show the
  applied power, see the section above.
- **Not a load limiter.** This firmware can't stop anything outside its own
  actors, such as a kettle on the same circuit. Size the circuit protection for
  the worst case anyway.
- Every actor now runs on one shared period start. Ungrouped actors switch on
  together at the start of each period, as before. Upstream's per-actor timing
  was in practice also near-aligned.
