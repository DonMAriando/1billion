#include "presentation.hpp"
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
struct SdlLifetime {
    SdlLifetime() {
        SDL_SetAppMetadata("1 Billion / Arca 09","0.1.0","io.donmariando.onebillion");
        if(!SDL_Init(SDL_INIT_VIDEO)) {const std::string error=SDL_GetError();SDL_Quit();throw std::runtime_error(error);}
    }
    ~SdlLifetime() {SDL_Quit();}
};
struct Options {
    std::size_t capacity{32768},stress{};
    std::uint32_t seed{9};
    int captureTicks{900},smokeFrames{};
    const char* capture{};
    bool audio{true},software{},selfTest{};
};
unsigned number(std::string_view text) {
    unsigned result{};const auto parsed=std::from_chars(text.data(),text.data()+text.size(),result);
    if(parsed.ec!=std::errc{} || parsed.ptr!=text.data()+text.size()) throw std::invalid_argument("invalid integer argument");
    return result;
}
bool readOptions(int argc,char** argv,Options& o) {
    for(int i=1;i<argc;++i) {
        const std::string_view arg=argv[i];
        if(arg=="--help") {
            std::puts("1 BILLION / ARCA 09 / C++20 + SDL 3.4.18\n"
                "  --capacity N       Concurrent enemy cap, 128..131072 (default 32768)\n"
                "  --seed N           Reproducible simulation seed\n"
                "  --stress N         Replenished population; no damage to walls or core\n"
                "  --demo             16384-enemy stress scene (not campaign progression)\n"
                "  --no-audio         Disable procedural audio\n"
                "  --software         Explicit software renderer\n"
                "  --capture FILE.bmp Render a deterministic scene, save and exit\n"
                "  --capture-ticks N  Scene warmup ticks (default 900)\n"
                "  --smoke-frames N   Exit after N frames (automation)\n"
                "  --self-test-ui     Verify keyboard/mouse commands and renderer\n"
                "\nENTER start / SPACE pause / F1 complete Spanish manual\n"
                "For simulation-only measurements run siege_benchmark.");return false;
        }
        if(arg=="--no-audio") {o.audio=false;continue;}
        if(arg=="--software") {o.software=true;continue;}
        if(arg=="--demo") {o.stress=16384;continue;}
        if(arg=="--self-test-ui") {o.selfTest=true;o.audio=false;o.software=true;continue;}
        if(arg!="--capacity" && arg!="--seed" && arg!="--stress" && arg!="--capture" && arg!="--capture-ticks" && arg!="--smoke-frames")
            throw std::invalid_argument(std::string("unknown argument: ")+std::string(arg));
        if(i+1==argc) throw std::invalid_argument("missing argument value");
        const char* value=argv[++i];
        if(arg=="--capture") {o.capture=value;o.audio=false;continue;}
        const unsigned n=number(value);
        if(arg=="--capacity") o.capacity=n;
        if(arg=="--stress") o.stress=n;
        if(arg=="--seed") o.seed=n;
        if(arg=="--capture-ticks") {if(n>30000) throw std::invalid_argument("capture ticks must be <=30000");o.captureTicks=static_cast<int>(n);}
        if(arg=="--smoke-frames") {if(n<1 || n>10000) throw std::invalid_argument("smoke frames must be 1..10000");o.smokeFrames=static_cast<int>(n);}
    }
    o.capacity=std::max(o.capacity,o.stress);
    if(o.capacity<128 || o.capacity>131072) throw std::invalid_argument("capacity must be 128..131072");
    return true;
}
void verifyInput(siege::Presentation& view,siege::Simulation& sim) {
    const auto key=[&](SDL_Keycode code) {SDL_Event event{};event.type=SDL_EVENT_KEY_DOWN;event.key.key=code;view.handle(event,sim);};
    const auto mouse=[&](Uint32 type,Uint8 button,float x,float y) {
        SDL_Event event{};event.type=type;event.button.button=button;event.button.x=x;event.button.y=y;view.handle(event,sim);
    };
    const auto check=[](bool ok,const char* detail) {if(!ok) throw std::runtime_error(std::string("UI verification failed: ")+detail);};
    view.render(sim,1,{});view.present();
    key(SDLK_E);check(sim.towers()[0].kind==siege::TowerKind::Mortar,"keyboard construction");
    key(SDLK_SPACE);check(view.paused(),"tactical pause");
    const auto p=siege::socketPosition(2);const float x=620+(p.x-p.y)*18*0.94F,y=475+(p.x+p.y)*9*0.94F;
    mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,SDL_BUTTON_LEFT,x,y);mouse(SDL_EVENT_MOUSE_BUTTON_UP,SDL_BUTTON_LEFT,x,y);
    key(SDLK_Q);check(sim.towers()[2].kind==siege::TowerKind::Repeater,"mouse socket selection");
    key(SDLK_1);mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,SDL_BUTTON_RIGHT,620,593);
    check(sim.squads()[0].destination.x>6 && sim.squads()[0].destination.y>6,"squad movement order");
    key(SDLK_N);check(!view.paused() && sim.matter()==700,"restart");
    key(SDLK_F1);check(view.paused(),"help pause");key(SDLK_ESCAPE);check(!view.paused(),"help dismissal");
    key(SDLK_RETURN);check(sim.phase()==siege::Phase::Assault,"start wave");
    for(int i=0;i<750;++i) sim.tick();
    view.render(sim,1,{});view.present();
    std::puts("UI checks passed: construction, mouse selection, squads, pause, restart, help and rendering.");
}
} // namespace

