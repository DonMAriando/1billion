#pragma once

#include "simulation.hpp"
#include <SDL3/SDL.h>
#include <array>
#include <memory>
#include <vector>

namespace siege {
struct Color {float r,g,b,a{1};};
struct WindowDelete {void operator()(SDL_Window* p) const noexcept {SDL_DestroyWindow(p);}};
struct RendererDelete {void operator()(SDL_Renderer* p) const noexcept {SDL_DestroyRenderer(p);}};
struct TextureDelete {void operator()(SDL_Texture* p) const noexcept {SDL_DestroyTexture(p);}};
struct SurfaceDelete {void operator()(SDL_Surface* p) const noexcept {SDL_DestroySurface(p);}};
struct AudioDelete {void operator()(SDL_AudioStream* p) const noexcept {SDL_DestroyAudioStream(p);}};
using SurfacePtr=std::unique_ptr<SDL_Surface,SurfaceDelete>;

class Geometry {
public:
    explicit Geometry(std::size_t quads);
    void clear() noexcept {count_=0;}
    void quad(Vec2 a,Vec2 b,Vec2 c,Vec2 d,Color color,Vec2 uv0={0,0},Vec2 uv1={0,0}) noexcept;
    void rect(float x,float y,float width,float height,Color color) noexcept;
    void line(Vec2 a,Vec2 b,float width,Color color) noexcept;
    void flush(SDL_Renderer* renderer,SDL_Texture* texture=nullptr);
    std::size_t count() const noexcept {return count_;}
    std::size_t bytes() const noexcept;
private:
    std::vector<SDL_Vertex> vertices_;
    std::vector<int> indices_;
    std::size_t count_{};
};

struct FrameMetrics {double frameMs{16.7}, simulationMs{};int steps{};};

class Presentation {
public:
    Presentation(std::size_t capacity,bool software,bool audio,bool hidden);
    ~Presentation() = default;
    Presentation(const Presentation&)=delete;
    Presentation& operator=(const Presentation&)=delete;
    bool handle(SDL_Event event,Simulation& sim);
    void update(float realDelta,const Simulation& sim);
    void render(const Simulation& sim,float interpolation,FrameMetrics metrics);
    bool capture(const char* path);
    void present();
    bool paused() const noexcept {return paused_ || help_;}
    bool muted() const noexcept {return muted_;}
    std::size_t ownedBytes() const noexcept;

private:
    enum class Action {Start,Pause,Repeater,Mortar,Cryo,Upgrade,Repair,Research,Generator,Overdrive,Squad1,Squad2,Squad3,Reset,Help};
    struct Button {SDL_FRect bounds;Action action;};
    struct DrawItem {Vec2 position;float width,height,elevation;Color color;int type,next;};
    struct Distant {Vec2 position;float size,shade;};
    Vec2 project(Vec2 p,float z=0) const noexcept;
    Vec2 unproject(Vec2 p) const noexcept;
    bool visible(Vec2 p,float margin=80) const noexcept;
    void fontInit();
    void text(float x,float y,const char* value,float scale,Color color);
    void paragraph(float x,float y,const char* value,int columns,float scale,Color color);
    void box(Vec2 position,float halfWidth,float halfHeight,float elevation,Color color);
    void diamond(Vec2 center,float halfWidth,float halfHeight,Color color,float z=0);
    void world(const Simulation& sim,float alpha);
    void interface(const Simulation& sim,FrameMetrics metrics);
    void helpOverlay();
    void enqueue(DrawItem item) noexcept;
    void button(float x,float y,float width,float height,const char* label,Action action,bool enabled=true,bool accent=false);
    void meter(float x,float y,float width,float height,float value,Color color);
    void invoke(Action action,Simulation& sim);
    void choose(Vec2 mouse,const Simulation& sim);
    void notify(Result result) noexcept;
    void audioUpdate(const Simulation& sim);

    // Declaration order is ownership order; reverse destruction frees audio/font/renderer before window.
    std::unique_ptr<SDL_Window,WindowDelete> window_;
    std::unique_ptr<SDL_Renderer,RendererDelete> renderer_;
    std::unique_ptr<SDL_Texture,TextureDelete> font_;
    std::unique_ptr<SDL_AudioStream,AudioDelete> audio_;
    Geometry geometry_,letters_;
    std::vector<DrawItem> items_;
    std::array<int,384> depthHeads_{};
    std::array<Distant,12000> distant_{};
    std::array<Button,40> buttons_{};
    std::array<float,2048> audioSamples_{};
    std::array<char,160> message_{};
    std::size_t itemCount_{},buttonCount_{};
    Vec2 camera_{620,475},mouse_{},dragStart_{},panStart_{};
    float zoom_{0.94F},messageTime_{},elapsed_{};
    double audioTime_{};
    std::uint32_t noise_{937};
    int selectedSocket_{0},selectedWall_{-1},lastWave_{-1};
    std::uint8_t selection_{1};
    bool paused_{},help_{},telemetry_{},muted_{},dragging_{},panning_{},reduceMotion_{},fullscreen_{};
};
} // namespace siege
