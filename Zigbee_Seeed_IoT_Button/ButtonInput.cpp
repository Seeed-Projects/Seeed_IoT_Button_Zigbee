#include "ButtonInput.h"

ButtonTiming defaultButtonTiming() {
  ButtonTiming timing;
  timing.debounceMs = 20;     // typical mechanical bounce settles within ~5-15ms
  timing.minPressMs = 30;     // real clicks last longer than residual chatter
  timing.doubleClickMs = 600; // gap from first release to second press
  timing.longPressMs = 1000;  // hold duration for long press / factory reset
  return timing;
}

ButtonInput::ButtonInput() : ButtonInput(defaultButtonTiming()) {}

ButtonInput::ButtonInput(ButtonTiming timing)
  : timing_(timing),
    started_(false),
    rawPressed_(false),
    stablePressed_(false),
    lastRawChangeMs_(0),
    state_(kIdle),
    pressStartMs_(0),
    releaseMs_(0),
    longPressSent_(false) {}

void ButtonInput::setTiming(ButtonTiming timing) {
  timing_ = timing;
}

ButtonTiming ButtonInput::timing() const {
  return timing_;
}

ButtonEvent ButtonInput::update(bool rawPressed, uint32_t nowMs) {
  if (!started_) {
    rawPressed_ = rawPressed;
    stablePressed_ = rawPressed;
    lastRawChangeMs_ = nowMs;
    started_ = true;
  }

  if (rawPressed != rawPressed_) {
    rawPressed_ = rawPressed;
    lastRawChangeMs_ = nowMs;
  }

  ButtonEvent edgeEvent = BUTTON_EVENT_NONE;
  if ((nowMs - lastRawChangeMs_) >= timing_.debounceMs && rawPressed_ != stablePressed_) {
    stablePressed_ = rawPressed_;
    edgeEvent = handleStableEdge(stablePressed_, nowMs);
  }

  if (state_ == kPressed && stablePressed_ && !longPressSent_ &&
      (nowMs - pressStartMs_) >= timing_.longPressMs) {
    longPressSent_ = true;
    state_ = kLocked;
    return BUTTON_EVENT_LONG_PRESS;
  }

  if (state_ == kWaitDouble && (nowMs - releaseMs_) >= timing_.doubleClickMs) {
    state_ = kIdle;
    return BUTTON_EVENT_SINGLE_CLICK;
  }

  return edgeEvent;
}

ButtonEvent ButtonInput::handleStableEdge(bool pressed, uint32_t nowMs) {
  if (pressed) {
    if (state_ == kIdle) {
      state_ = kPressed;
      pressStartMs_ = nowMs;
      longPressSent_ = false;
      return BUTTON_EVENT_NONE;
    }
    if (state_ == kWaitDouble) {
      state_ = kLocked;
      return BUTTON_EVENT_DOUBLE_CLICK;
    }
    return BUTTON_EVENT_NONE;
  }

  if (state_ == kPressed) {
    uint32_t heldMs = nowMs - pressStartMs_;
    if (longPressSent_ || heldMs < timing_.minPressMs) {
      state_ = kIdle;
    } else {
      state_ = kWaitDouble;
      releaseMs_ = nowMs;
    }
    return BUTTON_EVENT_NONE;
  }

  if (state_ == kLocked) {
    state_ = kIdle;
  }
  return BUTTON_EVENT_NONE;
}
