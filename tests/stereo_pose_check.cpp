#include "core/vr/stereo_pose_mailbox.h"
#include <atomic>
#include <cstdio>
#include <thread>
using namespace bvr::vr;
#define CHECK(x) do { if(!(x)){std::printf("FAIL line %d: %s\n",__LINE__,#x);return 1;} } while(0)
StereoPose sample(uint32_t gen) {
    StereoPose s{};s.generation=gen;
    s.head.px=float(gen);s.head.qw=1;
    for(int e=0;e<2;++e){s.eyes[e]=s.head;s.eyes[e].px+=e?.03f:-.03f;
        for(int j=0;j<4;++j)s.angles[e][j]=float(gen+e*4+j);}
    return s;
}
bool same(const StereoPose& a,const StereoPose& b) {
    if(a.generation!=b.generation || a.head.px!=b.head.px || a.head.qw!=b.head.qw)return false;
    for(int e=0;e<2;++e){
        if(a.eyes[e].px!=b.eyes[e].px || a.eyes[e].qw!=b.eyes[e].qw)return false;
        for(int j=0;j<4;++j)if(a.angles[e][j]!=b.angles[e][j])return false;
    }
    return true;
}
int main() {
    StereoPoseMailbox box;StereoPose out{};
    CHECK(!box.read(out));
    box.publish(sample(1),true);
    // Force the exact publication window that used to expose a new head id
    // before that id's eyes existed. No timing sleeps or probabilistic race.
    std::atomic<bool> headLocated{},finishEyes{};
    std::thread writer([&]{
        StereoPose next{};next.head=sample(2).head;next.generation=2;
        headLocated=true;
        while(!finishEyes.load())std::this_thread::yield();
        next=sample(2);box.publish(next,true);
    });
    while(!headLocated.load())std::this_thread::yield();
    const bool completePrevious=box.read(out)&&same(out,sample(1));
    finishEyes=true;writer.join();CHECK(completePrevious);
    CHECK(box.read(out)&&same(out,sample(2)));
    box.publish(sample(3),false);CHECK(!box.read(out)); // actual tracking loss
    box.publish(sample(4),true);CHECK(box.read(out)&&same(out,sample(4)));
    box.invalidate();CHECK(!box.read(out)); // session teardown
    box.publish(sample(5),true);
    std::atomic<bool> start{},done{},bad{};std::atomic<unsigned> reads{};
    std::thread reader([&]{start=true;while(!done.load()){
        StereoPose s;if(box.read(s)){if(!same(s,sample(s.generation)))bad=true;++reads;}
    }});
    while(!start.load() || !reads.load())std::this_thread::yield();
    for(uint32_t gen=6;gen<100006;++gen)box.publish(sample(gen),true);
    done=true;reader.join();CHECK(!bad.load());CHECK(reads.load()>0);
    CHECK(box.read(out)&&same(out,sample(100005)));
    std::printf("PASS: coherent publication during split locate, tracking loss, teardown, recovery; %u concurrent reads over 100000 publications\n",reads.load());
}
