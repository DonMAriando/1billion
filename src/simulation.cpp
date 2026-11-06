#include "simulation.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace siege {
namespace {
constexpr std::array<EnemyStats, 7> Archetypes{{
    {12, 1.45F, 0.46F, 0.65F, 0.22F, 1},
    {8, 2.65F, 0.40F, 0.65F, 0.18F, 1},
    {120, 0.82F, 5.5F, 0.80F, 0.43F, 9},
    {26, 1.10F, 1.7F, 4.2F, 0.27F, 3},
    {240, 0.95F, 3.0F, 2.8F, 0.55F, 18},
    {4200, 0.65F, 45.0F, 1.2F, 1.15F, 250},
    {18000, 0.48F, 110.0F, 2.0F, 1.85F, 700}
}};
constexpr std::array<WaveInfo, 8> Waves{{
    {"EL RUIDO", "El Coro encontro el reactor. Conserva el perimetro.", "CONTACTO MASIVO / DRONES"},
    {"LA CARRERA", "Las sondas aprendieron nuestras distancias de tiro.", "CORREDORES / MAYOR VELOCIDAD"},
    {"EL ARIETE", "Un Arconte coordina el ataque. Concentra el fuego.", "ARCONTE / BLINDAJE PESADO"},
    {"LLUVIA ACIDA", "Los corrosivos atacan desde fuera de la muralla.", "ARTILLERIA BIOLOGICA / LARGO ALCANCE"},
    {"EL MENSAJERO", "Los Heraldos aceleran a las unidades de su frente.", "HERALDOS / AURA DE MANDO"},
    {"LA RESPUESTA", "Dos mentes, una orden: abrir el refugio.", "ARCONTE REFORZADO / TODOS LOS FRENTES"},
    {"SIN HORIZONTE", "Ya no se ve el suelo. El salto esta casi listo.", "SATURACION / HORDA DE ELITE"},
    {"EL SOBERANO", "Su mente entro al frente. Resiste hasta sellar el Arca.", "SOBERANO / ASALTO FINAL"}
}};
std::size_t validCapacity(std::size_t value) {
    if (value < 128 || value > 131072) throw std::invalid_argument("capacity must be 128..131072");
    return value;
}
bool validSocket(int socket) noexcept { return socket >= 0 && socket < SocketCount; }
bool validWall(int wall) noexcept { return wall >= 0 && wall < WallCount; }
float length(Vec2 v) noexcept { return std::sqrt(v.x * v.x + v.y * v.y); }
} // namespace

float distanceSquared(Vec2 a, Vec2 b) noexcept { const float x=a.x-b.x, y=a.y-b.y; return x*x+y*y; }
Vec2 wallPosition(int index) noexcept {
    if (!validWall(index)) return {};
    const float along = -CityHalf + (static_cast<float>(index % WallsPerSide) + 0.5F) * (2*CityHalf / WallsPerSide);
    switch(index / WallsPerSide) {
        case 0: return {along, -CityHalf};
        case 1: return {CityHalf, along};
        case 2: return {along, CityHalf};
        default: return {-CityHalf, along};
    }
}
Vec2 socketPosition(int index) noexcept {
    if (!validSocket(index)) return {};
    Vec2 p = wallPosition(index / 6 * WallsPerSide + index % 6 * 2 + 1);
    if (index / 6 == 0) p.y += 1.35F;
    if (index / 6 == 1) p.x -= 1.35F;
    if (index / 6 == 2) p.y -= 1.35F;
    if (index / 6 == 3) p.x += 1.35F;
    return p;
}
int nearestWall(Vec2 p) noexcept {
    float best = std::numeric_limits<float>::max(); int result=0;
    for(int i=0;i<WallCount;++i) { const float d=distanceSquared(p,wallPosition(i)); if(d<best) {best=d;result=i;} }
    return result;
}
const EnemyStats& enemyStats(EnemyKind kind) noexcept { return Archetypes[static_cast<std::size_t>(kind)]; }
const char* enemyName(EnemyKind kind) noexcept {
    constexpr const char* names[]{"DRON", "CORREDOR", "ARIETE", "CORROSIVO", "HERALDO", "ARCONTE", "SOBERANO"};
    return names[static_cast<std::size_t>(kind)];
}
const char* towerName(TowerKind kind) noexcept {
    constexpr const char* names[]{"ZOCALO VACIO", "REPETIDORA", "MORTERO", "CRIOEMISOR"};
    return names[static_cast<std::size_t>(kind)];
}
const char* resultText(Result result) noexcept {
    switch(result) {
        case Result::Ok: return "ORDEN ACEPTADA";
        case Result::Occupied: return "ESTE ZOCALO YA TIENE UNA DEFENSA";
        case Result::NoMatter: return "MATERIA INSUFICIENTE";
        case Result::NoPower: return "FALTA POTENCIA / AMPLIA EL REACTOR [G]";
        case Result::Locked: return "TECNOLOGIA BLOQUEADA / CONSULTA INVESTIGACION";
        case Result::MaxLevel: return "NIVEL MAXIMO ALCANZADO";
        case Result::Terminal: return "PARTIDA FINALIZADA / REINICIA CON [N]";
        default: return "SELECCIONA UN ZOCALO O TRAMO DE MURALLA";
    }
}
const WaveInfo& waveInfo(int wave) noexcept { return Waves[static_cast<std::size_t>(std::clamp(wave,1,8)-1)]; }

