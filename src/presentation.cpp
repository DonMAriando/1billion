#include "presentation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace siege {
namespace {
constexpr Color Background{0.031F,0.047F,0.067F};
constexpr Color Panel{0.055F,0.078F,0.098F};
constexpr Color Raised{0.080F,0.110F,0.133F};
constexpr Color Border{0.14F,0.20F,0.23F};
constexpr Color White{0.88F,0.93F,0.90F};
constexpr Color Muted{0.40F,0.52F,0.54F};
constexpr Color Teal{0.24F,0.90F,0.74F};
constexpr Color Red{1.0F,0.32F,0.36F};
constexpr Color Gold{1.0F,0.76F,0.36F};
constexpr Color Violet{0.66F,0.48F,0.98F};
constexpr const char* Sides[]{"NORTE", "ESTE", "SUR", "OESTE"};
constexpr SDL_Rect WorldClip{20,170,1204,660};
Color shade(Color c,float amount) noexcept {return {c.r*amount,c.g*amount,c.b*amount,c.a};}
Color mix(Color a,Color b,float f) noexcept {return {a.r+(b.r-a.r)*f,a.g+(b.g-a.g)*f,a.b+(b.b-a.b)*f,a.a+(b.a-a.a)*f};}
float dist(Vec2 a,Vec2 b) noexcept {return std::sqrt(distanceSquared(a,b));}
bool contains(SDL_FRect r,Vec2 p) noexcept {return p.x>=r.x && p.y>=r.y && p.x<=r.x+r.w && p.y<=r.y+r.h;}
std::array<std::uint8_t,7> glyph(char c) noexcept {
    switch(c) {
        case 'A':return {14,17,17,31,17,17,17};case 'B':return {30,17,17,30,17,17,30};
        case 'C':return {14,17,16,16,16,17,14};case 'D':return {30,17,17,17,17,17,30};
        case 'E':return {31,16,16,30,16,16,31};case 'F':return {31,16,16,30,16,16,16};
        case 'G':return {14,17,16,23,17,17,15};case 'H':return {17,17,17,31,17,17,17};
        case 'I':return {31,4,4,4,4,4,31};case 'J':return {7,2,2,2,2,18,12};
        case 'K':return {17,18,20,24,20,18,17};case 'L':return {16,16,16,16,16,16,31};
        case 'M':return {17,27,21,21,17,17,17};case 'N':return {17,25,21,19,17,17,17};
        case 'O':return {14,17,17,17,17,17,14};case 'P':return {30,17,17,30,16,16,16};
        case 'Q':return {14,17,17,17,21,18,13};case 'R':return {30,17,17,30,20,18,17};
        case 'S':return {15,16,16,14,1,1,30};case 'T':return {31,4,4,4,4,4,4};
        case 'U':return {17,17,17,17,17,17,14};case 'V':return {17,17,17,17,17,10,4};
        case 'W':return {17,17,17,21,21,21,10};case 'X':return {17,17,10,4,10,17,17};
        case 'Y':return {17,17,10,4,4,4,4};case 'Z':return {31,1,2,4,8,16,31};
        case '0':return {14,17,19,21,25,17,14};case '1':return {4,12,4,4,4,4,14};
        case '2':return {14,17,1,2,4,8,31};case '3':return {30,1,1,14,1,1,30};
        case '4':return {2,6,10,18,31,2,2};case '5':return {31,16,16,30,1,1,30};
        case '6':return {14,16,16,30,17,17,14};case '7':return {31,1,2,4,8,8,8};
        case '8':return {14,17,17,14,17,17,14};case '9':return {14,17,17,15,1,1,14};
        case ':':return {0,4,4,0,4,4,0};case '.':return {0,0,0,0,0,6,6};
        case ',':return {0,0,0,0,6,6,4};case '/':return {1,2,2,4,8,8,16};
        case '-':return {0,0,0,31,0,0,0};case '+':return {0,4,4,31,4,4,0};
        case '%':return {25,25,2,4,8,19,19};case '[':return {14,8,8,8,8,8,14};
        case ']':return {14,2,2,2,2,2,14};case '(':return {2,4,8,8,8,4,2};
        case ')':return {8,4,2,2,2,4,8};case '!':return {4,4,4,4,4,0,4};
        case '?':return {14,17,1,2,4,0,4};case '=':return {0,31,0,31,0,0,0};
        case '>':return {16,8,4,2,4,8,16};case '<':return {1,2,4,8,4,2,1};
        case '_':return {0,0,0,0,0,0,31};case '|':return {4,4,4,4,4,4,4};
        case '#':return {10,10,31,10,31,10,10};case '*':return {0,21,14,31,14,21,0};
        default:return {};
    }
}
void require(bool result,const char* operation) {if(!result) throw std::runtime_error(std::string(operation)+": "+SDL_GetError());}
Color enemyColor(EnemyKind k) noexcept {
    switch(k) {
        case EnemyKind::Runner:return {1,0.39F,0.42F};case EnemyKind::Brute:return Gold;
        case EnemyKind::Spitter:return {0.70F,0.86F,0.31F};case EnemyKind::Herald:return Violet;
        case EnemyKind::Archon:return {1,0.58F,0.23F};case EnemyKind::Sovereign:return {1,0.20F,0.45F};
        default:return {0.86F,0.23F,0.36F};
    }
}
} // namespace

Geometry::Geometry(std::size_t quads) : vertices_(quads*4),indices_(quads*6) {
    for(std::size_t i=0;i<quads;++i) {
        const int v=static_cast<int>(i*4);const std::size_t j=i*6;
        indices_[j]=v;indices_[j+1]=v+1;indices_[j+2]=v+2;
        indices_[j+3]=v;indices_[j+4]=v+2;indices_[j+5]=v+3;
    }
}
void Geometry::quad(Vec2 a,Vec2 b,Vec2 c,Vec2 d,Color color,Vec2 uv0,Vec2 uv1) noexcept {
    if(count_*4==vertices_.size()) return; // Cosmetic saturation is bounded; simulation is unaffected.
    const SDL_FColor f{color.r,color.g,color.b,color.a};const std::size_t i=count_++*4;
    vertices_[i]={{a.x,a.y},f,{uv0.x,uv0.y}};vertices_[i+1]={{b.x,b.y},f,{uv1.x,uv0.y}};
    vertices_[i+2]={{c.x,c.y},f,{uv1.x,uv1.y}};vertices_[i+3]={{d.x,d.y},f,{uv0.x,uv1.y}};
}
void Geometry::rect(float x,float y,float width,float height,Color color) noexcept {quad({x,y},{x+width,y},{x+width,y+height},{x,y+height},color);}
void Geometry::line(Vec2 a,Vec2 b,float width,Color color) noexcept {
    const float l=dist(a,b);if(l<0.001F) return;
    const float x=(b.y-a.y)/l*width*0.5F,y=(a.x-b.x)/l*width*0.5F;
    quad({a.x+x,a.y+y},{b.x+x,b.y+y},{b.x-x,b.y-y},{a.x-x,a.y-y},color);
}
void Geometry::flush(SDL_Renderer* renderer,SDL_Texture* texture) {
    if(count_>0) require(SDL_RenderGeometry(renderer,texture,vertices_.data(),static_cast<int>(count_*4),indices_.data(),static_cast<int>(count_*6)),"geometry");
}
std::size_t Geometry::bytes() const noexcept {return vertices_.capacity()*sizeof(SDL_Vertex)+indices_.capacity()*sizeof(int);}

