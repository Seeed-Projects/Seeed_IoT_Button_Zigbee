#include "ButtonInput.h"

#include <stdio.h>
#include <stdlib.h>

static int g_failed = 0;
static int g_passed = 0;

#define EXPECT_EQ(actual, expected, label)                                                     \
  do {                                                                                         \
    long _a = (long)(actual);                                                                  \
    long _e = (long)(expected);                                                                \
    if (_a != _e) {                                                                            \
      printf("FAIL  %s: got %ld expected %ld\n", (label), _a, _e);                             \
      g_failed++;                                                                              \
    } else {                                                                                   \
      printf("PASS  %s\n", (label));                                                           \
      g_passed++;                                                                              \
    }                                                                                          \
  } while (0)


/**
 * Drive the detector from t0 to tEnd at a fixed poll interval.
 * 按固定轮询间隔，从 t0 跑到 tEnd。
 *
 * pressedAt(t) returns the raw pin level at time t.
 */
template <typename PressedFn>
static ButtonEvent runWindow(ButtonInput &button, uint32_t t0, uint32_t tEnd, uint32_t pollMs,
                             PressedFn pressedAt, ButtonEvent *firstEvent, uint32_t *firstEventMs) {
  ButtonEvent seen = BUTTON_EVENT_NONE;
  *firstEvent = BUTTON_EVENT_NONE;
  *firstEventMs = 0;
  for (uint32_t t = t0; t <= tEnd; t += pollMs) {
    ButtonEvent event = button.update(pressedAt(t), t);
    if (event != BUTTON_EVENT_NONE) {
      if (seen == BUTTON_EVENT_NONE) {
        *firstEvent = event;
        *firstEventMs = t;
      }
      seen = event;
    }
  }
  return seen;
}

static bool pressBetween(uint32_t t, uint32_t onMs, uint32_t offMs) {
  return t >= onMs && t < offMs;
}

static void test_single_click() {
  ButtonInput button;
  ButtonEvent first = BUTTON_EVENT_NONE;
  uint32_t at = 0;
  auto pressedAt = [](uint32_t t) { return pressBetween(t, 100, 180); };
  ButtonEvent last = runWindow(button, 0, 900, 10, pressedAt, &first, &at);
  EXPECT_EQ(first, BUTTON_EVENT_SINGLE_CLICK, "single click is reported after the double-click window");
  EXPECT_EQ(last, BUTTON_EVENT_SINGLE_CLICK, "single click is the only gesture");
  if (at < 100 + 20 + 600) {
    printf("FAIL  single click fired too early at %u ms\n", at);
    g_failed++;
  } else {
    printf("PASS  single click waited for the double-click window (t=%u)\n", at);
    g_passed++;
  }
}

static void test_double_click_comfortable_gap() {
  ButtonInput button;
  ButtonEvent first = BUTTON_EVENT_NONE;
  uint32_t at = 0;
  // First click 80ms, 450ms gap, second click 80ms. The old 400ms window treated this as single.
  auto pressedAt = [](uint32_t t) {
    return pressBetween(t, 50, 130) || pressBetween(t, 580, 660);
  };
  ButtonEvent last = runWindow(button, 0, 900, 10, pressedAt, &first, &at);
  EXPECT_EQ(first, BUTTON_EVENT_DOUBLE_CLICK, "450ms gap still counts as double click");
  EXPECT_EQ(last, BUTTON_EVENT_DOUBLE_CLICK, "double click does not also emit single click");
}

static void test_double_click_fast() {
  ButtonInput button;
  ButtonEvent first = BUTTON_EVENT_NONE;
  uint32_t at = 0;
  auto pressedAt = [](uint32_t t) {
    return pressBetween(t, 20, 90) || pressBetween(t, 160, 230);
  };
  ButtonEvent last = runWindow(button, 0, 500, 10, pressedAt, &first, &at);
  EXPECT_EQ(first, BUTTON_EVENT_DOUBLE_CLICK, "fast double click is detected");
  EXPECT_EQ(last, BUTTON_EVENT_DOUBLE_CLICK, "fast double click does not emit single");
}

static void test_short_second_click_with_fine_poll() {
  ButtonInput button;
  ButtonEvent first = BUTTON_EVENT_NONE;
  uint32_t at = 0;
  // 50ms second press is easy to miss with a 100ms loop delay.
  auto pressedAt = [](uint32_t t) {
    return pressBetween(t, 10, 90) || pressBetween(t, 180, 230);
  };
  runWindow(button, 0, 500, 10, pressedAt, &first, &at);
  EXPECT_EQ(first, BUTTON_EVENT_DOUBLE_CLICK, "50ms second press is seen at 10ms poll");
}

static void test_long_press() {
  ButtonInput button;
  ButtonEvent first = BUTTON_EVENT_NONE;
  uint32_t at = 0;
  auto pressedAt = [](uint32_t t) { return t >= 50 && t < 1400; };
  runWindow(button, 0, 1500, 10, pressedAt, &first, &at);
  EXPECT_EQ(first, BUTTON_EVENT_LONG_PRESS, "held button emits long press");
  if (at < 1000) {
    printf("FAIL  long press fired too early at %u ms\n", at);
    g_failed++;
  } else {
    printf("PASS  long press waited for hold time (t=%u)\n", at);
    g_passed++;
  }
}

