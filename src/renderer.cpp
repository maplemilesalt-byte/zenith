#include "renderer.h"
#include "memory/guest_memory.h"
#include <algorithm>
#include <cmath>
Renderer::Renderer(int w,int h):width_(w),height_(h),pixels_(static_cast<std::size_t>(w)*h,0){}
void Renderer::clear(std::uint32_t c){std::fill(pixels_.begin(),pixels_.end(),c);}
void Renderer::putPixel(int x,int y,std::uint32_t c){if(x<0||y<0||x>=width_||y>=height_)return;pixels_[static_cast<std::size_t>(y)*width_+x]=c;}
void Renderer::drawLine(int x0,int y0,int x1,int y1,std::uint32_t c){int dx=std::abs(x1-x0),sx=x0<x1?1:-1,dy=-std::abs(y1-y0),sy=y0<y1?1:-1,e=dx+dy;for(;;){putPixel(x0,y0,c);if(x0==x1&&y0==y1)break;int e2=2*e;if(e2>=dy){e+=dy;x0+=sx;}if(e2<=dx){e+=dx;y0+=sy;}}}
void Renderer::drawCube(float angle){constexpr float scale=220.f,dist=4.f;float c=std::cos(angle),s=std::sin(angle);const Vertex v[]={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};int p[8][2]{};for(int i=0;i<8;i++){float rx=v[i].x*c-v[i].z*s,rz=v[i].x*s+v[i].z*c,k=scale/(rz+dist);p[i][0]=(int)(rx*k+width_*.5f);p[i][1]=(int)(v[i].y*k+height_*.5f);}const int e[][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};for(auto&a:e)drawLine(p[a[0]][0],p[a[0]][1],p[a[1]][0],p[a[1]][1],0x00FFFFFF);}
bool Renderer::drawGuestFramebuffer(const GuestMemory& m,std::uint64_t a,int gw,int gh){if(gw<=0||gh<=0)return false;const std::uint64_t bytes=(std::uint64_t)gw*gh*4;for(std::uint64_t i=0;i<bytes;i+=4){std::uint8_t b0,b1,b2,b3;if(!m.read8(a+i,b0)||!m.read8(a+i+1,b1)||!m.read8(a+i+2,b2)||!m.read8(a+i+3,b3))return false;int gx=(int)((i/4)%gw),gy=(int)((i/4)/gw);int x=gx*width_/gw,y=gy*height_/gh,x2=(gx+1)*width_/gw,y2=(gy+1)*height_/gh;std::uint32_t c=(std::uint32_t)b0|((std::uint32_t)b1<<8)|((std::uint32_t)b2<<16)|((std::uint32_t)b3<<24);for(int yy=y;yy<y2;yy++)for(int xx=x;xx<x2;xx++)putPixel(xx,yy,c);}return true;}
const std::uint32_t* Renderer::pixels()const{return pixels_.data();} int Renderer::width()const{return width_;} int Renderer::height()const{return height_;}