Presentation::Presentation(std::size_t capacity,bool software,bool audio,bool hidden)
    : geometry_(capacity*3+24000),letters_(14000),items_(capacity+1024) {
    SDL_SetHint(SDL_HINT_RENDER_LINE_METHOD,"3");
    window_.reset(SDL_CreateWindow("1 BILLION / ARCA 09",hidden ? 1600 : 1440,hidden ? 1000 : 900,SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY|(hidden ? SDL_WINDOW_HIDDEN : 0)));
    if(!window_) throw std::runtime_error(SDL_GetError());
    renderer_.reset(SDL_CreateRenderer(window_.get(),software ? "software" : nullptr));
    if(!renderer_) throw std::runtime_error(SDL_GetError());
    require(SDL_SetRenderLogicalPresentation(renderer_.get(),1600,1000,SDL_LOGICAL_PRESENTATION_LETTERBOX),"logical size");
    SDL_SetRenderVSync(renderer_.get(),hidden ? 0 : 1);
    require(SDL_SetRenderDrawBlendMode(renderer_.get(),SDL_BLENDMODE_BLEND),"blend mode");
    fontInit();
    std::uint32_t random=7719;
    auto unit=[&random]() {random=random*1664525U+1013904223U;return static_cast<float>(random>>8)/16777216.0F;};
    for(auto& dot:distant_) {
        do {dot.position={-47+unit()*94,-47+unit()*94};} while(std::max(std::abs(dot.position.x),std::abs(dot.position.y))<22);
        dot.size=0.06F+unit()*0.12F;dot.shade=0.45F+unit()*0.55F;
    }
    if(audio && SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        const SDL_AudioSpec spec{SDL_AUDIO_F32,1,48000};
        audio_.reset(SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr));
        if(audio_) SDL_ResumeAudioStreamDevice(audio_.get());
    }
    if(!audio_) muted_=true;
    std::snprintf(message_.data(),message_.size(),"SELECCIONA UN ZOCALO. CONSTRUYE DEFENSAS. ENTER INICIA EL ASALTO. F1 AYUDA.");messageTime_=18;
}
void Presentation::fontInit() {
    SurfacePtr surface(SDL_CreateSurface(96,48,SDL_PIXELFORMAT_RGBA32));
    if(!surface) throw std::runtime_error(SDL_GetError());
    require(SDL_FillSurfaceRect(surface.get(),nullptr,SDL_MapSurfaceRGBA(surface.get(),0,0,0,0)),"font background");
    const Uint32 ink=SDL_MapSurfaceRGBA(surface.get(),255,255,255,255);
    for(int code=32;code<128;++code) {
        const auto rows=glyph(static_cast<char>(code));const int index=code-32;
        for(int y=0;y<7;++y) for(int x=0;x<5;++x) if(rows[static_cast<std::size_t>(y)]&(1<<(4-x))) {
            const SDL_Rect pixel{index%16*6+x,index/16*8+y,1,1};require(SDL_FillSurfaceRect(surface.get(),&pixel,ink),"font pixel");
        }
    }
    font_.reset(SDL_CreateTextureFromSurface(renderer_.get(),surface.get()));
    if(!font_) throw std::runtime_error(SDL_GetError());
    require(SDL_SetTextureScaleMode(font_.get(),SDL_SCALEMODE_NEAREST),"font sampling");
    require(SDL_SetTextureBlendMode(font_.get(),SDL_BLENDMODE_BLEND),"font blending");
}
void Presentation::text(float x,float y,const char* value,float scale,Color color) {
    const float initial=x;
    while(*value) {
        char c=*value++;if(c=='\n') {x=initial;y+=10*scale;continue;}
        if(c>='a' && c<='z') c=static_cast<char>(c-'a'+'A');
        if(c>='!' && c<='~') {
            const int index=c-32;const float u=static_cast<float>(index%16*6)/96,v=static_cast<float>(index/16*8)/48;
            letters_.quad({x,y},{x+5*scale,y},{x+5*scale,y+7*scale},{x,y+7*scale},color,{u,v},{u+5.0F/96,v+7.0F/48});
        }
        x+=6*scale;
    }
}
void Presentation::paragraph(float x,float y,const char* value,int columns,float scale,Color color) {
    char line[80];columns=std::clamp(columns,1,78);
    while(*value) {
        const int available=std::min(columns,static_cast<int>(std::strlen(value)));int count=available;
        if(value[available]!=0 && value[available]!=' ') {while(count>0 && value[count]!=' ') --count;if(count==0) count=available;}
        std::memcpy(line,value,static_cast<std::size_t>(count));line[count]=0;text(x,y,line,scale,color);
        value+=count;while(*value==' ') ++value;y+=10*scale;
    }
}
Vec2 Presentation::project(Vec2 p,float z) const noexcept {return {camera_.x+(p.x-p.y)*18*zoom_,camera_.y+(p.x+p.y)*9*zoom_-z*24*zoom_};}
Vec2 Presentation::unproject(Vec2 p) const noexcept {
    const float x=(p.x-camera_.x)/(18*zoom_),y=(p.y-camera_.y)/(9*zoom_);return {(x+y)*0.5F,(y-x)*0.5F};
}
bool Presentation::visible(Vec2 p,float margin) const noexcept {return p.x>-margin && p.x<1240+margin && p.y>170-margin && p.y<830+margin;}
void Presentation::diamond(Vec2 p,float w,float h,Color c,float z) {
    geometry_.quad(project({p.x-w,p.y-h},z),project({p.x+w,p.y-h},z),project({p.x+w,p.y+h},z),project({p.x-w,p.y+h},z),c);
}
void Presentation::box(Vec2 p,float w,float h,float z,Color color) {
    const Vec2 a=project({p.x-w,p.y-h},z),b=project({p.x+w,p.y-h},z),c=project({p.x+w,p.y+h},z),d=project({p.x-w,p.y+h},z);
    geometry_.quad(d,c,project({p.x+w,p.y+h}),project({p.x-w,p.y+h}),shade(color,0.44F));
    geometry_.quad(b,c,project({p.x+w,p.y+h}),project({p.x+w,p.y-h}),shade(color,0.67F));
    geometry_.quad(a,b,c,d,color);
}
void Presentation::enqueue(DrawItem item) noexcept {
    if(itemCount_==items_.size()) return;
    const int depth=std::clamp(static_cast<int>((item.position.x+item.position.y+96)*2),0,383);
    item.next=depthHeads_[static_cast<std::size_t>(depth)];depthHeads_[static_cast<std::size_t>(depth)]=static_cast<int>(itemCount_);
    items_[itemCount_++]=item;
}
void Presentation::world(const Simulation& sim,float alpha) {
    geometry_.clear();itemCount_=0;depthHeads_.fill(-1);
    diamond({},48,48,{0.055F,0.073F,0.078F});
    for(int y=-42;y<=42;y+=2) for(int x=-42;x<=42;x+=2) {
        const Vec2 p{static_cast<float>(x),static_cast<float>(y)};if(!visible(project(p),55)) continue;
        const bool inside=std::abs(x)<10 && std::abs(y)<10;
        const float n=static_cast<float>((x*x+7*y*y+31)%17)*0.0009F;
        diamond(p,0.98F,0.98F,inside ? Color{0.090F+n,0.128F+n,0.132F+n} : Color{0.061F+n,0.079F+n,0.083F+n});
    }
    for(int k=-44;k<=44;k+=4) {
        const float f=static_cast<float>(k);const Color grid{0.12F,0.16F,0.17F,0.45F};
        geometry_.line(project({f,-48}),project({f,48}),0.65F,grid);
        geometry_.line(project({-48,f}),project({48,f}),0.65F,grid);
    }
    const float pulse=reduceMotion_ ? 0 : std::sin(elapsed_*0.7F)*0.12F;
    for(const auto& dot:distant_) {
        const Vec2 p=project(dot.position);if(!visible(p,3)) continue;
        const float size=std::max(1.0F,dot.size*18*zoom_);
        geometry_.rect(p.x,p.y,size,size*1.4F,shade({0.47F+pulse,0.14F,0.22F},dot.shade));
    }
    // Internal routes and the reactor landing pad.
    diamond({},8.8F,0.38F,{0.14F,0.20F,0.21F});diamond({},0.38F,8.8F,{0.14F,0.20F,0.21F});
    diamond({},2.35F,2.35F,{0.16F,0.26F,0.27F});
    for(int i=0;i<SocketCount;++i) {
        const Vec2 p=socketPosition(i);const auto& t=sim.towers()[static_cast<std::size_t>(i)];
        if(t.kind==TowerKind::Empty) {
            const Color c=i==selectedSocket_ ? Teal : Color{0.23F,0.33F,0.34F};
            diamond(p,0.48F,0.48F,c);diamond(p,0.35F,0.35F,Background);
            const Vec2 q=project(p);geometry_.line({q.x-4,q.y},{q.x+4,q.y},1,c);geometry_.line({q.x,q.y-3},{q.x,q.y+3},1,c);
        } else {
            const Color c=t.kind==TowerKind::Repeater ? Teal : t.kind==TowerKind::Mortar ? Gold : Color{0.40F,0.70F,1};
            enqueue({p,0.46F,0.46F,0.90F+0.13F*static_cast<float>(t.level),t.flash>0 ? mix(c,White,t.flash*0.7F) : c,10+i,-1});
        }
        if(i==selectedSocket_) {
            const Vec2 a=project({p.x-0.8F,p.y-0.8F}),b=project({p.x+0.8F,p.y-0.8F}),c=project({p.x+0.8F,p.y+0.8F}),d=project({p.x-0.8F,p.y+0.8F});
            geometry_.line(a,b,2,Teal);geometry_.line(b,c,2,Teal);geometry_.line(c,d,2,Teal);geometry_.line(d,a,2,Teal);
        }
    }
    for(int i=0;i<WallCount;++i) {
        const auto& w=sim.walls()[static_cast<std::size_t>(i)];const Vec2 p=wallPosition(i);
        const float hp=w.health/w.maximum;
        if(hp<=0) {diamond(p,0.7F,0.7F,{0.26F,0.12F,0.12F});continue;}
        Color c=mix({0.76F,0.27F,0.29F},{0.47F,0.60F,0.60F},hp);
        if(w.flash>0 && hp<0.75F) c=mix(c,Red,0.35F);
        if(i==selectedWall_) c=Gold;
        enqueue({p,i/12%2==0 ? 0.80F : 0.35F,i/12%2==0 ? 0.35F : 0.80F,1.3F,c,1,-1});
    }
    // The bunker, archives, foundry and additional reactor capacitors.
    enqueue({{},1.4F,1.4F,2.0F,{0.34F,0.46F,0.48F},2,-1});
    enqueue({{-4,-2},1.25F,0.9F,1.2F,{0.23F,0.36F,0.42F},3,-1});
    enqueue({{3,-3.2F},1,1.4F,0.95F,{0.42F,0.38F,0.27F},3,-1});
    enqueue({{-3.8F,4.5F},1.4F,0.9F,0.8F,{0.28F,0.33F,0.41F},3,-1});
    enqueue({{3.5F,4.5F},1.35F,0.9F,0.8F,{0.28F,0.33F,0.41F},3,-1});
    for(int i=0;i<sim.generators();++i) enqueue({{-5+static_cast<float>(i)*1.8F,-5.6F},0.45F,0.45F,1.3F,Teal,3,-1});
    const auto& pool=sim.enemies();
    for(std::size_t i=0;i<pool.count;++i) {
        Vec2 p{pool.previousX[i]+(pool.x[i]-pool.previousX[i])*alpha,pool.previousY[i]+(pool.y[i]-pool.previousY[i])*alpha};
        if(!visible(project(p),90)) continue;
        const auto& spec=enemyStats(pool.kind[i]);Color c=enemyColor(pool.kind[i]);
        if(pool.slowUntil[i]>sim.time()) c={0.44F,0.76F,1};
        const int type=pool.kind[i]>=EnemyKind::Archon ? 4 : 0;
        enqueue({p,spec.size,spec.size,spec.size*2.1F,c,type,-1});
        if(type==4) {
            const Vec2 at=project(p,spec.size*2.1F+0.5F);char label[40];
            std::snprintf(label,sizeof(label),"%s",enemyName(pool.kind[i]));
            text(at.x-static_cast<float>(std::strlen(label))*4.5F,at.y-12,label,1.5F,Gold);
            const float maxHealth=spec.health*(1+static_cast<float>(sim.wave()-1)*0.095F);
            meter(at.x-42,at.y+3,84,4,pool.health[i]/maxHealth,Red);
        }
    }
    for(std::size_t i=0;i<sim.squads().size();++i) {
        const auto& s=sim.squads()[i];const Vec2 p{s.previous.x+(s.position.x-s.previous.x)*alpha,s.previous.y+(s.position.y-s.previous.y)*alpha};
        if(selection_&(1U<<i)) {
            diamond(p,1.2F,0.9F,{0.14F,0.35F,0.31F});diamond(s.destination,0.35F,0.35F,{0.20F,0.50F,0.42F});
            const Vec2 from=project(p),to=project(s.destination);if(dist(from,to)>20) geometry_.line(from,to,1,{0.2F,0.6F,0.5F,0.4F});
        }
        for(int member=0;member<6;++member) {
            const Vec2 q{p.x+static_cast<float>(member%3-1)*0.5F,p.y+static_cast<float>(member/3)*0.5F};
            enqueue({q,0.14F,0.14F,0.55F,selection_&(1U<<i) ? White : Teal,0,-1});
        }
        const Vec2 at=project(p,1.1F);char label[24];std::snprintf(label,sizeof(label),"%zu",i+1);text(at.x-3,at.y,label,1.5F,Teal);
    }
    for(int head:depthHeads_) for(int i=head;i!=-1;i=items_[static_cast<std::size_t>(i)].next) {
        const auto& item=items_[static_cast<std::size_t>(i)];box(item.position,item.width,item.height,item.elevation,item.color);
        if(item.type==2) {box(item.position,0.55F,0.55F,2.4F,Teal);const Vec2 top=project(item.position,2.6F);text(top.x-24,top.y-6,"ARCA 09",1,White);}
        if(item.type>=10) {
            const int socket=item.type-10;const int side=socket/6;
            const Vec2 p=item.position;const Vec2 outward{p.x+(side==1 ? 0.65F : side==3 ? -0.65F : 0),p.y+(side==0 ? -0.65F : side==2 ? 0.65F : 0)};
            geometry_.line(project(p,item.elevation+0.12F),project(outward,item.elevation+0.12F),4*zoom_,White);
            const Vec2 top=project(p,item.elevation+0.3F);
            for(int level=0;level<sim.towers()[static_cast<std::size_t>(socket)].level;++level) geometry_.rect(top.x-5+static_cast<float>(level)*4,top.y-3,2,2,White);
        }
        if(item.type==4) {box(item.position,item.width*0.45F,item.height*0.45F,item.elevation+0.5F,White);}
    }
    for(const auto& fx:sim.effects()) if(fx.remaining>0) {
        const float life=fx.remaining/fx.lifetime;
        Color c=fx.kind==TowerKind::Mortar ? Gold : fx.kind==TowerKind::Cryo ? Color{0.4F,0.7F,1} : Teal;c.a=life;
        const Vec2 from=project(fx.from,1.2F),to=project(fx.to,0.3F);
        if(fx.kind==TowerKind::Mortar) {
            Vec2 middle{(from.x+to.x)*0.5F,(from.y+to.y)*0.5F-30*zoom_};
            geometry_.line(from,middle,1.4F,c);geometry_.line(middle,to,1.4F,c);
        } else geometry_.line(from,to,1.2F,c);
        if(fx.radius>0) {
            const float r=fx.radius*(1-life)*0.9F;
            const Vec2 a=project({fx.to.x-r,fx.to.y-r}),b=project({fx.to.x+r,fx.to.y-r}),cc=project({fx.to.x+r,fx.to.y+r}),d=project({fx.to.x-r,fx.to.y+r});
            geometry_.line(a,b,1,c);geometry_.line(b,cc,1,c);geometry_.line(cc,d,1,c);geometry_.line(d,a,1,c);
        }
    }
    require(SDL_SetRenderClipRect(renderer_.get(),&WorldClip),"world clip");geometry_.flush(renderer_.get());
    // Text attached to world objects uses the same clip as their geometry.
    letters_.flush(renderer_.get(),font_.get());letters_.clear();
    require(SDL_SetRenderClipRect(renderer_.get(),nullptr),"reset clip");
}
void Presentation::meter(float x,float y,float width,float height,float value,Color color) {
    geometry_.rect(x,y,width,height,Border);geometry_.rect(x,y,width*std::clamp(value,0.0F,1.0F),height,color);
}
void Presentation::button(float x,float y,float width,float height,const char* label,Action action,bool enabled,bool accent) {
    const SDL_FRect bounds{x,y,width,height};const bool hover=contains(bounds,mouse_);
    Color color=accent ? (enabled ? Teal : shade(Teal,0.4F)) : Raised;
    if(hover) color=mix(color,White,0.10F);
    geometry_.rect(x,y,width,height,color);
    geometry_.rect(x,y,width,1,accent ? shade(Teal,0.8F) : Border);
    const float scale=1.5F;const float length=static_cast<float>(std::strlen(label))*6*scale;
    text(x+(width-length)*0.5F,y+(height-10.5F)*0.5F,label,scale,accent ? Background : enabled ? White : Muted);
    if(buttonCount_<buttons_.size()) buttons_[buttonCount_++]={bounds,action};
}
void Presentation::interface(const Simulation& sim,FrameMetrics metrics) {
    geometry_.clear();buttonCount_=0;
    char value[200];
    geometry_.rect(0,0,1600,102,Background);geometry_.rect(24,100,1552,1,Border);
    geometry_.rect(24,27,5,42,Teal);text(44,25,"1 BILLION",4,White);text(46,66,"EL ULTIMO PERIMETRO",1.5F,Muted);
    text(390,25,"CONTACTOS EN LA RED / GLOBAL",1.25F,Muted);
    const auto total=sim.reserve()+sim.enemies().count;
    if(total>=GlobalHorde) std::snprintf(value,sizeof(value),"1.000.000.000");
    else std::snprintf(value,sizeof(value),"%llu.%03llu.%03llu",static_cast<unsigned long long>(total/1000000),static_cast<unsigned long long>(total/1000%1000),static_cast<unsigned long long>(total%1000));
    text(390,48,value,3,Red);
    text(790,25,"NUCLEO DEL ARCA",1.25F,Muted);meter(790,51,190,8,sim.coreHealth()/1000,sim.coreHealth()<400 ? Red : Teal);
    std::snprintf(value,sizeof(value),"INTEGRIDAD %03d%%",static_cast<int>(sim.coreHealth()/10));text(790,69,value,1,White);
    text(1055,25,"MATERIA",1.25F,Muted);std::snprintf(value,sizeof(value),"%05d",sim.matter());text(1055,47,value,3,Gold);
    text(1260,25,"POTENCIA",1.25F,Muted);std::snprintf(value,sizeof(value),"%02d / %02d",sim.powerUsed(),sim.powerAvailable());text(1260,49,value,2,Teal);
    text(1445,27,muted_ ? "M SILENCIO" : "M AUDIO",1,Muted);text(1445,48,"F1 AYUDA",1,White);text(1445,69,"F3 METRICAS",1,Muted);

    geometry_.rect(24,118,1200,44,Panel);
    const bool planning=sim.phase()==Phase::Planning;
    geometry_.rect(24,118,3,44,planning ? Teal : Red);
    text(42,132,paused() ? "PAUSA TACTICA" : planning ? "PREPARACION" : sim.isStress() ? "PRUEBA DE CARGA" : "ASALTO EN CURSO",1.5F,paused() ? Gold : planning ? Teal : Red);
    std::snprintf(value,sizeof(value),"EN EL FRENTE  %05zu",sim.enemies().count);text(288,132,value,1.5F,White);
    if(planning) std::snprintf(value,sizeof(value),"CONTACTO EN %02d S",static_cast<int>(std::ceil(sim.planningTime())));
    else std::snprintf(value,sizeof(value),"RESERVA DEL PULSO  %05d",sim.pending());
    text(590,132,value,1.5F,Muted);
    button(948,124,258,32,planning ? "[ENTER] INICIAR ASALTO" : paused() ? "[ESPACIO] CONTINUAR" : "[ESPACIO] PAUSA",planning ? Action::Start : Action::Pause,true,planning);

    // Right command console.
    geometry_.rect(1240,118,336,714,Panel);geometry_.rect(1240,118,336,3,Teal);
    std::snprintf(value,sizeof(value),"PULSO %02d / %02d",std::max(1,sim.wave()),sim.finalWave());text(1260,142,value,1.5F,Teal);
    text(1260,174,waveInfo(std::max(1,sim.wave())).name,2.5F,White);
    paragraph(1260,208,waveInfo(std::max(1,sim.wave())).briefing,46,1,Muted);
    std::snprintf(value,sizeof(value),"FRENTE PRINCIPAL: %s",Sides[sim.focusSide()]);text(1260,238,value,1.25F,Gold);
    const float progress=sim.wave()==0 ? 0 : static_cast<float>(sim.wave()-(sim.phase()==Phase::Planning ? 0 : 1))/static_cast<float>(sim.finalWave());
    meter(1260,264,296,5,sim.phase()==Phase::Victory ? 1 : progress,Teal);
    text(1260,285,"SINCRONIZACION DEL SALTO",1,Muted);
    geometry_.rect(1260,313,296,1,Border);text(1260,330,"LECTURA DEL PERIMETRO",1.25F,White);
    for(int side=0;side<4;++side) {
        const float y=360+static_cast<float>(side)*25;const float hp=sim.sideIntegrity(side);
        text(1260,y,Sides[side],1.25F,side==sim.focusSide() ? Gold : Muted);
        meter(1328,y+2,176,6,hp,hp<0.4F ? Red : Teal);
        std::snprintf(value,sizeof(value),"%03d",static_cast<int>(hp*100));text(1518,y,value,1,White);
    }
    geometry_.rect(1260,468,296,144,Background);
    const float radarX=1408,radarY=539,radarScale=1.25F;
    for(std::size_t i=0;i<sim.enemies().count;i+=std::max<std::size_t>(1,sim.enemies().count/1700))
        geometry_.rect(radarX+sim.enemies().x[i]*radarScale,radarY+sim.enemies().y[i]*radarScale,1.5F,1.5F,Red);
    geometry_.rect(radarX-13,radarY-13,26,26,Border);geometry_.rect(radarX-11,radarY-11,22,22,Panel);geometry_.rect(radarX-2,radarY-2,4,4,Teal);
    text(1270,480,"RADAR 09",1,Muted);std::snprintf(value,sizeof(value),"BRECHAS %02d",sim.breaches());text(1270,594,value,1,sim.breaches()>0 ? Red : Muted);
    text(1260,633,"EVOLUCION DEL ARCA",1.25F,White);
    const char* technology=sim.tech()==1 ? "I / BALISTICA" : sim.tech()==2 ? "II / INDUCCION" : "III / PLASMA";
    text(1260,658,technology,1.5F,Teal);
    if(sim.tech()<3) std::snprintf(value,sizeof(value),"[I] INVESTIGAR  %d",sim.researchCost());else std::snprintf(value,sizeof(value),"TECNOLOGIA MAXIMA");
    button(1260,690,296,40,value,Action::Research,sim.tech()<3 && sim.wave()>=(sim.tech()==1 ? 2 : 5) && sim.matter()>=sim.researchCost());
    std::snprintf(value,sizeof(value),"[G] +4 POTENCIA  %d",sim.generatorCost());button(1260,740,296,40,value,Action::Generator,sim.generators()<6 && sim.matter()>=sim.generatorCost());
    text(1260,800,sim.tech()==1 ? "CRIO: REQUIERE TECNOLOGIA II" : "INVESTIGAR REFUERZA LAS MURALLAS",1,Muted);

    // World footer and three movable squads.
    geometry_.rect(24,796,1200,36,Background);
    geometry_.rect(38,809,7,7,Red);text(54,809,"CORO",1,Muted);geometry_.rect(110,809,7,7,Teal);text(126,809,"DEFENSAS",1,Muted);
    geometry_.rect(206,809,7,7,Gold);text(222,809,"PESADOS",1,Muted);
    const float pressure=sim.pressure();std::snprintf(value,sizeof(value),"PRESION %03d%%",static_cast<int>(pressure*100));text(370,807,value,1.5F,pressure>0.6F ? Red : Teal);
    meter(570,810,230,4,pressure,Red);text(855,809,"WASD CAMARA / RUEDA ZOOM",1,Muted);

    geometry_.rect(24,850,1200,120,Panel);text(42,864,"ESCUADRAS",1,Muted);
    for(int i=0;i<3;++i) {
        const float x=42+static_cast<float>(i)*113;
        std::snprintf(value,sizeof(value),"[%d] %s",i+1,i==0 ? "ALFA" : i==1 ? "BETA" : "GAMMA");
        button(x,886,103,41,value,static_cast<Action>(static_cast<int>(Action::Squad1)+i),true,(selection_&(1U<<i))!=0);
    }
    text(42,946,"CLIC DERECHO: MOVER / ARRASTRA: SELECCIONAR",1,Muted);
    geometry_.rect(391,864,1,91,Border);
    if(selectedSocket_>=0) {
        const auto& tower=sim.towers()[static_cast<std::size_t>(selectedSocket_)];
        std::snprintf(value,sizeof(value),"ZOCALO %02d / %s",selectedSocket_+1,towerName(tower.kind));text(412,864,value,1.25F,White);
        if(tower.kind==TowerKind::Empty) {
            button(412,889,190,38,"[Q] REPETIDORA 150",Action::Repeater,sim.matter()>=150 && sim.powerUsed()<sim.powerAvailable());
            button(612,889,190,38,"[E] MORTERO 260",Action::Mortar,sim.matter()>=260 && sim.powerUsed()+2<=sim.powerAvailable());
            button(812,889,190,38,"[T] CRIO 220",Action::Cryo,sim.tech()>=2 && sim.matter()>=220 && sim.powerUsed()+2<=sim.powerAvailable());
            text(412,946,"MORTERO: AREA / CRIO: RALENTIZA / TODAS DISPARAN SOLAS",1,Muted);
        } else {
            std::snprintf(value,sizeof(value),"[U] MEJORA %d",sim.upgradeCost(selectedSocket_));
            button(412,889,240,38,value,Action::Upgrade,tower.level<3 && sim.matter()>=sim.upgradeCost(selectedSocket_));
            std::snprintf(value,sizeof(value),"NIVEL %d / 3",tower.level);text(677,904,value,1.5F,Teal);
            text(412,946,"CLIC EN OTRO ZOCALO PARA CONSTRUIR O EN UNA MURALLA PARA REPARAR",1,Muted);
        }
    } else if(selectedWall_>=0) {
        const auto& wall=sim.walls()[static_cast<std::size_t>(selectedWall_)];
        std::snprintf(value,sizeof(value),"MURALLA %02d / INTEGRIDAD %d / %d",selectedWall_+1,static_cast<int>(wall.health),static_cast<int>(wall.maximum));text(412,864,value,1.25F,White);
        button(412,889,320,38,"[R] REPARAR +350 / 90",Action::Repair,sim.matter()>=90 && wall.health<wall.maximum);
        text(412,946,"UNA BRECHA DA ACCESO AL NUCLEO. RECONSTRUYE A TIEMPO.",1,Muted);
    } else {
        text(412,867,"ORDENES DE CAMPO",1.5F,White);text(412,899,"MUEVE LAS ESCUADRAS HACIA EL FRENTE MAS DEBIL.",1.5F,Teal);
        text(412,940,"CLIC EN UN ZOCALO: CONSTRUIR / CLIC EN MURO: REPARAR",1,Muted);
    }
    geometry_.rect(1240,850,336,120,Panel);
    if(sim.overdriveCooldown()>0) std::snprintf(value,sizeof(value),"RECARGA  %02d S",static_cast<int>(std::ceil(sim.overdriveCooldown())));else std::snprintf(value,sizeof(value),"[F] SOBRECARGA");
    button(1260,868,296,55,value,Action::Overdrive,sim.phase()==Phase::Assault && sim.overdriveCooldown()<=0,sim.overdriveTime()>0);
    text(1260,944,"8 S DOBLE CADENCIA / RECARGA 45 S",1,sim.overdriveTime()>0 ? Teal : Muted);

    if(messageTime_>0) text(24,983,message_.data(),1.25F,Teal);
    else text(24,983,"ARCA 09 / NO TENEMOS QUE MATARLOS A TODOS. SOLO COMPRAR TIEMPO.",1.25F,Muted);
    text(1360,983,"PROTOTIPO 0.1 / N REINICIA",1,Muted);
    if(planning) {
        geometry_.rect(379,707,490,59,Panel);geometry_.rect(379,707,3,59,Teal);
        text(402,719,sim.wave()==0 ? "NO TENEMOS QUE GANAR ESTA GUERRA." : "PULSO CONTENIDO. REORGANIZA EL FRENTE.",1.5F,White);
        text(402,743,sim.wave()==0 ? "TENEMOS QUE SOBREVIVIR AL SIGUIENTE MINUTO." : "RECICLA. INVESTIGA. PREPARA EL SIGUIENTE ASALTO.",1.25F,Muted);
    }
    if(sim.bossAlive()) {geometry_.rect(380,181,490,33,Panel);text(400,191,"! ENTIDAD DE MANDO EN EL PERIMETRO",1.5F,Gold);}
    if(telemetry_) {
        geometry_.rect(34,183,328,125,Background);
        std::snprintf(value,sizeof(value),"FRAME %.2F MS / SIM %.2F MS",metrics.frameMs,metrics.simulationMs);text(46,195,value,1.25F,White);
        std::snprintf(value,sizeof(value),"TICKS %d / CAP %zu",metrics.steps,sim.enemies().capacity());text(46,218,value,1.25F,Teal);
        std::snprintf(value,sizeof(value),"CORE %.2F MIB / DRAW %.2F MIB",static_cast<double>(sim.ownedBytes())/1048576.0,static_cast<double>(ownedBytes())/1048576.0);text(46,241,value,1.25F,Muted);
        std::snprintf(value,sizeof(value),"BAJAS %llu / SIM 30 HZ",static_cast<unsigned long long>(sim.stats().killed));text(46,264,value,1.25F,Muted);
        text(46,287,"F3 OCULTA / H REDUCE MOVIMIENTO",1,Muted);
    }
    if(dragging_ && dist(mouse_,dragStart_)>6) {
        const Color c{0.24F,0.90F,0.74F,0.8F};const Vec2 a=dragStart_,b{mouse_.x,dragStart_.y},d{dragStart_.x,mouse_.y};
        geometry_.line(a,b,1,c);geometry_.line(b,mouse_,1,c);geometry_.line(mouse_,d,1,c);geometry_.line(d,a,1,c);
    }
    if(sim.phase()==Phase::Victory || sim.phase()==Phase::Defeat) {
        geometry_.rect(20,170,1204,660,{Background.r,Background.g,Background.b,0.88F});
        const bool won=sim.phase()==Phase::Victory;
        geometry_.rect(300,335,644,238,Panel);geometry_.rect(300,335,644,3,won ? Teal : Red);
        text(338,368,won ? "EL ARCA SALTO." : "EL PERIMETRO CAYO.",3,won ? Teal : Red);
        text(338,423,won ? "EL CORO SIGUE AHI. NOSOTROS YA NO." : "LOS MUROS SOLO ERAN TIEMPO PRESTADO.",1.5F,White);
        std::snprintf(value,sizeof(value),"PULSOS %d / BAJAS %llu",sim.wave(),static_cast<unsigned long long>(sim.stats().killed));text(338,462,value,1.5F,Muted);
        button(338,511,568,40,"[N] OTRO INTENTO",Action::Reset,true,true);
    }
    if(help_) helpOverlay();
    geometry_.flush(renderer_.get());letters_.flush(renderer_.get(),font_.get());letters_.clear();
}
void Presentation::helpOverlay() {
    geometry_.rect(0,102,1600,898,{Background.r,Background.g,Background.b,0.94F});
    // Modal text must replace the underlying text, not blend over it.
    letters_.clear();
    geometry_.rect(320,173,960,650,Panel);geometry_.rect(320,173,960,3,Teal);
    text(360,212,"MANUAL DEL ULTIMO PERIMETRO",3,White);
    text(360,264,"RESISTE 8 PULSOS MIENTRAS EL REACTOR PREPARA EL SALTO.",1.5F,Teal);
    text(360,299,"LA HORDA ATACA CUATRO MUROS. UNA BRECHA LLEVA AL NUCLEO.",1.5F,Muted);
    constexpr const char* rows[]{
        "CLIC / ARRASTRA      SELECCIONA ZOCALOS, MUROS O ESCUADRAS",
        "1 / 2 / 3           SELECCIONA UNA ESCUADRA",
        "CLIC DERECHO        MUEVE LAS ESCUADRAS DENTRO DEL REFUGIO",
        "Q / E / T           CONSTRUYE REPETIDORA / MORTERO / CRIO",
        "U / R               MEJORA TORRETA / REPARA MURALLA",
        "I / G               INVESTIGA / AMPLIA POTENCIA",
        "F                   SOBRECARGA: 8 SEGUNDOS DE DOBLE CADENCIA",
        "ENTER / ESPACIO     INICIA PULSO / PAUSA TACTICA CON ORDENES",
        "WASD / RUEDA        CAMARA / ZOOM EN EL CURSOR",
        "M / H / F3          SILENCIO / MENOS MOVIMIENTO / METRICAS",
        "F11 / N / ESC       PANTALLA COMPLETA / REINICIA / SALIR"};
    for(std::size_t i=0;i<std::size(rows);++i) text(360,345+static_cast<float>(i)*29,rows[i],1.5F,i%2==0 ? White : Muted);
    text(360,688,"CRIO REQUIERE TECNOLOGIA II. INVESTIGA DESDE EL PULSO 2 / 5.",1.25F,Teal);
    text(360,720,"LA MASA DEL HORIZONTE ES UNA REPRESENTACION DE LA RESERVA GLOBAL.",1.25F,Muted);
    text(360,747,"LAS UNIDADES DEL FRENTE SE SIMULAN INDIVIDUALMENTE.",1.25F,Muted);
    button(360,779,880,32,"[F1] VOLVER AL PERIMETRO",Action::Help,true,true);
}
void Presentation::notify(Result result) noexcept {std::snprintf(message_.data(),message_.size(),"%s",resultText(result));messageTime_=5;}
void Presentation::invoke(Action action,Simulation& sim) {
    switch(action) {
        case Action::Start:notify(sim.startWave());break;case Action::Pause:paused_=!paused_;break;
        case Action::Repeater:notify(sim.build(selectedSocket_,TowerKind::Repeater));break;
        case Action::Mortar:notify(sim.build(selectedSocket_,TowerKind::Mortar));break;
        case Action::Cryo:notify(sim.build(selectedSocket_,TowerKind::Cryo));break;
        case Action::Upgrade:notify(sim.upgrade(selectedSocket_));break;case Action::Repair:notify(sim.repair(selectedWall_));break;
        case Action::Research:notify(sim.research());break;case Action::Generator:notify(sim.generator());break;
        case Action::Overdrive:notify(sim.overdrive());break;
        case Action::Squad1:selection_=1;selectedSocket_=selectedWall_=-1;break;
        case Action::Squad2:selection_=2;selectedSocket_=selectedWall_=-1;break;
        case Action::Squad3:selection_=4;selectedSocket_=selectedWall_=-1;break;
        case Action::Reset:sim.reset();paused_=false;selectedSocket_=0;selectedWall_=-1;selection_=1;notify(Result::Ok);break;
        case Action::Help:help_=!help_;break;
    }
}
void Presentation::choose(Vec2 mouse,const Simulation& sim) {
    float best=24;int socket=-1;
    for(int i=0;i<SocketCount;++i) {
        const float d=dist(mouse,project(socketPosition(i),sim.towers()[static_cast<std::size_t>(i)].kind==TowerKind::Empty ? 0 : 0.8F));
        if(d<best) {socket=i;best=d;}
    }
    if(socket>=0) {selectedSocket_=socket;selectedWall_=-1;return;}
    best=27;int wall=-1;
    for(int i=0;i<WallCount;++i) {const float d=dist(mouse,project(wallPosition(i),sim.walls()[static_cast<std::size_t>(i)].health>0 ? 1 : 0));if(d<best) {wall=i;best=d;}}
    if(wall>=0) {selectedWall_=wall;selectedSocket_=-1;return;}
    for(std::size_t i=0;i<sim.squads().size();++i) if(dist(mouse,project(sim.squads()[i].position,0.5F))<26) {
        if(SDL_GetModState()&SDL_KMOD_SHIFT) selection_|=static_cast<std::uint8_t>(1U<<i);else selection_=static_cast<std::uint8_t>(1U<<i);
        selectedSocket_=selectedWall_=-1;return;
    }
    selectedSocket_=selectedWall_=-1;
}
bool Presentation::handle(SDL_Event event,Simulation& sim) {
    if(event.type==SDL_EVENT_QUIT) return false;
    SDL_ConvertEventToRenderCoordinates(renderer_.get(),&event);
    if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST) {paused_=true;dragging_=panning_=false;}
    if(event.type==SDL_EVENT_MOUSE_MOTION) {
        mouse_={event.motion.x,event.motion.y};
        if(panning_) {camera_.x+=mouse_.x-panStart_.x;camera_.y+=mouse_.y-panStart_.y;panStart_=mouse_;}
    }
    if(event.type==SDL_EVENT_MOUSE_WHEEL && !help_) {
        const Vec2 anchor=unproject(mouse_);zoom_=std::clamp(zoom_*(event.wheel.y>0 ? 1.12F : 0.89F),0.35F,2.1F);
        const Vec2 moved=project(anchor);camera_.x+=mouse_.x-moved.x;camera_.y+=mouse_.y-moved.y;
    }
    if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN) {
        mouse_={event.button.x,event.button.y};
        if(event.button.button==SDL_BUTTON_LEFT) {
            if(help_) {invoke(Action::Help,sim);return true;}
            for(std::size_t i=0;i<buttonCount_;++i) if(contains(buttons_[i].bounds,mouse_)) {invoke(buttons_[i].action,sim);return true;}
            if(contains({20,170,1204,660},mouse_)) {dragging_=true;dragStart_=mouse_;}
        }
        if(event.button.button==SDL_BUTTON_MIDDLE && !help_) {panning_=true;panStart_=mouse_;}
        if(event.button.button==SDL_BUTTON_RIGHT && !help_ && contains({20,170,1204,660},mouse_)) {sim.moveSquads(selection_,unproject(mouse_));notify(Result::Ok);}
    }
    if(event.type==SDL_EVENT_MOUSE_BUTTON_UP) {
        if(event.button.button==SDL_BUTTON_MIDDLE) panning_=false;
        if(event.button.button==SDL_BUTTON_LEFT && dragging_) {
            mouse_={event.button.x,event.button.y};dragging_=false;
            if(dist(mouse_,dragStart_)<6) choose(mouse_,sim);
            else {
                const SDL_FRect area{std::min(mouse_.x,dragStart_.x),std::min(mouse_.y,dragStart_.y),std::abs(mouse_.x-dragStart_.x),std::abs(mouse_.y-dragStart_.y)};
                selection_=0;for(std::size_t i=0;i<sim.squads().size();++i) if(contains(area,project(sim.squads()[i].position))) selection_|=static_cast<std::uint8_t>(1U<<i);
                selectedSocket_=selectedWall_=-1;
            }
        }
    }
    if(event.type==SDL_EVENT_KEY_DOWN && !event.key.repeat) {
        if(event.key.key==SDLK_F1) {help_=!help_;return true;}
        if(event.key.key==SDLK_ESCAPE) {if(help_) {help_=false;return true;}return false;}
        if(help_) return true;
        switch(event.key.key) {
            case SDLK_RETURN:invoke(Action::Start,sim);break;case SDLK_SPACE:invoke(Action::Pause,sim);break;
            case SDLK_Q:invoke(Action::Repeater,sim);break;case SDLK_E:invoke(Action::Mortar,sim);break;
            case SDLK_T:invoke(Action::Cryo,sim);break;case SDLK_U:invoke(Action::Upgrade,sim);break;
            case SDLK_R:invoke(Action::Repair,sim);break;case SDLK_I:invoke(Action::Research,sim);break;
            case SDLK_G:invoke(Action::Generator,sim);break;case SDLK_F:invoke(Action::Overdrive,sim);break;
            case SDLK_1:invoke(Action::Squad1,sim);break;case SDLK_2:invoke(Action::Squad2,sim);break;case SDLK_3:invoke(Action::Squad3,sim);break;
            case SDLK_N:invoke(Action::Reset,sim);break;case SDLK_M:muted_=!muted_;break;
            case SDLK_H:reduceMotion_=!reduceMotion_;break;case SDLK_F3:telemetry_=!telemetry_;break;
            case SDLK_F11:fullscreen_=!fullscreen_;SDL_SetWindowFullscreen(window_.get(),fullscreen_);break;
            default:break;
        }
    }
    return true;
}
void Presentation::audioUpdate(const Simulation& sim) {
    if(!audio_) return;
    // Queue at most ~85 ms of mono PCM; never grow an unbounded backlog.
    const int queued=SDL_GetAudioStreamQueued(audio_.get());if(queued<0 || queued>=4096*static_cast<int>(sizeof(float))) return;
    const double pressure=static_cast<double>(sim.pressure());
    const bool quiet=muted_ || paused() || sim.phase()==Phase::Victory || sim.phase()==Phase::Defeat;
    const double amplitude=quiet ? 0 : 0.035+pressure*0.05;
    const bool breached=sim.breaches()>0;
    for(auto& sample:audioSamples_) {
        audioTime_+=1.0/48000;noise_=noise_*1664525U+1013904223U;
        const double noise=static_cast<double>(noise_>>8)/8388608.0-1;
        const double beat=std::pow(std::max(0.0,std::sin(audioTime_*(4+pressure*8))),12.0);
        const double hum=std::sin(audioTime_*6.28318530718*47)*0.35+std::sin(audioTime_*6.28318530718*71)*0.18;
        const double warning=breached ? std::sin(audioTime_*6.28318530718*(260+60*std::sin(audioTime_*3)))*0.22 : 0;
        sample=static_cast<float>((hum+noise*beat*0.22+warning)*amplitude);
    }
    if(!SDL_PutAudioStreamData(audio_.get(),audioSamples_.data(),static_cast<int>(sizeof(audioSamples_)))) audio_.reset();
}
void Presentation::update(float realDelta,const Simulation& sim) {
    elapsed_+=reduceMotion_ ? 0 : realDelta;messageTime_=std::max(0.0F,messageTime_-realDelta);
    if(lastWave_!=sim.wave()) {
        lastWave_=sim.wave();if(sim.wave()>0) {std::snprintf(message_.data(),message_.size(),"%s",waveInfo(sim.wave()).briefing);messageTime_=12;}
    }
    if(!help_) {
        const bool* keys=SDL_GetKeyboardState(nullptr);const float speed=470*realDelta;
        if(keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) camera_.y+=speed;
        if(keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) camera_.y-=speed;
        if(keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) camera_.x+=speed;
        if(keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) camera_.x-=speed;
        camera_.x=std::clamp(camera_.x,-600.0F,1850.0F);camera_.y=std::clamp(camera_.y,-500.0F,1450.0F);
    }
    audioUpdate(sim);
}
void Presentation::render(const Simulation& sim,float interpolation,FrameMetrics metrics) {
    require(SDL_SetRenderDrawColorFloat(renderer_.get(),Background.r,Background.g,Background.b,1),"clear color");require(SDL_RenderClear(renderer_.get()),"clear");
    letters_.clear();world(sim,std::clamp(interpolation,0.0F,1.0F));interface(sim,metrics);
}
bool Presentation::capture(const char* path) {
    SurfacePtr pixels(SDL_RenderReadPixels(renderer_.get(),nullptr));return pixels && SDL_SaveBMP(pixels.get(),path);
}
void Presentation::present() {require(SDL_RenderPresent(renderer_.get()),"present");}
std::size_t Presentation::ownedBytes() const noexcept {return sizeof(*this)+geometry_.bytes()+letters_.bytes()+items_.capacity()*sizeof(DrawItem);}
} // namespace siege
