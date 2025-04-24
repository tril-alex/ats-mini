#include "Common.h"
#include "Button.h"

ButtonTracker::ButtonTracker() {
  lastState = false;
  lastStableState = false;
  lastDebounceTime = millis();
  pressStartTime = 0;
}

ButtonTracker::State ButtonTracker::update(bool currentState) {
  unsigned long now = millis();
  State result = {false, false, false, false};

  // Debounce logic
  if (currentState != lastState) {
    lastDebounceTime = now;
    lastState = currentState;
  }

  // Check if debounce period has passed
  if ((now - lastDebounceTime) > DEBOUNCE_TIME) {
    // Only consider state change if it's different from stable state
    if (currentState != lastStableState) {
      lastStableState = currentState;

      // Handle press start
      if (currentState) {
        pressStartTime = now;
      } else { // Handle release
        if ((now - pressStartTime) > SHORT_PRESS_TIME) {
          result.wasShortPressed = true;
        } else {
          result.wasClicked = true;
        }
      }
    }
  }

  // Update current pressed state
  result.isPressed = lastStableState;

  // Check for long press (still pressed)
  if (result.isPressed) {
    result.isLongPressed = ((now - pressStartTime) > LONG_PRESS_TIME);
  }
  return result;
}
