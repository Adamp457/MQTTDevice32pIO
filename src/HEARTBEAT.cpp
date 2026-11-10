#include "MQTTDevice.h"

// CBPi heartbeat watchdog. CBPi (with the cbpi4-heartbeat-watchdog plugin) publishes
// cbpi/heartbeat once a second. If it goes silent for hbTimeout, or says "stopped",
// every actor and the induction hob are switched off here and their off payload is
// published retained, so the broker can't hand a stale "on" back later.
//
// While held (no beat yet since boot, or tripped) outputs are forced off. After a trip,
// "on" commands are refused (and answered with a retained off) until a fresh beat
// arrives and every off has been published. Commands then apply as normal.

unsigned long hbTimeout = 0;
bool hbTripped = false;
uint16_t hbPendingActors = 0;
bool hbPendingInd = false;

static unsigned long hbLast = 0; // last running beat; 0 = boot, so boot gets one timeout of grace
static bool hbSeen = false;      // a running beat since boot or since the trip
static bool hbStopped = false;

static const char *HB_OFF = "{\"state\": \"off\", \"power\": 0}"; // what CBPi's MQTTActor publishes

bool hbHold()
{
  return hbTimeout > 0 && (hbTripped || !hbSeen);
}

void hbSetTimeout(unsigned long seconds) // 0 = off
{
  unsigned long ms = seconds * 1000UL;
  if (ms > 0 && ms < HB_MIN_TIMEOUT)
    ms = HB_MIN_TIMEOUT;
  if (ms > 0 && hbTimeout == 0)
    hbLast = millis(); // switched on while running: one timeout of grace, not an instant trip
  if (ms == 0)
  {
    hbTripped = false; // switched off: stop refusing "on"
    hbPendingActors = 0;
    hbPendingInd = false;
  }
  hbTimeout = ms;
  hbSubscribe();
}

void hbSubscribe()
{
  if (hbTimeout > 0 && pubsubClient.connected())
    pubsubClient.subscribe(cbpi4heartbeat_topic);
}

void hbHandlemqtt(unsigned char *payload, unsigned int length)
{
  JsonDocument doc;
  JsonDocument filter;
  filter["state"] = true;
  if (deserializeJson(doc, (const char *)payload, length, DeserializationOption::Filter(filter)))
    return;
  if (doc["state"] == "stopped")
    hbStopped = true;
  else if (doc["state"] == "running")
  {
    hbLast = millis();
    hbSeen = true;
    hbStopped = false;
  }
}

static void hbTrip()
{
  hbTripped = true;
  hbSeen = false;
#ifdef ESP32
  DEBUG_ERROR("SYS", "CBPi heartbeat %s: switching all actors and induction off", hbStopped ? "stopped" : "lost");
#endif
  hbStopped = false;
  // Everything, not just what's on: also clears any stale retained "on" on the broker.
  for (uint8_t i = 0; i < numberOfActors; i++)
  {
    actors[i].setIsOn(false);
    actors[i].setIsOnBeforeError(false); // stop MQTT/sensor error recovery switching it back on
    actors[i].setActorState(true);
    actors[i].Update();
    hbPendingActors |= 1 << i;
  }
  if (inductionCooker.getIsEnabled())
  {
    inductionCooker.hbOff();
    hbPendingInd = true;
  }
  if (PIN_BUZZER != -100)
    sendAlarm(ALARM_ERROR);
}

static void hbPublishOffs()
{
  hbPendingActors &= (1 << numberOfActors) - 1; // actors deleted since the trip
  for (uint8_t i = 0; i < numberOfActors; i++)
  {
    if (!(hbPendingActors & (1 << i)))
      continue;
    if (actors[i].getActorTopic().length() > 0 && !pubsubClient.publish(actors[i].getActorTopic().c_str(), HB_OFF, true))
      return; // retry next pass
    hbPendingActors &= ~(1 << i);
  }
  if (hbPendingInd && (inductionCooker.getTopic().length() == 0 || pubsubClient.publish(inductionCooker.getTopic().c_str(), HB_OFF, true)))
    hbPendingInd = false;
}

void hbCheck()
{
  if (hbTimeout == 0)
    return;
  if (!hbTripped && (hbStopped || millis() - hbLast >= hbTimeout))
    hbTrip();
  if (!hbTripped)
    return;
  if ((hbPendingActors || hbPendingInd) && pubsubClient.connected())
    hbPublishOffs();
  if (hbSeen && !hbPendingActors && !hbPendingInd)
  {
    hbTripped = false;
#ifdef ESP32
    DEBUG_INFO("SYS", "CBPi heartbeat back: actors follow CBPi again");
#endif
  }
}
