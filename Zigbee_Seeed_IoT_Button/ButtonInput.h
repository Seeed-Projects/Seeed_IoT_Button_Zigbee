#pragma once

#include <stdint.h>

/**
 * Button event reported to the firmware.
 * 按键手势事件，交给固件上报 Zigbee。
 */
enum ButtonEvent {
  BUTTON_EVENT_NONE = 0,
  BUTTON_EVENT_SINGLE_CLICK = 1,
  BUTTON_EVENT_DOUBLE_CLICK = 2,
  BUTTON_EVENT_LONG_PRESS = 3,
};

/**
 * Timing knobs for debounce and click gestures (milliseconds).
 * 防抖和单击/双击/长按的时间参数（毫秒）。
 */
struct ButtonTiming {
  uint32_t debounceMs;      // Pin must stay unchanged this long before an edge is accepted
  uint32_t minPressMs;      // Shortest stable press that counts as a click
  uint32_t doubleClickMs;   // Max gap from first release to second press
  uint32_t longPressMs;     // Hold duration that emits a long-press
};

/** Factory defaults used when the sketch does not override timing. */
ButtonTiming defaultButtonTiming();

/**
 * Debounced single-button gesture detector.
 * 带防抖的单击/双击/长按检测器。
 *
 * Feed the raw pin level and current time on every poll. The detector
 * returns at most one event per update.
 * 每次轮询传入原始电平和当前时间，每次最多返回一个事件。
 */
class ButtonInput {
public:
  ButtonInput();
  explicit ButtonInput(ButtonTiming timing);

  void setTiming(ButtonTiming timing);
  ButtonTiming timing() const;

  /**
   * Advance the detector.
   * 推进检测状态。
   *
   * @param rawPressed true when the button pin reads as pressed (active-low already decoded)
   *                   true 表示按键当前按下（调用方已把低电平转换成按下）
   * @param nowMs      monotonic time in milliseconds, e.g. millis()
   *                   单调时钟，单位毫秒，例如 millis()
   * @return gesture event, or BUTTON_EVENT_NONE
   *         手势事件；无事件时返回 BUTTON_EVENT_NONE
   */
  ButtonEvent update(bool rawPressed, uint32_t nowMs);

private:
  enum State {
    kIdle,
    kPressed,
    kWaitDouble,
    kLocked,
  };

  ButtonEvent handleStableEdge(bool pressed, uint32_t nowMs);

  ButtonTiming timing_;
  bool started_;
  bool rawPressed_;
  bool stablePressed_;
  uint32_t lastRawChangeMs_;
  State state_;
  uint32_t pressStartMs_;
  uint32_t releaseMs_;
  bool longPressSent_;
};
