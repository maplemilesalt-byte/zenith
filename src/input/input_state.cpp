#include "input_state.h"
#include <algorithm>
#include <cmath>

void InputState::reset(){
    controller_={}; gamepadLeftStick_={}; gamepadRightStick_={}; gamepadLeftTrigger_=gamepadRightTrigger_=0.0f;
    gamepadDpadUp_=gamepadDpadDown_=gamepadDpadLeft_=gamepadDpadRight_=false;
    gamepadA_=gamepadB_=gamepadX_=gamepadY_=false; gamepadLB_=gamepadRB_=gamepadLS_=gamepadRS_=gamepadView_=gamepadMenu_=false;
    w_=a_=s_=d_=up_=down_=left_=right_=space_=ctrl_=e_=q_=z_=c_=r_=f_=v_=g_=tab_=enter_=di_=dk_=dj_=dl_=false;
    rebuildMovement();
}
void InputState::rebuildMovement(){
    float x=(d_?1.0f:0.0f)-(a_?1.0f:0.0f);
    float y=(s_?1.0f:0.0f)-(w_?1.0f:0.0f);
    const float len=std::sqrt(x*x+y*y);
    if(len>1.0f){x/=len;y/=len;}
    controller_.leftStick=(std::abs(gamepadLeftStick_.x)>0.02f||std::abs(gamepadLeftStick_.y)>0.02f)?gamepadLeftStick_:AnalogStick{x,y};
    controller_.rightStick=(std::abs(gamepadRightStick_.x)>0.02f||std::abs(gamepadRightStick_.y)>0.02f)?gamepadRightStick_:AnalogStick{(right_?1.0f:0.0f)-(left_?1.0f:0.0f),(down_?1.0f:0.0f)-(up_?1.0f:0.0f)};
    controller_.leftTrigger=std::max(gamepadLeftTrigger_,r_?1.0f:0.0f);
    controller_.rightTrigger=std::max(gamepadRightTrigger_,f_?1.0f:0.0f);
    controller_.jump=space_||gamepadA_;
    controller_.buttonA=space_||gamepadA_;
    controller_.buttonB=ctrl_||gamepadB_;
    controller_.buttonX=e_||gamepadX_;
    controller_.buttonY=q_||gamepadY_;
    controller_.leftBumper=z_||gamepadLB_;
    controller_.rightBumper=c_||gamepadRB_;
    controller_.leftStickClick=v_||gamepadLS_;
    controller_.rightStickClick=g_||gamepadRS_;
    controller_.view=tab_||gamepadView_;
    controller_.menu=enter_||gamepadMenu_;
    controller_.dpadUp=di_||gamepadDpadUp_;
    controller_.dpadDown=dk_||gamepadDpadDown_;
    controller_.dpadLeft=dl_||gamepadDpadLeft_;
    controller_.dpadRight=dl_||gamepadDpadRight_;
}
void InputState::setKey(std::uint32_t key,bool pressed){
    switch(key){
        case ZenithKey::W:w_=pressed;break;case ZenithKey::A:a_=pressed;break;case ZenithKey::S:s_=pressed;break;case ZenithKey::D:d_=pressed;break;
        case ZenithKey::Up:up_=pressed;break;case ZenithKey::Down:down_=pressed;break;case ZenithKey::Left:left_=pressed;break;case ZenithKey::Right:right_=pressed;break;
        case ZenithKey::Space:space_=pressed;break;case ZenithKey::Ctrl:ctrl_=pressed;break;case ZenithKey::E:e_=pressed;break;case ZenithKey::Q:q_=pressed;break;
        case ZenithKey::Z:z_=pressed;break;case ZenithKey::C:c_=pressed;break;case ZenithKey::R:r_=pressed;break;case ZenithKey::F:f_=pressed;break;
        case ZenithKey::V:v_=pressed;break;case ZenithKey::G:g_=pressed;break;case ZenithKey::Tab:tab_=pressed;break;case ZenithKey::Enter:enter_=pressed;break;
        case ZenithKey::DpadUp:di_=pressed;break;case ZenithKey::DpadDown:dk_=pressed;break;case ZenithKey::DpadLeft:dj_=pressed;break;case ZenithKey::DpadRight:dl_=pressed;break;
        default:return;
    }
    rebuildMovement();
}
void InputState::setGamepadButton(std::uint32_t button,bool pressed){
    switch(button){
        case ZenithGamepad::A:gamepadA_=pressed;break;case ZenithGamepad::B:gamepadB_=pressed;break;case ZenithGamepad::X:gamepadX_=pressed;break;case ZenithGamepad::Y:gamepadY_=pressed;break;
        case ZenithGamepad::LB:gamepadLB_=pressed;break;case ZenithGamepad::RB:gamepadRB_=pressed;break;case ZenithGamepad::LS:gamepadLS_=pressed;break;case ZenithGamepad::RS:gamepadRS_=pressed;break;
        case ZenithGamepad::View:gamepadView_=pressed;break;case ZenithGamepad::Menu:gamepadMenu_=pressed;break;
        case ZenithGamepad::DpadUp:gamepadDpadUp_=pressed;break;case ZenithGamepad::DpadDown:gamepadDpadDown_=pressed;break;case ZenithGamepad::DpadLeft:gamepadDpadLeft_=pressed;break;case ZenithGamepad::DpadRight:gamepadDpadRight_=pressed;break;
        default:return;
    }
    rebuildMovement();
}
void InputState::setGamepadAxis(std::uint32_t axis,float value){
    value=std::max(-1.0f,std::min(1.0f,value));
    switch(axis){
        case ZenithGamepad::LeftX:gamepadLeftStick_.x=value;break;case ZenithGamepad::LeftY:gamepadLeftStick_.y=value;break;
        case ZenithGamepad::RightX:gamepadRightStick_.x=value;break;case ZenithGamepad::RightY:gamepadRightStick_.y=value;break;
        case ZenithGamepad::LeftTrigger:gamepadLeftTrigger_=(value+1.0f)*0.5f;break;case ZenithGamepad::RightTrigger:gamepadRightTrigger_=(value+1.0f)*0.5f;break;
        default:return;
    }
    rebuildMovement();
}
