#include "input_state.h"
#include <cmath>

void InputState::reset(){controller_={};w_=a_=s_=d_=up_=down_=left_=right_=space_=false;}
void InputState::rebuildMovement(){
    float x=(d_?1.0f:0.0f)-(a_?1.0f:0.0f);
    float y=(s_?1.0f:0.0f)-(w_?1.0f:0.0f);
    const float len=std::sqrt(x*x+y*y);
    if(len>1.0f){x/=len;y/=len;}
    controller_.leftStick={x,y};
    controller_.rightStick={
        (right_?1.0f:0.0f)-(left_?1.0f:0.0f),
        (down_?1.0f:0.0f)-(up_?1.0f:0.0f)
    };
    controller_.jump=space_;
}
void InputState::setKey(std::uint32_t key,bool pressed){
    switch(key){
        case ZenithKey::W:w_=pressed;break; case ZenithKey::A:a_=pressed;break;
        case ZenithKey::S:s_=pressed;break; case ZenithKey::D:d_=pressed;break;
        case ZenithKey::Up:up_=pressed;break; case ZenithKey::Down:down_=pressed;break;
        case ZenithKey::Left:left_=pressed;break; case ZenithKey::Right:right_=pressed;break;
        case ZenithKey::Space:space_=pressed;break; default:return;
    }
    rebuildMovement();
}
