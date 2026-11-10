#ifndef PWMSCHEDULE_H
#define PWMSCHEDULE_H

#include <stdint.h>

// On-windows within one PWM period, for n actors with requests req[] (0-100 %) and
// power-share groups grp[] (0 = none). Ungrouped actors get [0, req% of period).
// Actors sharing a group > 0 are scaled by 100/total when their requests add up to more
// than 100 %, and placed back to back with a dead-time gap after each window (including
// the wrap to the next period), so no two members of a group are ever on together.
// Pure, so it can be tested off the device: see test/test_pwmschedule.cpp.
inline void pwmSchedule(unsigned long period, unsigned long dead, uint8_t n,
                        const uint8_t *req, const uint8_t *grp,
                        unsigned long *start, unsigned long *len)
{
  for (uint8_t i = 0; i < n; i++)
  {
    start[i] = 0;
    len[i] = period * req[i] / 100;
  }
  for (uint8_t i = 0; i < n; i++)
  {
    const uint8_t g = grp[i];
    bool first = g > 0;
    for (uint8_t j = 0; j < i && first; j++)
      first = grp[j] != g;
    if (!first)
      continue;
    unsigned long total = 0;
    uint8_t active = 0;
    for (uint8_t j = i; j < n; j++)
      if (grp[j] == g)
      {
        total += req[j];
        active += req[j] > 0;
      }
    const unsigned long gap = active > 1 ? dead : 0;
    const unsigned long avail = period - active * gap;
    const unsigned long scale = total > 100 ? total : 100;
    unsigned long t = 0;
    for (uint8_t j = i; j < n; j++)
      if (grp[j] == g && req[j] > 0)
      {
        start[j] = t;
        len[j] = avail * req[j] / scale;
        t += len[j] + gap;
      }
  }
}

// Is an actor with this window on at this point of the period? (unsigned: false before start)
inline bool pwmIsOn(unsigned long phase, unsigned long start, unsigned long len)
{
  return phase - start < len;
}

#endif
