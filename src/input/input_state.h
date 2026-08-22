#pragma once

#include <cstdint>

struct AnalogStick { float x=0.0f; float y=0.0f; };
struct VirtualControllerState {
    AnalogStick leftStick;
    AnalogStick rightStick;
    bool jump=false;
    bool dpadUp=false, dpadDown=false, dpadLeft=false, dpadRight=false;
    bool buttonA=false, buttonB=false, buttonX=false, buttonY=false;
};
class InputState {
public:
    void reset();
    void setKey(std::uint32_t key,bool pressed);
    const VirtualControllerState& controller() const { return controller_; }
private:
    void rebuildMovement();
    VirtualControllerState controller_{};
    bool w_=false,a_=false,s_=false,d_=false,up_=false,down_=false,left_=false,right_=false,space_=false;
};
namespace ZenithKey {
constexpr std::uint32_t W=1,A=2,S=3,D=4,Up=5,Down=6,Left=7,Right=8,Space=9;
} 