static void test_bounce_during_hold_still_long_press() {
  ButtonInput button;
  ButtonEvent first = BUTTON_EVENT_NONE;
  uint32_t at = 0;
  auto pressedAt = [](uint32_t t) {
    if (t < 40) {
      return false;
    }
    // 8ms chatter around t=300, shorter than debounce.
    if (t >= 300 && t < 308) {
      return false;
    }
    return t < 1400;
  };
  ButtonEvent last = runWindow(button, 0, 1500, 2, pressedAt, &first, &at);
  EXPECT_EQ(first, BUTTON_EVENT_LONG_PRESS, "bounce during hold does not cancel long press");
  EXPECT_EQ(last, BUTTON_EVENT_LONG_PRESS, "bounce during hold does not emit a click");
}

static void test_release_bounce_is_not_double_click() {
  ButtonInput button;
  ButtonEvent first = BUTTON_EVENT_NONE;
  uint32_t at = 0;
  auto pressedAt = [](uint32_t t) {
    if (pressBetween(t, 40, 140)) {
      return true;
    }
    // 8ms bounce after release.
    if (pressBetween(t, 148, 156)) {
      return true;
    }
    return false;
  };
  ButtonEvent last = runWindow(button, 0, 900, 2, pressedAt, &first, &at);
  EXPECT_EQ(first, BUTTON_EVENT_SINGLE_CLICK, "release bounce is not a double click");
  EXPECT_EQ(last, BUTTON_EVENT_SINGLE_CLICK, "release bounce yields a single click only");
}

static void test_min_press_ignored() {
  ButtonTiming timing = defaultButtonTiming();
  ButtonInput button(timing);
  ButtonEvent first = BUTTON_EVENT_NONE;
  uint32_t at = 0;
  // 25ms press: longer than debounce (20ms), shorter than min_press (30ms).
  auto pressedAt = [](uint32_t t) { return pressBetween(t, 50, 75); };
  ButtonEvent last = runWindow(button, 0, 800, 1, pressedAt, &first, &at);
  EXPECT_EQ(first, BUTTON_EVENT_NONE, "press shorter than min_press is ignored");
  EXPECT_EQ(last, BUTTON_EVENT_NONE, "ignored press emits no later event");
}

static void test_after_double_click_hold_is_locked() {
  ButtonInput button;
  ButtonEvent events[8];
  int n = 0;
  auto pressedAt = [](uint32_t t) {
    return pressBetween(t, 20, 90) || (t >= 160 && t < 1500);
  };
  for (uint32_t t = 0; t <= 1600; t += 10) {
    ButtonEvent event = button.update(pressedAt(t), t);
    if (event != BUTTON_EVENT_NONE && n < 8) {
      events[n++] = event;
    }
  }
  EXPECT_EQ(n, 1, "second press hold does not start another gesture");
  if (n >= 1) {
    EXPECT_EQ(events[0], BUTTON_EVENT_DOUBLE_CLICK, "locked hold after double click");
  } else {
    EXPECT_EQ(0, 1, "locked hold after double click");
  }
}

static void test_custom_double_click_window() {
  ButtonTiming timing = defaultButtonTiming();
  timing.doubleClickMs = 300;
  ButtonInput button(timing);
  ButtonEvent first = BUTTON_EVENT_NONE;
  uint32_t at = 0;
  auto pressedAt = [](uint32_t t) {
    return pressBetween(t, 50, 120) || pressBetween(t, 500, 580);
  };
  ButtonEvent last = runWindow(button, 0, 1000, 10, pressedAt, &first, &at);
  EXPECT_EQ(first, BUTTON_EVENT_SINGLE_CLICK, "custom 300ms window treats 380ms gap as single");
  EXPECT_EQ(last, BUTTON_EVENT_SINGLE_CLICK, "late second press starts a new click, not a double");
}

static void test_custom_long_press() {
  ButtonTiming timing = defaultButtonTiming();
  timing.longPressMs = 400;
  ButtonInput button(timing);
  ButtonEvent first = BUTTON_EVENT_NONE;
  uint32_t at = 0;
  auto pressedAt = [](uint32_t t) { return t >= 20 && t < 800; };
  runWindow(button, 0, 900, 10, pressedAt, &first, &at);
  EXPECT_EQ(first, BUTTON_EVENT_LONG_PRESS, "custom long-press time is honored");
  if (at > 500) {
    printf("FAIL  custom long press fired late at %u ms\n", at);
    g_failed++;
  } else {
    printf("PASS  custom long press fired at t=%u\n", at);
    g_passed++;
  }
}

int main() {
  test_single_click();
  test_double_click_comfortable_gap();
  test_double_click_fast();
  test_short_second_click_with_fine_poll();
  test_long_press();
  test_bounce_during_hold_still_long_press();
  test_release_bounce_is_not_double_click();
  test_min_press_ignored();
  test_after_double_click_hold_is_locked();
  test_custom_double_click_window();
  test_custom_long_press();

  printf("\n%d passed, %d failed\n", g_passed, g_failed);
  return g_failed == 0 ? 0 : 1;
}
