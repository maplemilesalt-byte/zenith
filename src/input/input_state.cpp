#include "input_state.h"
#include <cmath>

void InputState::reset(){controller_={};w_=a_=s_=d_=up_=down_=left_=right_=space_=ctrl_=e_=q_=z_=c_=r_=f_=v_=g_=tab_=enter_=di_=dk_=dj_=dl_=false;}
void InputState::rebuildMovement(){
    float x=(d_?1.0f:0.0f)-(a_?1.0f:0.0f);
    float y=(s_?1.0f:0.0f)-(w_?1.0f:0.0f);
    const float len=std::sqrt(x*x+y*y);
    if(len>1.0f){x/=len;y/=len;}
    controller_.leftStick={x,y};
    controller_.rightStick={(right_?1.0f:0.0f)-(left_?1.0f:0.0f),(down_?1.0f:0.0f)-(up_?1.0f:0.0f)};
    controller_.jump=space_;
    controller_.buttonA=space_;
    controller_.buttonB=ctrl_;
    controller_.buttonX=e_;
    controller_.buttonY=q_;
    controller_.leftBumper=z_;
    controller_.rightBumper=c_;
    controller_.leftTrigger=r_?1.0f:0.0f;
    controller_.rightTrigger=f_?1.0f:0.0f;
    controller_.leftStickClick=v_;
    controller_.rightStickClick=g_;
    controller_.view=tab_;
    controller_.menu=enter_;
    controller_.dpadUp=di_;
    controller_.dpadDown=dk_;
    controller_.dpadLeft=dj_;
    controller_.dpadRight=dl_;
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
