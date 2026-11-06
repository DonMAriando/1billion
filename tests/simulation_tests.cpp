#include "simulation.hpp"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <stdexcept>

namespace {std::atomic<bool> track{false};std::atomic<std::size_t> allocations{0};int failures=0;}
void* operator new(std::size_t size) {
    if(track.load(std::memory_order_relaxed)) allocations.fetch_add(1,std::memory_order_relaxed);
    if(void* p=std::malloc(size==0 ? 1 : size)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) {return ::operator new(size);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}

#define CHECK(expr) do {if(!(expr)) {std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#expr);++failures;}} while(false)

int main() {
    using namespace siege;
    {
        EnemyPool pool(128);
        for(int i=0;i<128;++i) CHECK(pool.add({static_cast<float>(i),-35},static_cast<float>(i+1),EnemyKind::Brute,7));
        CHECK(!pool.add({},1,EnemyKind::Drone,0));CHECK(pool.count==128);
        pool.slowUntil[127]=77;pool.inside[127]=1;pool.erase(4);
        CHECK(pool.count==127);CHECK(pool.x[4]==127);CHECK(pool.health[4]==128);
        CHECK(pool.slowUntil[4]==77);CHECK(pool.inside[4]==1);CHECK(pool.kind[4]==EnemyKind::Brute);
        pool.erase(999);CHECK(pool.count==127);pool.clear();CHECK(pool.count==0);CHECK(pool.capacity()==128);
        bool threw=false;try {EnemyPool invalid(0);}catch(const std::invalid_argument&) {threw=true;}CHECK(threw);
    }
    {
        auto sim=std::make_unique<Simulation>();
        CHECK(sim->phase()==Phase::Planning);CHECK(sim->wave()==0);CHECK(sim->powerUsed()==8);
        const int before=sim->matter();
        CHECK(sim->build(1,TowerKind::Repeater)==Result::Occupied);CHECK(sim->matter()==before);
        CHECK(sim->build(-1,TowerKind::Mortar)==Result::Invalid);CHECK(sim->build(24,TowerKind::Mortar)==Result::Invalid);
        CHECK(sim->build(0,TowerKind::Cryo)==Result::Locked);CHECK(sim->research()==Result::Locked);
        CHECK(sim->build(0,TowerKind::Mortar)==Result::Ok);CHECK(sim->matter()==before-260);CHECK(sim->powerUsed()==10);
        CHECK(sim->build(2,TowerKind::Mortar)==Result::Ok);CHECK(sim->powerUsed()==12);
        const int after=sim->matter();CHECK(sim->build(3,TowerKind::Repeater)==Result::NoPower);CHECK(sim->matter()==after);
        CHECK(sim->generator()==Result::Ok);CHECK(sim->powerAvailable()==16);CHECK(sim->matter()==0);
        CHECK(sim->upgrade(0)==Result::NoMatter);CHECK(sim->repair(0)==Result::MaxLevel);
        sim->moveSquads(7,{900,-900});for(int i=0;i<150;++i) sim->tick();
        for(const auto& squad:sim->squads()) {CHECK(squad.destination.x<=8);CHECK(squad.destination.y>=-8);}
        sim->reset();CHECK(sim->matter()==700);CHECK(sim->powerUsed()==8);CHECK(sim->time()==0);CHECK(sim->enemies().count==0);
    }
    {
        auto a=std::make_unique<Simulation>(Config{4096,123,8});auto b=std::make_unique<Simulation>(Config{4096,123,8});
        a->build(0,TowerKind::Mortar);b->build(0,TowerKind::Mortar);a->startWave();b->startWave();
        for(int i=0;i<2200;++i) {a->tick();b->tick();}
        CHECK(a->stateHash()==b->stateHash());CHECK(a->stats().killed>0);CHECK(a->stats().shots>0);
        CHECK(a->reserve()+a->enemies().count+a->stats().killed==GlobalHorde);
        CHECK(a->stats().spawned==a->stats().killed+a->enemies().count);
        for(std::size_t i=0;i<a->enemies().count;++i) {CHECK(std::isfinite(a->enemies().x[i]));CHECK(a->enemies().health[i]>0);}
    }
    {
        auto sim=std::make_unique<Simulation>(Config{128,9,8});sim->startWave();
        bool capped=false;
        for(int i=0;i<3000;++i) {
            sim->tick();capped=capped || sim->enemies().count==128;
            if(sim->stats().ticks%100==0) CHECK(sim->reserve()+sim->enemies().count+sim->stats().killed==GlobalHorde);
        }
        CHECK(capped);CHECK(sim->stats().peakActive<=128);CHECK(sim->pending()>0);
        CHECK(sim->stats().spawned+static_cast<std::uint64_t>(sim->pending())==static_cast<std::uint64_t>(sim->waveSize()));
    }
    {
        auto sim=std::make_unique<Simulation>();sim->startWave();
        for(int i=0;i<30000 && sim->phase()!=Phase::Defeat && sim->phase()!=Phase::Victory;++i) sim->tick();
        CHECK(sim->phase()==Phase::Defeat);CHECK(sim->breaches()>0);CHECK(sim->coreHealth()==0);
        const auto ticks=sim->stats().ticks;sim->tick();CHECK(sim->stats().ticks==ticks);
        CHECK(sim->build(0,TowerKind::Mortar)==Result::Terminal);
    }
    {
        // End-to-end campaign: a commander reinvests, researches, repairs and uses overdrive.
        auto sim=std::make_unique<Simulation>();sim->startWave();
        for(int tick=0;tick<40000 && sim->phase()!=Phase::Defeat && sim->phase()!=Phase::Victory;++tick) {
            if(tick%30==0) {
                for(int j=0;j<WallCount;++j) if(sim->walls()[static_cast<std::size_t>(j)].health<sim->walls()[static_cast<std::size_t>(j)].maximum*0.75F) sim->repair(j);
                sim->research();
                for(int j=0;j<SocketCount;++j) {
                    const auto kind=j%4<2 ? TowerKind::Repeater : j%4==2 ? TowerKind::Mortar : TowerKind::Cryo;
                    if(sim->towers()[static_cast<std::size_t>(j)].kind==TowerKind::Empty) {
                        if(sim->powerUsed()+Simulation::powerCost(kind)>sim->powerAvailable()) sim->generator();
                        sim->build(j,kind);
                    } else sim->upgrade(j);
                }
                sim->overdrive();
            }
            sim->tick();
        }
        CHECK(sim->phase()==Phase::Victory);CHECK(sim->wave()==8);CHECK(sim->tech()==3);CHECK(sim->stats().bossesKilled==4);
        CHECK(sim->reserve()+sim->stats().killed==GlobalHorde);
    }
    {
        auto sim=std::make_unique<Simulation>(Config{4096,9,1});
        sim->build(0,TowerKind::Mortar);sim->upgrade(1);sim->startWave();
        for(int i=0;i<15000 && sim->phase()==Phase::Assault;++i) {if(i%300==0) sim->overdrive();sim->tick();}
        CHECK(sim->phase()==Phase::Victory);CHECK(sim->pending()==0);CHECK(sim->enemies().count==0);
        CHECK(sim->stats().killed==static_cast<std::uint64_t>(sim->waveSize()));
    }
    {
        auto sim=std::make_unique<Simulation>(Config{16384,44,8});sim->startStress(16384);
        allocations=0;track=true;
        for(int i=0;i<900;++i) sim->tick();
        sim->reset();sim->startWave();for(int i=0;i<1200;++i) sim->tick();
        track=false;
        CHECK(allocations.load()==0);CHECK(sim->ownedBytes()<2U*1024*1024);
        CHECK(sim->reserve()+sim->enemies().count+sim->stats().killed==GlobalHorde);
        std::puts("Hot simulation / reset / wave transitions: 0 C++ allocations");
    }
    if(failures) {std::fprintf(stderr,"%d checks failed\n",failures);return 1;}
    std::puts("All simulation, economy, cap, determinism, breach, victory and allocation checks passed.");
    return 0;
}
