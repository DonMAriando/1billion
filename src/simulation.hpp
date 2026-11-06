#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace siege {

constexpr float FixedStep = 1.0F / 30.0F;
constexpr float CityHalf = 10.0F;
constexpr float WorldHalf = 48.0F;
constexpr int WallsPerSide = 12;
constexpr int WallCount = 4 * WallsPerSide;
constexpr int SocketCount = 24;
constexpr int MaxEffects = 2048;
constexpr int GridWidth = 64;
constexpr float CellSize = 1.5F;
constexpr std::uint64_t GlobalHorde = 1'000'000'000ULL;

struct Vec2 { float x{}, y{}; };
float distanceSquared(Vec2 a, Vec2 b) noexcept;
Vec2 wallPosition(int index) noexcept;
Vec2 socketPosition(int index) noexcept;
int nearestWall(Vec2 position) noexcept;

enum class EnemyKind : std::uint8_t { Drone, Runner, Brute, Spitter, Herald, Archon, Sovereign, Count };
enum class TowerKind : std::uint8_t { Empty, Repeater, Mortar, Cryo };
enum class Phase : std::uint8_t { Planning, Assault, Victory, Defeat };
enum class Result : std::uint8_t { Ok, Occupied, NoMatter, NoPower, Locked, MaxLevel, Invalid, Terminal };

struct EnemyStats { float health, speed, damage, reach, size; int salvage; };
const EnemyStats& enemyStats(EnemyKind kind) noexcept;
const char* enemyName(EnemyKind kind) noexcept;
const char* towerName(TowerKind kind) noexcept;
const char* resultText(Result result) noexcept;

// Dense, bounded SoA. A dead slot is filled with the last live slot; no per-agent allocation.
class EnemyPool {
public:
    explicit EnemyPool(std::size_t capacity);
    bool add(Vec2 position, float health, EnemyKind kind, std::uint16_t wall) noexcept;
    void erase(std::size_t index) noexcept;
    void clear() noexcept { count = 0; }
    std::size_t capacity() const noexcept { return x.size(); }
    std::size_t ownedBytes() const noexcept;

    std::size_t count{};
    std::vector<float> x, y, previousX, previousY, health, slowUntil;
    std::vector<std::uint16_t> wall;
    std::vector<EnemyKind> kind;
    std::vector<std::uint8_t> inside;
};

struct Wall { float health{850.0F}; float maximum{850.0F}; float flash{}; };
struct Tower { TowerKind kind{TowerKind::Empty}; int level{1}; float cooldown{}; float flash{}; };
struct Squad { Vec2 position{}, previous{}, destination{}; float cooldown{}; };
struct Effect { Vec2 from{}, to{}; float remaining{}, lifetime{}, radius{}; TowerKind kind{}; };
struct Config { std::size_t capacity{32768}; std::uint32_t seed{9}; int finalWave{8}; };
struct Stats {
    std::uint64_t killed{}, spawned{}, ticks{}, rejectedSpawns{}, shots{}, targetVisits{}, effectsReplaced{};
    std::size_t peakActive{};
    int bossesKilled{};
};
struct WaveInfo { const char* name; const char* briefing; const char* warning; };
const WaveInfo& waveInfo(int wave) noexcept;

class Simulation {
public:
    explicit Simulation(Config config = {});
    void reset() noexcept;
    void tick() noexcept;
    Result startWave() noexcept;
    Result build(int socket, TowerKind kind) noexcept;
    Result upgrade(int socket) noexcept;
    Result repair(int wall) noexcept;
    Result research() noexcept;
    Result generator() noexcept;
    Result overdrive() noexcept;
    void moveSquads(std::uint8_t selection, Vec2 destination) noexcept;
    void startStress(std::size_t population) noexcept;

    static int buildCost(TowerKind kind) noexcept;
    static int powerCost(TowerKind kind) noexcept;
    int upgradeCost(int socket) const noexcept;
    int researchCost() const noexcept { return tech_ == 1 ? 480 : 850; }
    int generatorCost() const noexcept { return 180 + generators_ * 100; }
    std::size_t ownedBytes() const noexcept;
    std::uint64_t stateHash() const noexcept;

    const EnemyPool& enemies() const noexcept { return enemies_; }
    const std::array<Wall, WallCount>& walls() const noexcept { return walls_; }
    const std::array<Tower, SocketCount>& towers() const noexcept { return towers_; }
    const std::array<Squad, 3>& squads() const noexcept { return squads_; }
    const std::array<Effect, MaxEffects>& effects() const noexcept { return effects_; }
    const Stats& stats() const noexcept { return stats_; }
    Phase phase() const noexcept { return phase_; }
    int wave() const noexcept { return wave_; }
    int finalWave() const noexcept { return config_.finalWave; }
    int focusSide() const noexcept { return focusSide_; }
    int matter() const noexcept { return matter_; }
    int tech() const noexcept { return tech_; }
    int powerUsed() const noexcept { return powerUsed_; }
    int powerAvailable() const noexcept { return 12 + generators_ * 4; }
    int generators() const noexcept { return generators_; }
    float coreHealth() const noexcept { return coreHealth_; }
    float planningTime() const noexcept { return planningTime_; }
    float time() const noexcept { return time_; }
    float pressure() const noexcept;
    float overdriveTime() const noexcept { return overdriveTime_; }
    float overdriveCooldown() const noexcept { return overdriveCooldown_; }
    float sideIntegrity(int side) const noexcept;
    int breaches() const noexcept;
    int pending() const noexcept { return pending_; }
    int waveSize() const noexcept { return waveSize_; }
    std::uint64_t reserve() const noexcept { return reserve_; }
    EnemyKind bossKind() const noexcept { return bossKind_; }
    bool bossAlive() const noexcept;
    bool isStress() const noexcept { return stress_; }

private:
    std::uint32_t random() noexcept;
    float unit() noexcept;
    bool terminal() const noexcept;
    void spawn(EnemyKind kind, int side, float advance = 0.0F) noexcept;
    void spawnWave() noexcept;
    void moveEnemies() noexcept;
    void rebuildGrid() noexcept;
    void fireDefenses() noexcept;
    void shoot(Vec2 origin, float range, float damage, TowerKind kind, float splash) noexcept;
    void splash(Vec2 center, float radius, float damage, bool slow) noexcept;
    void addEffect(Vec2 from, Vec2 to, TowerKind kind, float radius) noexcept;
    void removeDead() noexcept;
    int cell(Vec2 p) const noexcept;
    void finishWave() noexcept;

    Config config_;
    EnemyPool enemies_;
    std::vector<int> next_;
    std::array<int, GridWidth * GridWidth> heads_{};
    std::array<std::uint16_t, GridWidth * GridWidth> density_{};
    std::array<Wall, WallCount> walls_{};
    std::array<Tower, SocketCount> towers_{};
    std::array<Squad, 3> squads_{};
    std::array<Effect, MaxEffects> effects_{};
    Stats stats_{};
    Phase phase_{Phase::Planning};
    std::uint64_t rng_{}, reserve_{GlobalHorde};
    float time_{}, coreHealth_{1000}, planningTime_{30}, spawnCredit_{}, incomeCredit_{};
    float overdriveTime_{}, overdriveCooldown_{};
    int wave_{}, pending_{}, waveSize_{}, focusSide_{}, matter_{700}, powerUsed_{8};
    int generators_{}, tech_{1};
    std::size_t effectCursor_{}, stressPopulation_{};
    EnemyKind bossKind_{EnemyKind::Drone};
    int bossesPending_{};
    bool stress_{};
};
} // namespace siege