EnemyPool::EnemyPool(std::size_t capacity)
    : x(validCapacity(capacity)), y(capacity), previousX(capacity), previousY(capacity), health(capacity),
      slowUntil(capacity), wall(capacity), kind(capacity), inside(capacity) {}
bool EnemyPool::add(Vec2 position, float hp, EnemyKind type, std::uint16_t target) noexcept {
    if(count == capacity()) return false;
    const auto i = count++;
    x[i]=previousX[i]=position.x; y[i]=previousY[i]=position.y;
    health[i]=hp; slowUntil[i]=0; wall[i]=target; kind[i]=type; inside[i]=0;
    return true;
}
void EnemyPool::erase(std::size_t index) noexcept {
    if(index>=count) return;
    const auto last=--count;
    x[index]=x[last]; y[index]=y[last]; previousX[index]=previousX[last]; previousY[index]=previousY[last];
    health[index]=health[last]; slowUntil[index]=slowUntil[last]; wall[index]=wall[last]; kind[index]=kind[last];inside[index]=inside[last];
}
std::size_t EnemyPool::ownedBytes() const noexcept {
    return (x.capacity()+y.capacity()+previousX.capacity()+previousY.capacity()+health.capacity()+slowUntil.capacity())*sizeof(float)
        +wall.capacity()*sizeof(std::uint16_t)+kind.capacity()*sizeof(EnemyKind)+inside.capacity()*sizeof(std::uint8_t);
}

