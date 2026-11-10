#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <Arduino.h>

// CBPi heartbeat watchdog, contract v1 of cbpi4-heartbeat-watchdog
#define cbpi4heartbeat_topic "cbpi/heartbeat"
#define HB_MIN_TIMEOUT 5000 // ms; CBPi beats once a second

extern unsigned long hbTimeout; // ms, 0 = watchdog off
extern bool hbTripped;
extern uint16_t hbPendingActors; // bit i: actors[i] still needs its retained off published
extern bool hbPendingInd;

bool hbHold();
void hbSetTimeout(unsigned long seconds);
void hbSubscribe();
void hbHandlemqtt(unsigned char *payload, unsigned int length);
void hbCheck();

#endif
