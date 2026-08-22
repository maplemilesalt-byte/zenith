#pragma once
#include <cstdint>

struct AnalogStick { float x=0.0f; float y=0.0f; };
struct VirtualControllerState {
    AnalogStick leftStick;
    AnalogStick rightStick;
    float leftTrigger=0.0f, rightTrigger=0.0f;
    bool dpadUp=false, dpadDown=false, dpadLeft=false, dpadRight=false;
    bool buttonA=false, buttonB=false, buttonX=false, buttonY=false;
    bool leftBumper=false, rightBumper=false;
    bool leftStickClick=false, rightStickClick=false;
    bool view=false, menu=false;
};
class InputState {
public:
    void reset();
    void setKey(std::uint32_t key,bool pressed);
    const VirtualControllerState& controller() const { return controller_; }
private:
    void rebuildMovement();
    VirtualControllerState controller_{};
    bool w_=false,a_=false,s_=false,d_=false,up_=false,down_=false,left_=false,right_=false;
    bool space_=false,ctrl_=false,e_=false,q_=false,z_=false,c_=false,r_=false,f_=false;
    bool v_=false,g_=false,tab_=false,enter_=false,di_=false,dk_=false,dj_=false,dl_=false;
};
namespace ZenithKey {
constexpr std::uint32_t W=1,A=2,S=3,D=4,Up=5,Down=6,Left=7,Right=8,Space=9;
constexpr std::uint32_t Ctrl=10,E=11,Q=12,Z=13,C=14,R=15,F=16,V=17,G=18,Tab=19,Enter=20;
constexpr std::uint32_t DpadUp=21,DpadDown=22,DpadLeft=23,DpadRight=24;
}
