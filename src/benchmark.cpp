#include "simulation.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string_view>
#include <vector>

int main(int argc,char** argv) {
    std::size_t population=32768;
    int seconds=10;
    for(int i=1;i<argc;++i) {
        const std::string_view arg=argv[i];
        if(arg=="--help") {std::puts("siege_benchmark [--enemies 128..131072] [--seconds 1..300]\nMeasures SIMULATION ONLY, not rendering or frame rate.");return 0;}
        if((arg!="--enemies" && arg!="--seconds") || i+1==argc) {std::fprintf(stderr,"Unknown or incomplete argument\n");return 2;}
        const std::string_view value=argv[++i];
        unsigned parsed=0;const auto result=std::from_chars(value.data(),value.data()+value.size(),parsed);
        if(result.ec!=std::errc{} || result.ptr!=value.data()+value.size()) {std::fprintf(stderr,"Invalid number\n");return 2;}
        if(arg=="--enemies") population=parsed;else seconds=static_cast<int>(std::min(parsed,301U));
    }
    if(population<128 || population>131072 || seconds<1 || seconds>300) {std::fprintf(stderr,"Out of range\n");return 2;}
    try {
        auto sim=std::make_unique<siege::Simulation>(siege::Config{population,9,8});
        sim->startStress(population);
        for(int i=0;i<150;++i) sim->tick();
        const int ticks=seconds*30;
        std::vector<double> timings(static_cast<std::size_t>(ticks));
        std::size_t minimum=population;
        for(auto& elapsed:timings) {
            const auto start=std::chrono::steady_clock::now();sim->tick();
            elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
            minimum=std::min(minimum,sim->enemies().count);
        }
        std::sort(timings.begin(),timings.end());
        double total=0;for(double t:timings) total+=t;
        std::printf("SIMULATION ONLY / 30 fixed ticks per simulated second\n");
        std::printf("population_requested,population_min,ticks,mean_ms,p95_ms,max_ms,owned_mib,kills,hash\n");
        std::printf("%zu,%zu,%d,%.4f,%.4f,%.4f,%.3f,%llu,%016llx\n",population,minimum,ticks,
            total/static_cast<double>(ticks),timings[static_cast<std::size_t>(ticks*95/100)],timings.back(),
            static_cast<double>(sim->ownedBytes())/(1024*1024),static_cast<unsigned long long>(sim->stats().killed),
            static_cast<unsigned long long>(sim->stateHash()));
    } catch(const std::exception& e) {std::fprintf(stderr,"%s\n",e.what());return 1;}
    return 0;
}
