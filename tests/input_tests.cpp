#include "input/input_state.h"
#include <cassert>
#include <cmath>

static void near(float actual,float expected){ assert(std::fabs(actual-expected)<0.001f); }

int main(){
    InputState input;
    input.reset();

    input.setGamepadButton(ZenithGamepad::A,true);
    assert(input.controller().buttonA);
    assert(input.controller().jump);

    input.setGamepadButton(ZenithGamepad::LB,true);
    input.setGamepadButton(ZenithGamepad::Menu,true);
    assert(input.controller().leftBumper);
    assert(input.controller().menu);

    input.setGamepadButton(ZenithGamepad::DpadRight,true);
    assert(input.controller().dpadRight);

    input.setGamepadAxis(ZenithGamepad::LeftX,0.75f);
    input.setGamepadAxis(ZenithGamepad::LeftY,-0.5f);
    input.setGamepadAxis(ZenithGamepad::RightX,-1.0f);
    input.setGamepadAxis(ZenithGamepad::RightY,1.0f);
    near(input.controller().leftStick.x,0.75f);
    near(input.controller().leftStick.y,-0.5f);
    near(input.controller().rightStick.x,-1.0f);
    near(input.controller().rightStick.y,1.0f);

    input.setGamepadAxis(ZenithGamepad::LeftTrigger,-1.0f);
    input.setGamepadAxis(ZenithGamepad::RightTrigger,1.0f);
    near(input.controller().leftTrigger,0.0f);
    near(input.controller().rightTrigger,1.0f);

    input.reset();
    assert(!input.controller().buttonA);
    assert(!input.controller().leftBumper);
    near(input.controller().leftStick.x,0.0f);
    near(input.controller().rightTrigger,0.0f);
    return 0;
}
