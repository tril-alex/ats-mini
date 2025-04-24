#ifndef BUTTON_H
#define BUTTON_H

#define DEBOUNCE_TIME          50
#define SHORT_PRESS_TIME       500
#define LONG_PRESS_TIME        2000

class ButtonTracker {
  public:
    struct State {
      bool isPressed;       // Current pressed state (after debounce)
      bool wasClicked;      // Released after <0.5s press
      bool wasShortPressed; // Released after >0.5s press
      bool isLongPressed;   // Still pressed after >2s
    };

    ButtonTracker();
    State update(bool currentState);

  private:
    bool lastState; // Last raw input state
    bool lastStableState; // Last debounced state
    unsigned long lastDebounceTime;
    unsigned long pressStartTime;
};

#endif