int main(int argc,char** argv) {
    try {
        Options options;if(!readOptions(argc,argv,options)) return 0;
        auto sim=std::make_unique<siege::Simulation>(siege::Config{options.capacity,options.seed,8});
        SdlLifetime sdl;
        auto view=std::make_unique<siege::Presentation>(options.capacity,options.software,options.audio,options.capture!=nullptr || options.selfTest || options.smokeFrames>0);
        if(options.selfTest) {verifyInput(*view,*sim);return 0;}
        if(options.stress>0) sim->startStress(options.stress);
        if(options.capture) {
            if(options.stress==0) {sim->build(0,siege::TowerKind::Mortar);sim->startWave();}
            for(int tick=0;tick<options.captureTicks;++tick) sim->tick();
            view->update(0,*sim);view->render(*sim,1,{});
            if(!view->capture(options.capture)) throw std::runtime_error(std::string("capture: ")+SDL_GetError());
            view->present();std::printf("Captured %s / %zu active / hash %016llx\n",options.capture,sim->enemies().count,static_cast<unsigned long long>(sim->stateHash()));return 0;
        }
        using Clock=std::chrono::steady_clock;
        auto previous=Clock::now();double accumulator=0;bool running=true;int frames=0;
        siege::FrameMetrics metrics;
        while(running) {
            const auto frameStart=Clock::now();
            const double elapsed=std::min(0.25,std::chrono::duration<double>(frameStart-previous).count());previous=frameStart;
            SDL_Event event{};while(SDL_PollEvent(&event)) if(!view->handle(event,*sim)) running=false;
            if(!running) break;
            metrics.steps=0;const auto simulationStart=Clock::now();
            if(view->paused()) accumulator=0;
            else {
                accumulator+=options.smokeFrames>0 ? static_cast<double>(siege::FixedStep) : elapsed;
                while(accumulator>=siege::FixedStep && metrics.steps<5) {sim->tick();accumulator-=siege::FixedStep;++metrics.steps;}
                // Slow the simulation under overload instead of running an unbounded catch-up spiral.
                if(metrics.steps==5) accumulator=std::min(accumulator,static_cast<double>(siege::FixedStep));
            }
            metrics.simulationMs=std::chrono::duration<double,std::milli>(Clock::now()-simulationStart).count();
            metrics.frameMs=metrics.frameMs*0.9+elapsed*1000*0.1;
            view->update(static_cast<float>(elapsed),*sim);
            view->render(*sim,view->paused() ? 1 : static_cast<float>(accumulator/siege::FixedStep),metrics);view->present();
            ++frames;if(options.smokeFrames>0 && frames>=options.smokeFrames) break;
            const double spent=std::chrono::duration<double>(Clock::now()-frameStart).count();
            if(spent<1.0/60.0) SDL_DelayNS(static_cast<Uint64>((1.0/60.0-spent)*1.0e9));
        }
    } catch(const std::exception& e) {std::fprintf(stderr,"1 Billion: %s\n",e.what());return 1;}
    return 0;
}
