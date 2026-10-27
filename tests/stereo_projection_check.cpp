#include "game/asylum/stereo_math.h"
#include "game/asylum/transition_plane.h"
#include "core/vr/flat_preview.h"
#include "core/util/xr_math.h"
#include <cstdio>
#include <cmath>
#include <limits>
using bvr::aa::math::M;
namespace math=bvr::aa::math;
void transform(const M&m,const float in[4],float out[4]){for(int j=0;j<4;++j){out[j]=0;for(int i=0;i<4;++i)out[j]+=in[i]*m.v[i][j];}}
bool same_ray(const float a[4],const float b[4]){return a[3]>0&&b[3]>0&&std::fabs(a[0]/a[3]-b[0]/b[3])<.00002f&&std::fabs(a[1]/a[3]-b[1]/b[3])<.00002f;}
int main(){
 // World-scale regression: a fixed 450-unit target has the stereo disparity
 // of a target at (5 / scale) real metres (higher scale = smaller world). Head translation uses that same
 // conversion, with identical rotations and projection/FOV at every scale.
 for(float scale:{.5f,1.f,1.5f,2.f}) {
  const float units=math::world_units(scale);float projected[2]{};
  for(int eye=0;eye<2;++eye){
   bvr::vr::HeadPose pose{};pose.qw=1;pose.px=.2f+(eye?.032f:-.032f);pose.py=.1f;pose.pz=-.3f;
   auto v=math::pose_view(pose,units);M inverse{};if(!math::inverse(v,inverse))return 10;
   if(std::fabs(inverse.v[3][0]-pose.px*units)>.0001f ||
      std::fabs(inverse.v[3][1]-pose.py*units)>.0001f ||
      std::fabs(inverse.v[3][2]+pose.pz*units)>.0001f)return 11;
   // Target 450 units ahead of translated head centre (no angular FOV change).
   const float point[4]={.2f*units,.1f*units,.3f*units+450,1};float q[4];transform(v,point,q);
   projected[eye]=q[0]/q[2];
   if(v.v[0][0]!=1 || v.v[1][1]!=1 || v.v[2][2]!=1)return 12;
  }
  if(std::fabs((projected[0]-projected[1])-.064f*scale/5)>.000001f)return 13;
 }
 if(math::world_units(1)!=90 || math::world_units(0)!=45 || math::world_units(9)!=180 ||
    math::world_units(std::numeric_limits<float>::quiet_NaN())!=90)return 14;
 std::puts("PASS: world scale preserves FOV and rotations; binocular disparity and head translation match physical scale.");
 constexpr float rad=3.141592654f/180;
 for(int pose=0;pose<3;++pose){
  float q[4]={0,0,0,1};if(pose)bvr::xrmath::xr_local_trim_quat(16*rad,31*rad,-22*rad,q);
  for(int e=0;e<2;++e){
   XrView view{XR_TYPE_VIEW};view.pose.orientation={q[0],q[1],q[2],q[3]};
   float baseline[3]={e?.032f:-.032f,0,0},rotated[3];bvr::xrmath::quat_rotate(q[0],q[1],q[2],q[3],baseline,rotated);
   view.pose.position={rotated[0]+.3f,rotated[1]+.1f,rotated[2]+.2f};
   view.fov={-(e?35.5f:49.5f)*rad,(e?49.5f:35.5f)*rad,39.4f*rad,-50.5f*rad};
   float angles[4]={view.fov.angleLeft,view.fov.angleRight,view.fov.angleUp,view.fov.angleDown};
   bvr::vr::HeadPose eye{view.pose.position.x,view.pose.position.y,view.pose.position.z,q[0],q[1],q[2],q[3]};
   M p{};p.v[2][2]=.999f;p.v[2][3]=1;p.v[3][2]=-10;
   const auto lens=math::projection(p,angles);
   const auto centred=math::centred_projection(p,angles);float crop[4];math::lens_crop(angles,crop);
   if(centred.v[2][0]!=0||centred.v[2][1]!=0||centred.v[2][2]!=p.v[2][2]||centred.v[3][2]!=p.v[3][2])return 5;
   for(float x:{-.95f,0.f,.95f})for(float y:{-.95f,0.f,.95f}){
    const float ray[4]={(x-lens.v[2][0])/lens.v[0][0],(y-lens.v[2][1])/lens.v[1][1],1,1};float clip[4];transform(centred,ray,clip);
    const float sx=(clip[0]+1)*.5f,sy=(1-clip[1])*.5f;
    if(sx<0||sx>1||sy<0||sy>1||std::fabs((sx-crop[2])/crop[0]*2-1-x)>.00002f||std::fabs(1-(sy-crop[3])/crop[1]*2-y)>.00002f)return 6;
   }
   if(lens.v[2][2]!=p.v[2][2]||lens.v[3][2]!=p.v[3][2])return 1;
   const auto world=math::mul(math::pose_view(eye),lens);
   for(float depth:{2.f,5.f,100.f}){
    const float point[3]={.1f,.2f,-depth},input[4]={point[0]*90,point[1]*90,-point[2]*90,1};float rendered[4],expected[4];
    transform(world,input,rendered);bvr::vr::preview::clip(view,point,expected);
    if(!same_ray(rendered,expected)){std::puts("World camera does not match submitted XR ray");return 2;}
   }
   // Stock pixel-space canvas -> a shared upright 4m plane. Test all corners
   // and centre with nonzero pitch/yaw/roll: same world points, different eyes.
   M canvas{};canvas.v[0][0]=2.f/1238;canvas.v[1][1]=-2.f/1328;canvas.v[3][0]=-1;canvas.v[3][1]=1;canvas.v[3][3]=1;
   constexpr float halfWidth=2.3f,aspect=1238.f/1328;
   const auto ui=math::canvas_projection(canvas,eye,angles,aspect,halfWidth);
   for(float x:{0.f,619.f,1238.f})for(float y:{0.f,664.f,1328.f}){
    const float input[4]={x,y,0,1},point[3]={(x/619-1)*halfWidth,(1-y/664)*halfWidth/aspect,-4};float rendered[4],expected[4];
    transform(ui,input,rendered);bvr::vr::preview::clip(view,point,expected);
    if(!same_ray(rendered,expected)){std::puts("Canvas is not on the same world plane through each eye");return 3;}
   }
   // Independently prove the optical corners; a sign/symmetric error fails.
   for(float x:{std::tan(angles[0]),std::tan(angles[1])}){
    const float input[4]={x*200,0,200,1};float result[4];transform(lens,input,result);
    const float expected=x==std::tan(angles[0])?-1.f:1.f;
    if(std::fabs(result[0]/result[3]-expected)>.00002f)return 4;
   }
  }
 }
 // The transition plane covers both eye frusta at0.8m and keeps one physical
 // plane under yaw/pitch/roll, including opposite quaternion signs and largeIPD.
 for(float ipd:{.064f,.13f})for(float roll:{0.f,.4f}){
  XrView views[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};for(int e=0;e<2;++e){auto&q=views[e].pose.orientation;q={0,0,std::sin(roll*.5f),std::cos(roll*.5f)};const float local[3]={e?ipd*.5f:-ipd*.5f,0,0};float moved[3];bvr::xrmath::quat_rotate(q.x,q.y,q.z,q.w,local,moved);views[e].pose.position={moved[0]+3,moved[1]+1,moved[2]-.5f};views[e].fov={e?-.65f:-.9f,e?.9f:.65f,.8f,-1.0f};}
  if(roll){auto&q=views[1].pose.orientation;q.x=-q.x;q.y=-q.y;q.z=-q.z;q.w=-q.w;}
  float clips[2][4][4]{};if(!bvr::aa::transition_plane(views,192.f/208.f,clips))return 5;
  for(int e=0;e<2;++e)for(int c=0;c<4;++c){const auto&v=clips[e][c];if(std::fabs(v[3]-.8f)>.0001f||(c&1?v[0]/v[3]<1:v[0]/v[3]>-1)||(c&2?v[1]/v[3]>-1:v[1]/v[3]<1))return 6;}
 }
 std::puts("PASS: near bat plane covers BOTH asymmetric eye frusta at0.8m with large IPD/head roll and opposite quaternion signs.");
 std::puts("PASS: actual scene/canvas transforms match XR rays through asymmetric lenses, mixed head rotations, near/far geometry and a shared upright UI plane.");
}
