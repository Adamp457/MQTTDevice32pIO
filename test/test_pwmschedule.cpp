// Host check for pwmSchedule():  g++ -std=c++11 -I include test/test_pwmschedule.cpp -o /tmp/t && /tmp/t
#include <assert.h>
#include <stdio.h>
#include "PWMSCHEDULE.h"

static const unsigned long P = 5000, D = 20;

// Sweep two periods ms by ms: at most one member of a group is on, and when the group
// hands over from one member to another there are at least D ms with nobody on.
static void checkNoOverlap(uint8_t n, const uint8_t *req, const uint8_t *grp)
{
  unsigned long s[15], l[15];
  pwmSchedule(P, D, n, req, grp, s, l);
  for (uint8_t g = 1; g < 4; g++)
  {
    int last = -1;          // member last seen on
    unsigned long idle = 0; // ms with nobody on since then
    for (unsigned long ph = 0; ph < 2 * P; ph++)
    {
      int on = -1, count = 0;
      for (uint8_t i = 0; i < n; i++)
        if (grp[i] == g && pwmIsOn(ph % P, s[i], l[i]))
          on = i, count++;
      assert(count <= 1);
      if (on < 0)
        idle++;
      else
      {
        if (last >= 0 && on != last && ph >= P) // first period only warms up 'last'
          assert(idle >= D);
        last = on;
        idle = 0;
      }
    }
  }
}

int main()
{
  unsigned long s[15], l[15];

  { // ungrouped: unchanged duty
    uint8_t req[] = {70, 60}, grp[] = {0, 0};
    pwmSchedule(P, D, 2, req, grp, s, l);
    assert(l[0] == 3500 && l[1] == 3000 && s[0] == 0 && s[1] == 0);
  }
  { // grouped, total <= 100: requests kept (less dead time), back to back with a gap
    uint8_t req[] = {30, 30}, grp[] = {1, 1};
    pwmSchedule(P, D, 2, req, grp, s, l);
    assert(l[0] == (P - 2 * D) * 30 / 100 && l[1] == l[0]);
    assert(s[1] == s[0] + l[0] + D);
  }
  { // grouped, 70 + 60 = 130: scaled to 100 % of what's available, ~54/46
    uint8_t req[] = {70, 60}, grp[] = {1, 1};
    pwmSchedule(P, D, 2, req, grp, s, l);
    assert(l[0] == (P - 2 * D) * 70 / 130 && l[1] == (P - 2 * D) * 60 / 130);
    assert(s[1] + l[1] + D <= P); // gap before the wrap too
  }
  { // grouped, one off: the other gets its full request, no dead time needed
    uint8_t req[] = {100, 0}, grp[] = {1, 1};
    pwmSchedule(P, D, 2, req, grp, s, l);
    assert(l[0] == P && l[1] == 0);
  }
  { // two groups and an ungrouped actor don't affect each other
    uint8_t req[] = {80, 50, 90, 90, 40}, grp[] = {1, 0, 2, 2, 1};
    pwmSchedule(P, D, 5, req, grp, s, l);
    assert(l[1] == 2500);
    assert(l[2] == l[3] && l[2] == (P - 2 * D) * 90 / 180);
  }
  uint8_t cases[][4] = {{100, 100, 0, 0}, {70, 60, 0, 0}, {30, 30, 0, 0}, {100, 100, 100, 0}, {1, 99, 50, 50}, {100, 0, 100, 0}};
  uint8_t grp[] = {1, 1, 2, 2};
  uint8_t grp3[] = {1, 1, 1, 0};
  for (auto &c : cases)
  {
    checkNoOverlap(4, c, grp);
    checkNoOverlap(4, c, grp3);
  }
  printf("pwmSchedule: all checks passed\n");
  return 0;
}
