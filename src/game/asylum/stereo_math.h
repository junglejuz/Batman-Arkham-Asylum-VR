#pragma once
#include "core/vr/openxr_runtime.h"
#include <cmath>
namespace bvr::aa::math {
// Game units per real metre. WORLDSCALEDIR (JJ 2026-10-25: "when you raise the slider it reduces the size of the world",
// as City / Origins / Knight): a higher scale = more units per metre = a smaller-looking world.
inline float world_units(float scale){return 90.f*(std::isfinite(scale)?std::fmax(.5f,std::fmin(2.f,scale)):1.f);}
struct alignas(16) M {float v[4][4];};
inline M mul(const M&a,const M&b){M r{};for(int i=0;i<4;i++)for(int j=0;j<4;j++)for(int k=0;k<4;k++)r.v[i][j]+=a.v[i][k]*b.v[k][j];return r;}
inline M pose_view(const vr::HeadPose&p,float units=90){
 const float x=p.qx,y=p.qy,z=p.qz,w=p.qw;
 const float r[3][3]={{1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)},{2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)},{2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)}};
 const float flip[3]={1,1,-1};M d{};
 for(int i=0;i<3;i++)for(int j=0;j<3;j++)d.v[i][j]=r[i][j]*flip[i]*flip[j];
 const float t[3]={p.px*units,p.py*units,-p.pz*units};
 for(int j=0;j<3;j++)for(int i=0;i<3;i++)d.v[3][j]-=t[i]*d.v[i][j];d.v[3][3]=1;return d;
}
inline M projection(M p,const float angles[4]){
 const float l=std::tan(angles[0]),r=std::tan(angles[1]),u=std::tan(angles[2]),d=std::tan(angles[3]);
 p.v[0][0]=2/(r-l);p.v[2][0]=-(r+l)/(r-l);p.v[1][1]=2/(u-d);p.v[2][1]=-(u+d)/(u-d);return p;
}
// Legacy screen-space passes may assume m20/m21 are zero. Render a centred
// enclosing frustum, then map its pixels to the actual XR lens before submit.
inline M centred_projection(M p,const float angles[4]){
 const float h=std::fmax(-std::tan(angles[0]),std::tan(angles[1]));
 const float v=std::fmax(std::tan(angles[2]),-std::tan(angles[3]));
 p.v[0][0]=1/h;p.v[1][1]=1/v;p.v[2][0]=p.v[2][1]=0;return p;
}
inline void lens_crop(const float angles[4],float uv[4]){
 const float l=std::tan(angles[0]),r=std::tan(angles[1]),u=std::tan(angles[2]),d=std::tan(angles[3]);
 const float h=std::fmax(-l,r),v=std::fmax(u,-d);
 uv[0]=(r-l)/(2*h);uv[1]=(u-d)/(2*v);uv[2]=.5f+l/(2*h);uv[3]=.5f-u/(2*v);
}
inline bool inverse(const M& input,M& output){
 double a[4][8]{};for(int i=0;i<4;++i)for(int j=0;j<8;++j)a[i][j]=j<4?input.v[i][j]:double(j-4==i);
 for(int c=0;c<4;++c){int p=c;for(int i=c+1;i<4;++i)if(std::fabs(a[i][c])>std::fabs(a[p][c]))p=i;
  if(std::fabs(a[p][c])<1e-20)return false;
  if(p!=c)for(int j=0;j<8;++j){const double x=a[c][j];a[c][j]=a[p][j];a[p][j]=x;}
  const double s=a[c][c];for(int j=0;j<8;++j)a[c][j]/=s;
  for(int i=0;i<4;++i)if(i!=c){const double s2=a[i][c];for(int j=0;j<8;++j)a[i][j]-=s2*a[c][j];}
 }
 for(int i=0;i<4;++i)for(int j=0;j<4;++j){output.v[i][j]=float(a[i][j+4]);if(!std::isfinite(output.v[i][j]))return false;}return true;
}
// Cancel the shared world origin in double precision BEFORE rounding to the
// GPU matrix. Inverting a float world VP loses distance and motion precision.
inline bool relative_projection(const M& view,const M& projection,const M& previousView,const M& previousProjection,M& result){
 auto inv=[](const M& m,double out[4][4]){double a[4][8]{};
  for(int i=0;i<4;++i)for(int j=0;j<8;++j)a[i][j]=j<4?m.v[i][j]:double(i==j-4);
  for(int c=0;c<4;++c){int p=c;for(int i=c+1;i<4;++i)if(std::fabs(a[i][c])>std::fabs(a[p][c]))p=i;
   if(std::fabs(a[p][c])<1e-20)return false;
   if(p!=c)for(int j=0;j<8;++j){double t=a[p][j];a[p][j]=a[c][j];a[c][j]=t;}
   double s=a[c][c];for(int j=0;j<8;++j)a[c][j]/=s;
   for(int i=0;i<4;++i)if(i!=c){double f=a[i][c];for(int j=0;j<8;++j)a[i][j]-=f*a[c][j];}}
  for(int i=0;i<4;++i)for(int j=0;j<4;++j)out[i][j]=a[i][j+4];return true;};
 double iv[4][4]{},ip[4][4]{},relative[4][4]{},projected[4][4]{};
 if(!inv(view,iv)||!inv(projection,ip))return false;
 for(int i=0;i<4;++i)for(int j=0;j<4;++j)for(int k=0;k<4;++k)relative[i][j]+=iv[i][k]*previousView.v[k][j];
 for(int i=0;i<4;++i)for(int j=0;j<4;++j)for(int k=0;k<4;++k)projected[i][j]+=relative[i][k]*previousProjection.v[k][j];
 for(int i=0;i<4;++i)for(int j=0;j<4;++j){double d{};for(int k=0;k<4;++k)d+=ip[i][k]*projected[k][j];result.v[i][j]=float(d);if(!std::isfinite(result.v[i][j]))return false;}return true;
}
// Original canvas clip coordinates -> one upright plane -> the actual eye.
// The engine retains its original shaders, clip masks, textures and layout.
inline M canvas_projection(const M& original,const vr::HeadPose& eye,const float angles[4],float aspect,float halfWidthMetres){
 constexpr float units=90, distance=4;
 M plane{};plane.v[0][0]=halfWidthMetres*units;plane.v[1][1]=halfWidthMetres/aspect*units;
 plane.v[3][2]=distance*units;plane.v[3][3]=1;
 M p{};p.v[2][2]=1;p.v[2][3]=1;p.v[3][2]=-.1f;
 return mul(mul(mul(original,plane),pose_view(eye,units)),projection(p,angles));
}
}