Simulation::Simulation(Config config) : config_(config), enemies_(config.capacity), next_(config.capacity) {
    if (config.finalWave<1 || config.finalWave>8) throw std::invalid_argument("finalWave must be 1..8");
    reset();
}
void Simulation::reset() noexcept {
    enemies_.clear(); heads_.fill(-1); density_.fill(0); walls_.fill(Wall{}); towers_.fill(Tower{});
    effects_.fill(Effect{}); stats_={}; rng_=static_cast<std::uint64_t>(config_.seed)+0x9e3779b97f4a7c15ULL;
    phase_=Phase::Planning; reserve_=GlobalHorde; time_=0; coreHealth_=1000; planningTime_=30;
    spawnCredit_=incomeCredit_=0; overdriveTime_=overdriveCooldown_=0; wave_=pending_=waveSize_=focusSide_=0;
    matter_=700; powerUsed_=8; generators_=0; tech_=1; effectCursor_=stressPopulation_=0;
    bossKind_=EnemyKind::Drone; bossesPending_=0;stress_=false;
    for(int side=0;side<4;++side) for(int slot : {1,4}) towers_[static_cast<std::size_t>(side*6+slot)].kind=TowerKind::Repeater;
    for(std::size_t i=0;i<squads_.size();++i) {
        const Vec2 p{(static_cast<float>(i)-1.0F)*3.0F,1.5F}; squads_[i]={p,p,p,0};
    }
}
std::uint32_t Simulation::random() noexcept {
    rng_ ^= rng_>>12; rng_ ^= rng_<<25; rng_ ^= rng_>>27;
    return static_cast<std::uint32_t>((rng_*2685821657736338717ULL)>>32);
}
float Simulation::unit() noexcept { return static_cast<float>(random()>>8)*(1.0F/16777216.0F); }
bool Simulation::terminal() const noexcept { return phase_==Phase::Victory || phase_==Phase::Defeat; }
Result Simulation::startWave() noexcept {
    if(terminal()) return Result::Terminal;
    if(phase_!=Phase::Planning) return Result::Invalid;
    ++wave_; phase_=Phase::Assault; planningTime_=0; focusSide_=(wave_*3+static_cast<int>(config_.seed%4))%4;
    waveSize_=static_cast<int>(1800.0F*std::pow(1.37F,static_cast<float>(wave_-1)));
    pending_=waveSize_; spawnCredit_=0;
    bossesPending_=wave_==6 ? 2 : (wave_==3 || wave_==8 ? 1 : 0);
    bossKind_=wave_==8 ? EnemyKind::Sovereign : EnemyKind::Archon;
    return Result::Ok;
}
void Simulation::spawn(EnemyKind kind, int side, float advance) noexcept {
    if(enemies_.count==enemies_.capacity() || reserve_==0) {++stats_.rejectedSpawns;return;}
    const int lane=static_cast<int>(random()%WallsPerSide);
    const int wall=side*WallsPerSide+lane;
    const float spread=-29.0F+unit()*58.0F;
    const float radius=35.0F+unit()*11.0F-advance;
    Vec2 p{};
    if(side==0) p={spread,-radius};
    if(side==1) p={radius,spread};
    if(side==2) p={spread,radius};
    if(side==3) p={-radius,spread};
    const float multiplier=1.0F+static_cast<float>(std::max(0,wave_-1))*0.095F;
    enemies_.add(p,enemyStats(kind).health*multiplier,kind,static_cast<std::uint16_t>(wall));
    --reserve_; ++stats_.spawned;
}
void Simulation::spawnWave() noexcept {
    if(stress_) {
        while(enemies_.count<stressPopulation_ && reserve_>0) {
            const auto roll=random()%100;
            const EnemyKind kind=roll<74 ? EnemyKind::Drone : roll<89 ? EnemyKind::Runner : roll<95 ? EnemyKind::Brute : roll<99 ? EnemyKind::Spitter : EnemyKind::Herald;
            spawn(kind,static_cast<int>(random()%4),unit()*20);
        }
        return;
    }
    spawnCredit_=std::min(spawnCredit_+(85.0F+static_cast<float>(wave_)*29.0F)*FixedStep,256.0F);
    int budget=256;
    while(spawnCredit_>=1 && pending_>0 && enemies_.count<enemies_.capacity() && budget-->0) {
        int side=static_cast<int>(random()%4);
        if(random()%100<52) side=focusSide_;
        const auto roll=random()%100;
        EnemyKind kind=EnemyKind::Drone;
        if(wave_>=2 && roll<25) kind=EnemyKind::Runner;
        if(wave_>=3 && roll>=90) kind=EnemyKind::Brute;
        if(wave_>=4 && roll>=77 && roll<90) kind=EnemyKind::Spitter;
        if(wave_>=5 && roll==76) kind=EnemyKind::Herald;
        if(bossesPending_>0 && pending_<=waveSize_/2) {
            kind=bossKind_;side=wave_==6 && bossesPending_==1 ? (focusSide_+2)%4 : focusSide_;--bossesPending_;
        }
        spawn(kind,side); --pending_; spawnCredit_-=1;
    }
}
int Simulation::cell(Vec2 p) const noexcept {
    const int x=std::clamp(static_cast<int>((p.x+WorldHalf)/CellSize),0,GridWidth-1);
    const int y=std::clamp(static_cast<int>((p.y+WorldHalf)/CellSize),0,GridWidth-1);
    return y*GridWidth+x;
}
void Simulation::moveEnemies() noexcept {
    std::array<bool,4> heralds{};
    for(std::size_t i=0;i<enemies_.count;++i)
        if(enemies_.kind[i]==EnemyKind::Herald && enemies_.health[i]>0) heralds[enemies_.wall[i]/WallsPerSide]=true;
    for(auto& wall:walls_) wall.flash=std::max(0.0F,wall.flash-FixedStep*2.5F);
    for(std::size_t i=0;i<enemies_.count;++i) {
        Vec2 p{enemies_.x[i],enemies_.y[i]}; enemies_.previousX[i]=p.x; enemies_.previousY[i]=p.y;
        const auto& spec=enemyStats(enemies_.kind[i]);
        auto& wall=walls_[enemies_.wall[i]];
        const bool breach=wall.health<=0;
        const bool interior=enemies_.inside[i]!=0;
        Vec2 target=interior ? Vec2{} : wallPosition(enemies_.wall[i]);
        Vec2 delta{target.x-p.x,target.y-p.y};const float d2=delta.x*delta.x+delta.y*delta.y;
        const float reach=interior ? 1.65F : breach ? 0.65F : spec.reach;
        if(!interior && breach && d2<=reach*reach) {enemies_.inside[i]=1;continue;}
        if(d2<=reach*reach) {
            if(!stress_) {
                const float damage=spec.damage*FixedStep*(1.0F+0.10F*static_cast<float>(wave_-1));
                if(interior) coreHealth_=std::max(0.0F,coreHealth_-damage*1.6F);
                else {wall.health=std::max(0.0F,wall.health-damage);wall.flash=1;}
            }
        } else {
            const float d=std::sqrt(d2);
            float speed=spec.speed*(1.0F+0.025F*static_cast<float>(std::max(0,wave_-1)));
            if(heralds[enemies_.wall[i]/WallsPerSide]) speed*=1.25F;
            if(enemies_.slowUntil[i]>time_) speed*=0.38F;
            // Density from the previous tick gives bounded crowd resistance without pairwise tests.
            const float crowd=static_cast<float>(density_[static_cast<std::size_t>(cell(p))]);
            speed*=1.0F/(1.0F+std::min(crowd,120.0F)*0.004F);
            const float move=std::min(speed*FixedStep,d-0.01F);
            p.x+=delta.x/d*move; p.y+=delta.y/d*move;
            enemies_.x[i]=p.x; enemies_.y[i]=p.y;
        }
    }
}
void Simulation::rebuildGrid() noexcept {
    heads_.fill(-1); density_.fill(0);
    for(std::size_t i=0;i<enemies_.count;++i) {
        const auto c=static_cast<std::size_t>(cell({enemies_.x[i],enemies_.y[i]}));
        next_[i]=heads_[c]; heads_[c]=static_cast<int>(i);
        if(density_[c]<std::numeric_limits<std::uint16_t>::max()) ++density_[c];
    }
}
void Simulation::addEffect(Vec2 from,Vec2 to,TowerKind kind,float radius) noexcept {
    auto& effect=effects_[effectCursor_];
    if(effect.remaining>0) ++stats_.effectsReplaced;
    const float lifetime=kind==TowerKind::Mortar ? 0.38F : 0.13F;
    effect={from,to,lifetime,lifetime,radius,kind}; effectCursor_=(effectCursor_+1)%effects_.size();
}
void Simulation::splash(Vec2 center,float radius,float damage,bool slow) noexcept {
    const int minX=std::clamp(static_cast<int>((center.x-radius+WorldHalf)/CellSize),0,GridWidth-1);
    const int maxX=std::clamp(static_cast<int>((center.x+radius+WorldHalf)/CellSize),0,GridWidth-1);
    const int minY=std::clamp(static_cast<int>((center.y-radius+WorldHalf)/CellSize),0,GridWidth-1);
    const int maxY=std::clamp(static_cast<int>((center.y+radius+WorldHalf)/CellSize),0,GridWidth-1);
    for(int y=minY;y<=maxY;++y) for(int x=minX;x<=maxX;++x)
        for(int i=heads_[static_cast<std::size_t>(y*GridWidth+x)];i!=-1;i=next_[static_cast<std::size_t>(i)]) {
            ++stats_.targetVisits; const auto index=static_cast<std::size_t>(i);
            if(enemies_.health[index]<=0 || distanceSquared(center,{enemies_.x[index],enemies_.y[index]})>radius*radius) continue;
            enemies_.health[index]-=damage;
            if(slow) enemies_.slowUntil[index]=time_+2.8F;
        }
}
void Simulation::shoot(Vec2 origin,float range,float damage,TowerKind kind,float splashRadius) noexcept {
    const int minX=std::clamp(static_cast<int>((origin.x-range+WorldHalf)/CellSize),0,GridWidth-1);
    const int maxX=std::clamp(static_cast<int>((origin.x+range+WorldHalf)/CellSize),0,GridWidth-1);
    const int minY=std::clamp(static_cast<int>((origin.y-range+WorldHalf)/CellSize),0,GridWidth-1);
    const int maxY=std::clamp(static_cast<int>((origin.y+range+WorldHalf)/CellSize),0,GridWidth-1);
    int target=-1; float best=std::numeric_limits<float>::max();
    for(int y=minY;y<=maxY;++y) for(int x=minX;x<=maxX;++x)
        for(int i=heads_[static_cast<std::size_t>(y*GridWidth+x)];i!=-1;i=next_[static_cast<std::size_t>(i)]) {
            ++stats_.targetVisits; const auto index=static_cast<std::size_t>(i);
            if(enemies_.health[index]<=0) continue;
            const Vec2 p{enemies_.x[index],enemies_.y[index]};
            const float distance=distanceSquared(origin,p); if(distance>range*range) continue;
            float score=distanceSquared(p,wallPosition(enemies_.wall[index]))+distance*0.06F;
            if(enemies_.kind[index]>=EnemyKind::Archon) score-=2000;
            if(walls_[enemies_.wall[index]].health<=0) score-=200;
            // An intruder threatening the core always outranks an outside boss.
            if(enemies_.inside[index]) score-=10000;
            if(score<best) {target=i;best=score;}
        }
    if(target<0) return;
    const auto t=static_cast<std::size_t>(target); const Vec2 position{enemies_.x[t],enemies_.y[t]};
    if(splashRadius>0) splash(position,splashRadius,damage,kind==TowerKind::Cryo);
    else enemies_.health[t]-=damage;
    addEffect(origin,position,kind,splashRadius); ++stats_.shots;
}
void Simulation::fireDefenses() noexcept {
    for(int i=0;i<SocketCount;++i) {
        auto& t=towers_[static_cast<std::size_t>(i)]; t.flash=std::max(0.0F,t.flash-FixedStep*4);
        if(t.kind==TowerKind::Empty) continue;
        t.cooldown-=FixedStep; if(t.cooldown>0) continue;
        const float multiplier=(1.0F+static_cast<float>(t.level-1)*0.65F)*(1.0F+static_cast<float>(tech_-1)*0.55F);
        float delay=0.25F, range=11, damage=18, radius=0;
        if(t.kind==TowerKind::Mortar) {delay=1.5F;range=18;damage=38;radius=2.6F;}
        if(t.kind==TowerKind::Cryo) {delay=0.65F;range=11;damage=7;radius=2.4F;}
        const auto before=stats_.shots;
        shoot(socketPosition(i),range+static_cast<float>(t.level-1),damage*multiplier,t.kind,radius);
        t.cooldown=stats_.shots==before ? 0.12F : delay*(overdriveTime_>0 ? 0.50F : 1.0F);
        if(stats_.shots!=before) t.flash=1;
    }
    for(auto& squad:squads_) {
        squad.previous=squad.position;
        Vec2 delta{squad.destination.x-squad.position.x,squad.destination.y-squad.position.y};
        const float d=length(delta);
        if(d>0.03F) {const float m=std::min(d,3.5F*FixedStep);squad.position.x+=delta.x/d*m;squad.position.y+=delta.y/d*m;}
        squad.cooldown-=FixedStep;
        if(squad.cooldown<=0) {
            shoot(squad.position,10.0F,22.0F*(1.0F+0.4F*static_cast<float>(tech_-1)),TowerKind::Repeater,0);
            squad.cooldown=0.30F;
        }
    }
}
void Simulation::removeDead() noexcept {
    std::size_t i=0;
    while(i<enemies_.count) {
        if(enemies_.health[i]>0) {++i;continue;}
        matter_=std::min(999999,matter_+enemyStats(enemies_.kind[i]).salvage);
        if(enemies_.kind[i]>=EnemyKind::Archon) ++stats_.bossesKilled;
        enemies_.erase(i); ++stats_.killed;
    }
    // Grid links are never read again after dense compaction until the next rebuild.
}
void Simulation::finishWave() noexcept {
    if(wave_>=config_.finalWave) {phase_=Phase::Victory;return;}
    phase_=Phase::Planning; planningTime_=22; matter_=std::min(999999,matter_+180+wave_*35);
    for(auto& wall:walls_) if(wall.health>0) wall.health=std::min(wall.maximum,wall.health+70);
}
void Simulation::tick() noexcept {
    if(terminal()) return;
    ++stats_.ticks; time_+=FixedStep;
    overdriveTime_=std::max(0.0F,overdriveTime_-FixedStep);
    overdriveCooldown_=std::max(0.0F,overdriveCooldown_-FixedStep);
    for(auto& effect:effects_) effect.remaining=std::max(0.0F,effect.remaining-FixedStep);
    incomeCredit_+=FixedStep*(3.0F+static_cast<float>(generators_));
    if(incomeCredit_>=1) {const int amount=static_cast<int>(incomeCredit_);matter_=std::min(999999,matter_+amount);incomeCredit_-=static_cast<float>(amount);}
    if(phase_==Phase::Planning) {
        planningTime_-=FixedStep;
        // Squads can reposition during preparation too.
        for(auto& s:squads_) {s.previous=s.position;const Vec2 d{s.destination.x-s.position.x,s.destination.y-s.position.y};const float l=length(d);
            if(l>0.03F) {const float m=std::min(l,3.5F*FixedStep);s.position.x+=d.x/l*m;s.position.y+=d.y/l*m;}}
        if(planningTime_<=0) startWave();
        return;
    }
    spawnWave(); moveEnemies(); rebuildGrid(); fireDefenses(); removeDead();
    stats_.peakActive=std::max(stats_.peakActive,enemies_.count);
    if(!stress_ && coreHealth_<=0) phase_=Phase::Defeat;
    else if(!stress_ && pending_==0 && enemies_.count==0) finishWave();
}
int Simulation::buildCost(TowerKind kind) noexcept {
    switch(kind) {case TowerKind::Repeater:return 150;case TowerKind::Mortar:return 260;case TowerKind::Cryo:return 220;default:return 0;}
}
int Simulation::powerCost(TowerKind kind) noexcept { return kind==TowerKind::Empty ? 0 : kind==TowerKind::Repeater ? 1 : 2; }
Result Simulation::build(int socket,TowerKind kind) noexcept {
    if(terminal()) return Result::Terminal;
    if(!validSocket(socket) || kind<TowerKind::Repeater || kind>TowerKind::Cryo) return Result::Invalid;
    if(towers_[static_cast<std::size_t>(socket)].kind!=TowerKind::Empty) return Result::Occupied;
    if(kind==TowerKind::Cryo && tech_<2) return Result::Locked;
    if(powerUsed_+powerCost(kind)>powerAvailable()) return Result::NoPower;
    if(matter_<buildCost(kind)) return Result::NoMatter;
    matter_-=buildCost(kind); powerUsed_+=powerCost(kind); towers_[static_cast<std::size_t>(socket)]={kind,1,0,0}; return Result::Ok;
}
int Simulation::upgradeCost(int socket) const noexcept {
    return validSocket(socket) ? 130*towers_[static_cast<std::size_t>(socket)].level : 0;
}
Result Simulation::upgrade(int socket) noexcept {
    if(terminal()) return Result::Terminal;
    if(!validSocket(socket) || towers_[static_cast<std::size_t>(socket)].kind==TowerKind::Empty) return Result::Invalid;
    auto& t=towers_[static_cast<std::size_t>(socket)];
    if(t.level==3) return Result::MaxLevel;
    if(matter_<upgradeCost(socket)) return Result::NoMatter;
    matter_-=upgradeCost(socket);++t.level;return Result::Ok;
}
Result Simulation::repair(int index) noexcept {
    if(terminal()) return Result::Terminal;
    if(!validWall(index)) return Result::Invalid;
    auto& wall=walls_[static_cast<std::size_t>(index)];
    if(wall.health>=wall.maximum) return Result::MaxLevel;
    if(matter_<90) return Result::NoMatter;
    matter_-=90;wall.health=std::min(wall.maximum,wall.health+350);return Result::Ok;
}
Result Simulation::research() noexcept {
    if(terminal()) return Result::Terminal;
    if(tech_==3) return Result::MaxLevel;
    if(wave_<(tech_==1 ? 2 : 5)) return Result::Locked;
    if(matter_<researchCost()) return Result::NoMatter;
    matter_-=researchCost();++tech_;
    for(auto& wall:walls_) {wall.maximum+=250;if(wall.health>0) wall.health+=250;}
    return Result::Ok;
}
Result Simulation::generator() noexcept {
    if(terminal()) return Result::Terminal;
    if(generators_==6) return Result::MaxLevel;
    if(matter_<generatorCost()) return Result::NoMatter;
    matter_-=generatorCost();++generators_;return Result::Ok;
}
Result Simulation::overdrive() noexcept {
    if(terminal()) return Result::Terminal;
    if(overdriveCooldown_>0 || phase_!=Phase::Assault) return Result::Locked;
    overdriveTime_=8;overdriveCooldown_=45;return Result::Ok;
}
void Simulation::moveSquads(std::uint8_t selection,Vec2 destination) noexcept {
    if(terminal()) return;
    destination.x=std::clamp(destination.x,-8.0F,8.0F);destination.y=std::clamp(destination.y,-8.0F,8.0F);
    int ordinal=0;
    for(std::size_t i=0;i<squads_.size();++i) if(selection&(1U<<i)) {
        const float offset=static_cast<float>(ordinal++)*0.8F;
        squads_[i].destination={std::clamp(destination.x+offset,-8.0F,8.0F),destination.y};
    }
}
float Simulation::sideIntegrity(int side) const noexcept {
    if(side<0 || side>=4) return 0;
    float total=0,maximum=0;
    for(int i=0;i<WallsPerSide;++i) {const auto& w=walls_[static_cast<std::size_t>(side*WallsPerSide+i)];total+=w.health;maximum+=w.maximum;}
    return total/maximum;
}
int Simulation::breaches() const noexcept {int n=0;for(const auto& w:walls_) if(w.health<=0) ++n;return n;}
float Simulation::pressure() const noexcept {
    float worst=1; for(int i=0;i<4;++i) worst=std::min(worst,sideIntegrity(i));
    return std::clamp((1-worst)*0.70F+static_cast<float>(enemies_.count)/14000.0F+static_cast<float>(breaches())*0.04F,0.0F,1.0F);
}
bool Simulation::bossAlive() const noexcept {
    for(std::size_t i=0;i<enemies_.count;++i) if(enemies_.kind[i]>=EnemyKind::Archon) return true;
    return false;
}
void Simulation::startStress(std::size_t population) noexcept {
    reset();stress_=true;stressPopulation_=std::min(population,enemies_.capacity());phase_=Phase::Assault;wave_=7;
    tech_=3;matter_=999999;generators_=6;
    powerUsed_=0;
    for(int i=0;i<SocketCount;++i) {
        const auto kind=i%4<2 ? TowerKind::Repeater : i%4==2 ? TowerKind::Mortar : TowerKind::Cryo;
        towers_[static_cast<std::size_t>(i)]={kind,3,static_cast<float>(i)*0.01F,0};powerUsed_+=powerCost(kind);
    }
    spawnWave();rebuildGrid();stats_.peakActive=enemies_.count;
}
std::size_t Simulation::ownedBytes() const noexcept {return sizeof(*this)+enemies_.ownedBytes()+next_.capacity()*sizeof(int);}
std::uint64_t Simulation::stateHash() const noexcept {
    std::uint64_t hash=1469598103934665603ULL;
    auto mix=[&hash](std::uint64_t value) {hash^=value;hash*=1099511628211ULL;};
    mix(rng_);mix(stats_.ticks);mix(stats_.killed);mix(stats_.spawned);mix(static_cast<std::uint64_t>(matter_));
    mix(static_cast<std::uint64_t>(phase_));mix(static_cast<std::uint64_t>(wave_));mix(reserve_);
    mix(std::bit_cast<std::uint32_t>(coreHealth_));mix(std::bit_cast<std::uint32_t>(time_));
    for(const auto& w:walls_) mix(std::bit_cast<std::uint32_t>(w.health));
    for(std::size_t i=0;i<enemies_.count;++i) {
        mix(std::bit_cast<std::uint32_t>(enemies_.x[i]));mix(std::bit_cast<std::uint32_t>(enemies_.y[i]));
        mix(std::bit_cast<std::uint32_t>(enemies_.health[i]));mix(static_cast<std::uint64_t>(enemies_.kind[i]));mix(enemies_.wall[i]);
    }
    return hash;
}
} // namespace siege
