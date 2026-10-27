#include "game/asylum/head_aim.h"
#include "core/vr/flat_preview.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <cstring>

namespace h=bvr::aa::head_aim;
using bvr::vr::HeadPose;
int checks=0;
#define CHECK(x) do { ++checks; if(!(x)){std::printf("FAIL line %d: %s\n",__LINE__,#x);return 1;} } while(0)
bool near(float a,float b,float eps=.00015f){return std::fabs(a-b)<eps;}
HeadPose pose(float pitch,float yaw,float roll) {
    HeadPose p{};float q[4];
    bvr::xrmath::xr_local_trim_quat(pitch*h::pi/180,yaw*h::pi/180,roll*h::pi/180,q);
    p.qx=q[0];p.qy=q[1];p.qz=q[2];p.qw=q[3];return p;
}
// Independent quaternion-vector rotation: v + 2*(w*(q x v) + q x (q x v)).
void rotate(const HeadPose&q,const float v[3],float out[3]) {
    const float a[3]={q.qy*v[2]-q.qz*v[1],q.qz*v[0]-q.qx*v[2],q.qx*v[1]-q.qy*v[0]};
    const float b[3]={q.qy*a[2]-q.qz*a[1],q.qz*a[0]-q.qx*a[2],q.qx*a[1]-q.qy*a[0]};
    for(int j=0;j<3;++j)out[j]=v[j]+2*(q.qw*a[j]+b[j]);
}
bool aim_matches(const h::Solution&s,const HeadPose&p) {
    const float f[3]={0,0,-1};float local[3];rotate(p,f,local);
    const float y=h::radians(s.body.yaw),pitch=h::radians(s.body.pitch);
    const float cy=std::cos(y),sy=std::sin(y),cp=std::cos(pitch),sp=std::sin(pitch);
    const float expected[3]={-sy*local[0]-sp*cy*local[1]-cp*cy*local[2],
        cy*local[0]-sp*sy*local[1]-cp*sy*local[2],cp*local[1]-sp*local[2]};
    const float ty=h::radians(s.target.yaw),tp=h::radians(s.target.pitch);
    return near(std::cos(tp)*std::cos(ty),expected[0]) &&
        near(std::cos(tp)*std::sin(ty),expected[1]) && near(std::sin(tp),expected[2]);
}
int main(){
    // Retail HEAD-AIM report: native paired scene + valid first-person pawn,
    // but IsMenuLevel unknown (particles unknownStock>0), writes=0 forever.
    // That is missing menu evidence, not a positive menu verdict. Exercise the
    // SAME decision policy used by begin_frame/apply, then the actual aim math.
    using Mode=bvr::aa::menu_state::Mode;
    h::Context live{true,true,true,false,Mode::unknown};
    CHECK(live.evaluate(false,false)==h::Gate::needPawn);
    CHECK(!h::permits(live.evaluate(false,true)));
    CHECK(live.evaluate(true,false)==h::Gate::needCamera);
    CHECK(live.evaluate(true,true)==h::Gate::activePawnFallback);
    h::Tracker regression;h::Solution driven;
    CHECK(h::permits(live.evaluate(true,true))&&regression.solve({0,0,0},pose(20,35,0),driven));
    CHECK(driven.target.pitch!=0&&driven.target.yaw!=0&&aim_matches(driven,pose(20,35,0)));
    auto denied=live;denied.mode=Mode::menu;CHECK(denied.evaluate(true,true)==h::Gate::menu);
    denied=live;denied.firstPerson=false;CHECK(denied.evaluate(true,true)==h::Gate::off);
    denied=live;denied.requested=false;CHECK(denied.evaluate(true,true)==h::Gate::optionOff);
    denied=live;denied.native=false;CHECK(denied.evaluate(true,true)==h::Gate::noNative);
    denied=live;denied.panel=true;CHECK(denied.evaluate(true,true)==h::Gate::panel);
    live.mode=Mode::gameplay;CHECK(live.evaluate(true,true)==h::Gate::active);
    CHECK(!h::permits(live.evaluate(false,true))&&!h::permits(live.evaluate(true,false)));
    h::Tracker t;h::Solution s;h::Rotation current{0,0,1234};
    CHECK(t.solve(current,pose(0,0,0),s));CHECK(s.target.pitch==0&&s.target.yaw==0&&s.target.roll==1234);
    // The trim helper takes positive yaw to the right (negative XR Y angle).
    CHECK(t.solve(current,pose(0,30,0),s));CHECK(near(h::radians(s.target.yaw),h::pi/6));
    CHECK(t.solve(current,pose(20,0,0),s));CHECK(near(h::radians(s.target.pitch),h::pi/9));
    for(float pitch:{-80.f,-30.f,0.f,40.f,80.f})for(float yaw:{-179.f,-95.f,0.f,80.f,179.f})
    for(float roll:{-70.f,0.f,65.f})for(int bodyYaw:{-32500,0,26000}) {
        h::Tracker tracker;const h::Rotation initial{2200,bodyYaw,456};const auto head=pose(pitch,yaw,roll);
        CHECK(tracker.solve(initial,head,s));CHECK(aim_matches(s,head));CHECK(s.target.roll==456);
        tracker.commit(s);current=s.target;
        for(int frame=0;frame<32;++frame){
            CHECK(tracker.solve(current,head,s));CHECK(s.body.pitch==initial.pitch&&s.body.yaw==initial.yaw);
            CHECK(aim_matches(s,head));tracker.commit(s);current=s.target;
        }
        // Real stick movement while the HMD remains still must stay additive.
        current.yaw=h::wrap(int64_t(current.yaw)+900);current.pitch=h::wrap(int64_t(current.pitch)-200);
        CHECK(tracker.solve(current,head,s));CHECK(s.body.yaw==h::wrap(bodyYaw+900));CHECK(s.body.pitch==2000);
        CHECK(aim_matches(s,head));tracker.commit(s);
        const auto restored=tracker.remove(s.target);CHECK(restored.yaw==s.body.yaw&&restored.pitch==s.body.pitch&&restored.roll==456);
        // Recenter to neutral drops the old contribution without accumulating it.
        CHECK(tracker.solve(s.target,pose(0,0,0),s));CHECK(s.target.yaw==restored.yaw&&s.target.pitch==restored.pitch);
    }
    CHECK(h::wrap(32768)==-32768&&h::wrap(-32769)==32767&&h::wrap(65536)==0);
    // Moving head + continuous stick turns across wrap boundaries. Engine
    // deltas are applied to its last target, just as in the runtime adapter.
    h::Tracker moving;h::Rotation engine{0,32000,99};int expectedYaw=32000;
    for(int frame=0;frame<4096;++frame){
        engine.yaw=h::wrap(int64_t(engine.yaw)+73);expectedYaw=h::wrap(expectedYaw+73);
        const auto head=pose(45.f*std::sin(frame*.02f),170.f*std::sin(frame*.013f),25.f*std::cos(frame*.017f));
        CHECK(moving.solve(engine,head,s));CHECK(s.body.yaw==expectedYaw&&s.body.pitch==0);
        CHECK(aim_matches(s,head));moving.commit(s);engine=s.target;
    }
    engine.yaw=h::wrap(int64_t(engine.yaw)+300);
    CHECK(moving.remove(engine).yaw==h::wrap(expectedYaw+300));
    const auto before=t.offset;auto bad=pose(0,0,0);bad.qx=std::numeric_limits<float>::quiet_NaN();
    CHECK(!t.solve(current,bad,s));CHECK(!std::memcmp(&before,&t.offset,sizeof(before)));
    bad=pose(0,0,0);bad.qw=0;CHECK(!t.solve(current,bad,s));
    auto shifted=pose(15,40,10);CHECK(t.solve(current,shifted,s));const auto target=s.target;
    shifted.px=2;shifted.py=1;shifted.pz=-3;CHECK(t.solve(current,shifted,s));CHECK(!std::memcmp(&target,&s.target,sizeof(target)));

    for(int16_t value:{int16_t(-32768),int16_t(0),int16_t(32767)}) {
        CHECK(h::filter_pitch(value,false,true)==value);
        CHECK(h::filter_pitch(value,true,false)==value);
        CHECK(h::filter_pitch(value,true,true)==0);
    }
    for(float pitch:{-45.f,45.f}) {
        h::Tracker walking; h::Solution w;
        CHECK(walking.solve({0,0,0},pose(pitch,0,0),w,false)); walking.commit(w);
        for(int frame=0;frame<300;++frame) {
            auto recentered=w.target; recentered.pitch=0;
            CHECK(walking.solve(recentered,pose(pitch,0,0),w,false));
            CHECK(w.body.pitch==0); CHECK(aim_matches(w,pose(pitch,0,0)));
            walking.commit(w);
        }
        auto stick=w.target; stick.pitch=h::wrap(stick.pitch+100);
        CHECK(walking.solve(stick,pose(pitch,0,0),w,true)); CHECK(w.body.pitch==100);
    }
    // AIMLEVEL (JJ: stuck looking at the ground after the VR panel closed): a fresh take-over while the game camera looks
    // 30 deg down must aim level with a level head, and keep the head's own pitch otherwise; a release restores level.
    for(int takeovers=0;takeovers<3;++takeovers) {
        h::Tracker fresh; h::Solution f; const h::Rotation down{-5461,1200,0};
        CHECK(fresh.solve(down,pose(0,0,0),f,false)); CHECK(f.body.pitch==0); CHECK(std::abs(f.target.pitch)<=1);
        CHECK(f.target.yaw==1200); fresh.commit(f);
        CHECK(fresh.solve(f.target,pose(25,0,0),f,false)); CHECK(f.body.pitch==0); CHECK(near(h::radians(f.target.pitch),25*h::pi/180,.0005f));
        fresh.commit(f); CHECK(fresh.remove(f.target,false).pitch==0);
    }
    // HUD stays centred under arbitrary rigid head motion but keeps real stereo
    // disparity. Opposite quaternion signs describe the same orientation.
    float baseline[2][4][2]{};
    for(int sample=0;sample<5;++sample){
        const auto head=sample?pose(12.f*sample,37.f*sample,-9.f*sample):pose(0,0,0);
        HeadPose eyes[2];XrView views[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};
        for(int e=0;e<2;++e){
            eyes[e]=head;const float eye[3]={e?.034f:-.034f,0,0};float offset[3];rotate(head,eye,offset);
            eyes[e].px=offset[0]+sample*.3f;eyes[e].py=offset[1]+sample*.1f;eyes[e].pz=offset[2]-sample*.4f;
            if(e){eyes[e].qx*=-1;eyes[e].qy*=-1;eyes[e].qz*=-1;eyes[e].qw*=-1;}
            auto&v=views[e];v.pose.orientation={eyes[e].qx,eyes[e].qy,eyes[e].qz,eyes[e].qw};
            v.pose.position={eyes[e].px,eyes[e].py,eyes[e].pz};
            v.fov={-(e?35.f:50.f)*h::pi/180,(e?50.f:35.f)*h::pi/180,.7f,-.9f};
        }
        float plane[4][3];CHECK(h::hud_plane(eyes,4,2,1,plane));
        for(int e=0;e<2;++e)for(int corner=0;corner<4;++corner){
            float clip[4];bvr::vr::preview::clip(views[e],plane[corner],clip);CHECK(clip[3]>0);
            for(int j=0;j<2;++j){const float ndc=clip[j]/clip[3];if(!sample)baseline[e][corner][j]=ndc;else CHECK(near(ndc,baseline[e][corner][j]));}
        }
        // Independent centre ray: local +/- half-IPD at distance 4m.
        float centre[3]{};for(auto&c:plane)for(int j=0;j<3;++j)centre[j]+=c[j]*.25f;
        for(int e=0;e<2;++e){float clip[4];bvr::vr::preview::clip(views[e],centre,clip);
            const float l=std::tan(views[e].fov.angleLeft),r=std::tan(views[e].fov.angleRight);
            CHECK(near(clip[0]/clip[3],2*(e?-.034f:.034f)/(4*(r-l))-(r+l)/(r-l)));
        }
        CHECK(!h::hud_plane(eyes,0,2,1,plane));eyes[1].qw=7;CHECK(!h::hud_plane(eyes,4,2,1,plane));
    }
    std::printf("PASS: %d checks: retail unknown-menu gating regression, independent aim rays, additive stick turns, no accumulated head rotation, recenter/restore, invalid poses, shared HUD stereo and rigid head motion\n",checks);
}
