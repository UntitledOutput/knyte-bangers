

// main renderer
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#define NO_FONT_AWESOME

#include "rlImGui.h"	// include the API header
#include "rlImGuiColors.h"
#include "imgui.h"

// utilities
#include "cJSON/cJSON.h"
#include "PerlinNoise.hpp"

// required headers
#include <typeinfo> 
#include <memory>
#include <algorithm>
#include <map>
#include <vector>
#include <string>

// physics
#include "box2d/box2d.h"


#if !defined(VISUALIZE2DBOUNDS)
#define VISUALIZE2DBOUNDS false
#endif // VISUALIZE2DBOUNDS

#if !defined(SHOWCONSOLE)
#define SHOWCONSOLE false
#endif // SHOWCONSOLE

Camera3D Load3DCamera(float fovy, Vector3 position, CameraProjection projection, Vector3 target, Vector3 up);
Camera2D Load2DCamera(Vector2 offset, float rotation, Vector2 target, float zoom);

class Entity;
class Engine;

#define TO_STRING( x ) #x

//****************
#define CLASS_DECLARATION( classname )                                                      \
public:                                                                                     \
    static const std::size_t Type;                                                          \
    virtual bool IsClassType( const std::size_t classType ) const override;                 \

//****************
// CLASS_DEFINITION
// 
// This macro must be included in the class definition to properly initialize 
// variables used in type checking. Take special care to ensure that the 
// proper parentclass is indicated or the run-time type information will be
// incorrect. Only works on single-inheritance RTTI.
//****************
#define CLASS_DEFINITION( parentclass, childclass )                                         \
const std::size_t childclass::Type = std::hash< const char* >()( TO_STRING( childclass ) ); \
bool childclass::IsClassType( const std::size_t classType ) const {                         \
        if ( classType == childclass::Type )                                                \
            return true;                                                                    \
        return parentclass::IsClassType( classType );                                       \
}                                                                                           \

    // Input definitions

typedef enum InputAction {
    LEFT = 0,
    RIGHT,
    UP,
    DOWN,
    SECLEFT,
    SECRIGHT,
    SECUP,
    SECDOWN,
    MOTION,
    HORIZONTAL1,
    HORIZONTAL2,
    VERTICAL1,
    VERTICAL2,
    DPADUP,
    DPADDOWN,
    DPADLEFT,
    DPADRIGHT,
    START,
    SELECT,
    LEFTTRIGGER,
    RIGHTTRIGGER,
    LEFTBUMBER,
    RIGHTBUMPER,
    BUTTONUP,
    BUTTONDOWN,
    BUTTONLEFT,
    BUTTONRIGHT,
};

class InputManager {
public:
    static int GetInput(InputAction action);
    static float GetInputAxis(InputAction action);
    static bool GetMouseDown(MouseButton mb);
    static Vector2 GetMousePosition();
};

class TextureRes {
public:

    TextureRes();

    TextureRes(Texture2D texture);

public:

     Texture2D tex;

    virtual void Update();
};


class aurCamera {
public:
    aurCamera() = default;

    Camera2D cam2D;
    Camera3D cam3D;

    Vector3 targetOffset3D;
    Vector2 targetOffset2D;

    bool useCam2D;

    inline void Attach(Entity* entity, float width, float height) {
        entW = width;
        entH = height;
        attachedEntity = entity;
        printf("%.2f, %.2f\n", entW, entH);
    };

    void Update();

private:
    friend class Scene;

    Entity* attachedEntity;
    float entW;
    float entH;
};

class ModelRes {
public:
    ModelRes() = default;
    Model model;
    const char* path;
};


class ModelData {
public:
    ModelData() = default;
    std::vector<ModelRes*> models;
    int modelCount;
};

class Component {
public:

    static const std::size_t                    Type;
    virtual bool                                IsClassType(const std::size_t classType) const {
        return classType == Type;
    }

public:

    virtual                                ~Component() = default;
    Component(std::string&& initialValue)
        : value(initialValue) {
    }

    Component() = default;

public:
    std::string                             value = "uninitialized";
private:
    friend class ComponentHolder;
    friend class Entity;

    virtual void Update(Entity* Entity);
    virtual void Unload();
public:
    Entity* entity;
};

class ComponentHolder {
private:

    friend class Entity;
    friend class EntityData;

    ComponentHolder() = default;
public:
    std::vector< std::unique_ptr<Component> > components;
    template <class CompType, typename... Args>
    void Attach(Args&&... params);

    void Update(Entity* Entity);

    void Unload();
};

class Engine;
class Scene;

class Entity {
public:

    Vector3 localPosition;
    Vector3 localRotation;
    Vector3 localScale;

    Vector3 globalPosition;
    Vector3 globalRotation;
    Vector3 globalScale;

    ComponentHolder* componentHolder;

    Engine* engine;
    Scene* scene;

    const char* name;

    template< class ComponentType, typename... Args >
    inline ComponentType* AddComponent(Args&&... params) {

        componentHolder->components.emplace_back(std::make_unique< ComponentType >(std::forward< Args >(params)...));

        for (auto&& component : componentHolder->components) {
            if (component->IsClassType(ComponentType::Type)) {
                ComponentType* comp = static_cast<ComponentType*>(component.get());
                comp->entity = this;
                return comp;
            }
        }

        return static_cast<ComponentType*>(nullptr);
    }

    template< class ComponentType >
    inline ComponentType* GetComponent() {
        for (auto&& component : componentHolder->components) {
            if (component->IsClassType(ComponentType::Type))
                return static_cast<ComponentType*>(component.get());
        }

        return static_cast<ComponentType*>(nullptr);
    }

    template< class ComponentType >
    bool RemoveComponent();

    template< class ComponentType >
    std::vector< ComponentType* > GetComponents();

    template< class ComponentType >
    int RemoveComponents();

    virtual void Update();

    const char* tag = "none";

    Entity* parent;

    inline void AddChild(Entity* entity) {
        entity->parent = this;

        Vector3 absPos = entity->localPosition;
        for (Entity* p = entity->parent; p != nullptr; p = p->parent)
            absPos = Vector3Add(absPos, p->localPosition);
        globalPosition = absPos;

        Vector3 absRot = entity->localRotation;
        for (Entity* p = entity->parent; p != nullptr; p = p->parent)
            absRot = Vector3Add(absRot, p->localRotation);
        globalRotation = absRot;

        Vector3 absScl = entity->localScale;
        for (Entity* p = entity->parent; p != nullptr; p = p->parent)
            absScl = Vector3Add(absScl, p->localScale);
        globalScale = absScl;

        children.push_back(entity);
    }

    inline Entity* GetChild(int index) {
        return children.at(index);
    }

    std::vector<Entity*> children;

    void Unload();

private:
    friend class EntityData;

    Entity() = default;

};

class Renderer2D : public Component {
    CLASS_DECLARATION( Renderer2D )
public:
    Renderer2D(std::string&& initialValue)
        : Component(std::move(initialValue)) {
    }

    Renderer2D() = default;

    TextureRes* texture;

    void Update(Entity* Entity);
    void Init(TextureRes* res);
    Rectangle rec;
    Color tint = WHITE;
private:
};

class AnimatedRenderer2D : public Renderer2D {
    CLASS_DECLARATION( AnimatedRenderer2D )
public:
    AnimatedRenderer2D(std::string&& initialValue)
        : Renderer2D(std::move(initialValue)) {
    }

    AnimatedRenderer2D() = default;

    TextureRes* texture;
    Rectangle rec;
    Rectangle srcRect;

    void Update(Entity* Entity);
    void Init(const char* path);
    void Init(TextureRes* tex);
    //Rectangle rec;
private:

};


class Actor2D : public Entity {
public:
    Actor2D() = default;
    Renderer2D* renderer;

    void Init();

    void Update();
};

class EntityData {
public:
    EntityData() = default;
    std::vector<Entity*> entities;
    Entity* createEntity(const char* name);
    inline Entity* find(const char* name) {
        for (Entity* entity : entities) {
            if (strcmp(entity->name, name) == 0) {
                return entity;
                break;
            }
        }
    };
    int entityCount;
    void Update();
    void UI_Update();
    void Unload();
    Engine* engine;
    Scene* scene;
};

class Scene {
public:
    Scene() = default;
    ModelData* modelData;
    EntityData* entityData;
    virtual void scene_init();
    virtual void scene_update(aurCamera* cam);
    virtual void scene_unload();
    static void originalBegin2D(aurCamera* cam, EntityData* entityData);
    static void originalBegin3D(aurCamera* cam, EntityData* entityData);
    static void originalEnd(bool use2D);
    static void originalSetup();
    static void originalUnload();
    bool is2D;
    bool useRenderTexture;
    Engine* engine;
    bool isActive;
    int sceneIndex;
private:
    friend class SceneMgr;
    RenderTexture2D renderTexture;
};

class CameraMgr;

class SceneMgr {
public:
    void Init(Engine* eng);
    void Update(CameraMgr* camMgr);

    //void LoadScene(const char* path);

    template<class SceneType>
    inline SceneType* LoadScene()
    {
        scenes.emplace_back(std::make_unique< SceneType >());

        SceneType* scene = static_cast<SceneType*>(scenes.back().get());

        scene->entityData = new EntityData();
        scene->entityData->engine = engine;
        scene->entityData->scene = scene;
        scene->engine = engine;
        scene->sceneIndex = scenes.size()-1;
        //UnloadCurrentScene();
        //currentScene = scene;
        return scene;
    }
    
    void UnloadCurrentScene();

    int getSceneCount();
    void SetCurrentScene(int position);

private:

    Engine* engine;

    //Scene currentScene;

    std::vector<std::unique_ptr<Scene>> scenes;
};

class Collider2D : public Component {
    CLASS_DECLARATION( Collider2D )
public:
    Collider2D(std::string&& initialValue)
        : Component(std::move(initialValue)) {
    }

    Collider2D() = default;

    void Update(Entity* Entity);
    void Init(b2World world);

    b2Shape* getShape();
private:
    b2Shape* shape;
};

class BoxCollider2D : public Collider2D {
    CLASS_DECLARATION( BoxCollider2D )
public:
    BoxCollider2D(std::string&& initialValue)
        : Collider2D(std::move(initialValue)) {
    }

    BoxCollider2D();

    void Update(Entity* Entity);
    void Init(b2World* world, float sx, float sy);

    b2PolygonShape* getShape();
private:
    b2PolygonShape shape;
};

class Rigidbody2D : public Component {
    CLASS_DECLARATION( Rigidbody2D )
public:
    Rigidbody2D(std::string&& initialValue)
        : Component(std::move(initialValue)) {
    }

    Rigidbody2D() = default;

    b2Body* body;

    void Update(Entity* Entity);
    void Init(b2World* world, Entity* entity, const b2Shape* shape);
private:
};

class Player : public Component {
    CLASS_DECLARATION(Player)
public:
    Player(std::string&& initialValue)
        : Component(std::move(initialValue)) {
    }

    Player() = default;

    inline void Update(Entity* entity) {

        float horizontalAxis = InputManager::GetInputAxis(LEFT);
        float verticalAxis = InputManager::GetInputAxis(UP);

        if (entity->GetComponent<Rigidbody2D>()) {
            Rigidbody2D* rb = entity->GetComponent<Rigidbody2D>();

            rb->body->ApplyLinearImpulseToCenter({ horizontalAxis*1000.0f,0},true);

            //printf("%.2f\n", horizontalAxis);

        }
        else {

            entity->localPosition = Vector3Add(entity->localPosition, { horizontalAxis*2.5f,verticalAxis*2.5f,0 });

            if (horizontalAxis > 0) {
                AnimatedRenderer2D* renderer = entity->GetComponent<AnimatedRenderer2D>();
                if (renderer) {
                    renderer->srcRect.width = -abs(renderer->srcRect.width);
                }
            }
            else if (horizontalAxis < 0) {
                AnimatedRenderer2D* renderer = entity->GetComponent<AnimatedRenderer2D>();
                if (renderer) {
                    renderer->srcRect.width = abs(renderer->srcRect.width);
                }
            }

            //printf("%.2f\n", horizontalAxis);
        }

        /*

        if (direction == true) {
            health += 0.005f;
            //direction = false;
        }
        if (direction == false) {
            health -= 0.005f;
            //direction = true;
        }

        if (health <= 0.0f) {
            direction = true;
        }
        if (health >= 1.0f) {
            direction = false;
        }

        */

        health = Clamp(health, 0.0f, 1.0f);

        //printf("%.3f\n", health);

        if (health > 0.59 && health < 0.69) {
            entity->scene->entityData->entities.at(1)->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 5, 18 * 2, 18, 18 };
        }
        if (health <= 0.59) {
            entity->scene->entityData->entities.at(1)->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 6, 18 * 2, 18, 18 };
        }

        if (health >= 0.69) {
            entity->scene->entityData->entities.at(1)->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 4, 18 * 2, 18, 18 };
        }

        if (health > 0.39 && health < 0.49) {
            entity->scene->entityData->entities.at(2)->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 5, 18 * 2, 18, 18 };
        }
        if (health <= 0.39) {
            entity->scene->entityData->entities.at(2)->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 6, 18 * 2, 18, 18 };
        }

        if (health >= 0.49) {
            entity->scene->entityData->entities.at(2)->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 4, 18 * 2, 18, 18 };
        }

        if (health > 0.00 && health < 0.19) {
            entity->scene->entityData->entities.at(3)->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 5, 18 * 2, 18, 18 };
        }
        if (health <= 0.00) {
            entity->scene->entityData->entities.at(3)->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 6, 18 * 2, 18, 18 };
        }

        if (health >= 0.19) {
            entity->scene->entityData->entities.at(3)->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 4, 18 * 2, 18, 18 };
        }

    }

    inline void Init() {
        health = 1.0f;
    }

    float health;
    bool direction;
};

class PhysicsMgr {
public:
    PhysicsMgr();

    void Init();
    void Update();

    b2Vec2 gravity;
    b2World* world;
};

class CameraMgr {
public:
    CameraMgr() = default;

    aurCamera* loadCamera();
    aurCamera* loadCamera(Camera2D cam);
    aurCamera* loadCamera(Camera3D cam);

    aurCamera* currentCamera;
};

class TextureMgr {
public:
    // Load a texture
    TextureRes* LoadTextureM(const char* path);

    // Load a texture
    TextureRes* LoadTextureM(Texture2D tex);

    // Unload a texture.
    void UnloadTexture(int tindex);

    void UnloadAll();

    std::vector<TextureRes*> textures;

};

class Engine {
public:
    Engine() = default;

    SceneMgr* sceneMgr;
    PhysicsMgr* physicsMgr;
    CameraMgr* cameraMgr;
    TextureMgr* textureMgr;

    void Init();
    void Update();
    void Unload();
};


static Engine* engine;

void Draw3D(Scene scene, Camera3D camera); // Draw 3D scene to window/screen
void Draw3DCallback(Scene scene, Camera3D camera, void(*callback)()); // Draw 3D scene to window/screen (with callbacks)

void LoadSceneFromJson(const char* path); // Get scene from JSON
Scene CreateScene(); // Create scene from scratch
ModelRes LoadModelRes(const char* path); // Load ModelRes from path
void LoadModelToScene(ModelRes* model, Scene scene); // Load ModelRes into scene
void SaveSceneToJson(const char* path); // Save scene to JSON

// KNIGHTS IMPLEMENTATION

static int perlinSEED = 0;

static int perlinhash[] = { 208,34,231,213,32,248,233,56,161,78,24,140,71,48,140,254,245,255,247,247,40,
                     185,248,251,245,28,124,204,204,76,36,1,107,28,234,163,202,224,245,128,167,204,
                     9,92,217,54,239,174,173,102,193,189,190,121,100,108,167,44,43,77,180,204,8,81,
                     70,223,11,38,24,254,210,210,177,32,81,195,243,125,8,169,112,32,97,53,195,13,
                     203,9,47,104,125,117,114,124,165,203,181,235,193,206,70,180,174,0,167,181,41,
                     164,30,116,127,198,245,146,87,224,149,206,57,4,192,210,65,210,129,240,178,105,
                     228,108,245,148,140,40,35,195,38,58,65,207,215,253,65,85,208,76,62,3,237,55,89,
                     232,50,217,64,244,157,199,121,252,90,17,212,203,149,152,140,187,234,177,73,174,
                     193,100,192,143,97,53,145,135,19,103,13,90,135,151,199,91,239,247,33,39,145,
                     101,120,99,3,186,86,99,41,237,203,111,79,220,135,158,42,30,154,120,67,87,167,
                     135,176,183,191,253,115,184,21,233,58,129,233,142,39,128,211,118,137,139,255,
                     114,20,218,113,154,27,127,246,250,1,8,198,250,209,92,222,173,21,88,102,219 };

inline int noise2(int x, int y)
{
    int tmp = perlinhash[(y + perlinSEED) % 256];
    return perlinhash[(tmp + x) % 256];
}

inline float lin_inter(float x, float y, float s)
{
    return x + s * (y - x);
}

inline float smooth_inter(float x, float y, float s)
{
    return lin_inter(x, y, s * s * (3 - 2 * s));
}

inline float noise2d(float x, float y)
{
    int x_int = x;
    int y_int = y;
    float x_frac = x - x_int;
    float y_frac = y - y_int;
    int s = noise2(x_int, y_int);
    int t = noise2(x_int + 1, y_int);
    int u = noise2(x_int, y_int + 1);
    int v = noise2(x_int + 1, y_int + 1);
    float low = smooth_inter(s, t, x_frac);
    float high = smooth_inter(u, v, x_frac);
    return smooth_inter(low, high, y_frac);
}

inline float perlin2d(float x, float y, float freq, int depth)
{
    float xa = x * freq;
    float ya = y * freq;
    float amp = 1.0;
    float fin = 0;
    float div = 0.0;

    int i;
    for (i = 0; i < depth; i++)
    {
        div += 256 * amp;
        fin += noise2d(xa, ya) * amp;
        amp /= 2;
        xa *= 2;
        ya *= 2;
    }

    return fin / div;
}

namespace Game {
    namespace UI {
        namespace Devel {
            inline void DrawConsole(Camera2D* cam, bool* lock, b2World* world, Engine* engine, Player* player, EntityData* entityData)
            {
                ImGui::Begin("GDC");

                ImGui::SliderFloat("Camera Target - X", &cam->target.x, -1000, 1000);
                ImGui::SliderFloat("Camera Target - Y", &cam->target.y, -1000, 1000);
                ImGui::Text("Camera Target: %.2f, %.2f", cam->target.x, cam->target.y);
                ImGui::Checkbox("Lock Camera", lock);
                ImGui::Text("Gravity: %.2f, %.2f", world->GetGravity().x, world->GetGravity().y);
                ImGui::SliderFloat("Camera Zoom", &cam->zoom, 0, 100);
                ImGui::SliderFloat("Player Health", &player->health, 0.0f, 1.0f);

                ImGui::End();

                //ImGui::SliderFloat("Gravity - X", &engine->physicsMgr->gravity.x, -1000, 1000);
                //ImGui::SliderFloat("Gravity - Y", &engine->physicsMgr->gravity.y, -1000000, 1000);
            }
        }
    }
};

struct compare
{
    Vector2 key;
    compare(Vector2 const& i) : key(i) {}

    bool operator()(Vector2 const& i) {
        return (Vector2Equals(i, key));
    }
};

#define csprivate(x) private: x public:
#define cspublic(x) public: x private:

class TerrainGenerator : public Component {
    CLASS_DECLARATION(TerrainGenerator)
public:
    TerrainGenerator(std::string&& initialValue)
        : Component(std::move(initialValue)) {
    }

    TerrainGenerator() = default;

    int worldSize;
    float caveFreq, terrainFreq;
    float seed;
    float surfaceValue, dirtHeight;
    float heightMultiplier;
    int heightAddition;
    Image caveNoiseTexture;
    Texture2D stone;
    Texture2D dirt;
    Texture2D grass;
    Texture2D baseLog;
    Texture2D middleLog;
    Texture2D leaf;
    int treeChance;
    bool generateCaves;
    std::int32_t octaves;
    float perlinFreq;
    std::vector<Vector2> tiles;
    std::vector<Entity*> worldChunks;
    float minTreeHeight = 3;
    float maxTreeHeight = 10;
    int chunkSize = 16;

    float coalRarity, ironRarity, goldRarity, diamondRarity;
    Image coalSpread, ironSpread, goldSpread, diamondSpread;
   

    inline void Init(Scene* connectedScene, const char* path1, const char* path2, const char* path3) {
        worldSize = 400;
        caveFreq = 0.08f;
        terrainFreq = 0.06f;
        surfaceValue = 0.25f;
        seed = GetRandomValue(-10000,10000);
        heightMultiplier = 25;
        heightAddition = 25;
        dirtHeight = 5;
        generateCaves = true;
        stone = LoadTexture(path1);
        dirt = LoadTexture(path2);
        grass = LoadTexture(path3);
        treeChance = 15;
        octaves = 8;
        perlinFreq = 8.0f;
        scene = connectedScene;
        GenerateNoiseTexture(caveFreq, &caveNoiseTexture);
        CreateChunks();
        //GenerateTerrain();
    };

    inline void Update(Entity* entity) {

    }

    void CreateChunks() {
        int numChunks = worldSize / chunkSize;
        worldChunks.clear();
        for (int i = 0; i < numChunks; i++) {
            Entity* newChunk = scene->entityData->createEntity(TextFormat("%d", i));
            entity->AddChild(newChunk);
            worldChunks.push_back(newChunk);

        }
    }

    inline void GenerateTerrain() {
        for (int x = 0; x < worldSize; x++) {

            float height = Clamp((perlin.octave2D_01((x + seed) * terrainFreq, seed * terrainFreq, octaves)), 0, 1) * heightMultiplier + heightAddition;

            //printf("%.2f\n", height);

            for (int y = 0; y < height; y++) {
                //printf("%.2f\n",GetImageColor(noiseTexture, x, y).r);

                Texture2D tileSprite;
                Color tint = WHITE;


                if (y < height - dirtHeight) {

                    tileSprite = stone;
                }
                else if (y < height - 1) {
                    tileSprite = dirt;
                }
                else {
                    tileSprite = grass;

                    // we got the top layer
                }

                if (generateCaves) {
                    if (GetImageColor(caveNoiseTexture, x, y).r > 255 * surfaceValue)
                    {
                        PlaceTile(tileSprite, x, y);
                    }
                }
                else {
                    PlaceTile(tileSprite, x, y);
                }

                if (y >= height - 1) {
                    int t = GetRandomValue(0, treeChance);
                    if (t == 1) {
                        // generate a tree
                        if (std::find_if(tiles.begin(), tiles.end(), compare(Vector2{ (float)x,(float)y })) != tiles.end()) {
                            GenerateTree(x, y + 1);
                        }
                    }
                }

            }
        }
    }

    const siv::PerlinNoise perlin{(siv::PerlinNoise::seed_type)12345};

    inline void Unload() {
        UnloadImage(coalSpread);
        UnloadImage(ironSpread);
        UnloadImage(goldSpread);
        UnloadImage(diamondSpread);
    }

private:

    Scene* scene;

    void GenerateTree(float x, float y) {

        //printf("generating a tree\n");

        int treeHeight = GetRandomValue(minTreeHeight, maxTreeHeight);

        // generate log
        for (int i = 0; i <= treeHeight; i++) {
            if (i == 0) {
                PlaceTile(middleLog, x, y + i);
            }
            else {
                PlaceTile(middleLog, x, y + i);
            }
        }

        // generate leaves
        PlaceTile(leaf, x, y + treeHeight);
        PlaceTile(leaf, x, y + treeHeight+1);
        PlaceTile(leaf, x, y + treeHeight+2);

        PlaceTile(leaf, x-1, y + treeHeight);
        PlaceTile(leaf, x-1, y + treeHeight+1);

        PlaceTile(leaf, x + 1, y + treeHeight);
        PlaceTile(leaf, x + 1, y + treeHeight + 1);
    }

    void PlaceTile(Texture2D sprite, float x, float y) {

        int chunkCoord = round((int)x / chunkSize) * chunkSize;
        chunkCoord /= chunkSize;
        chunkCoord = Clamp(chunkCoord, 0, (worldSize / chunkSize)-1);

        //printf("%d\n", chunkCoord);

        Entity* chunk = worldChunks.at(chunkCoord);

        float tileSize = 1;

        Entity* newTile = scene->entityData->createEntity(TextFormat("tile %.2f, %.2f", x, y));
        newTile->localPosition = { ((float)x*chunk->globalScale.x) + ((sprite.width / 2)), ((float)y*chunk->globalScale.y) + ((sprite.height / 2)) };
        worldChunks.at(chunkCoord)->AddChild(newTile);
        Renderer2D* renderer = newTile->AddComponent<Renderer2D>();
        renderer->rec = { 0,0,tileSize * chunk->globalScale.x, tileSize * chunk->globalScale.y };
        renderer->Init(engine->textureMgr->LoadTextureM(sprite));
        tiles.push_back(Vector2Subtract(Vector2Divide({ newTile->localPosition.x, newTile->localPosition.y }, {chunk->globalScale.x, chunk->globalScale.y}), Vector2Multiply(Vector2One(), {((float)(sprite.width / 2)),((float)(sprite.height / 2))})));
    }

    void GenerateNoiseTexture(float frequency, Image* noiseTexture) {

        UnloadImage(*noiseTexture);

        Color* pixels = (Color*)RL_MALLOC(worldSize * worldSize * sizeof(Color));
        
        for (int y = 0; y < worldSize; y++)
        {
            for (int x = 0; x < worldSize; x++)
            {
                float v = perlin.octave2D_01((x + seed) * frequency, (y + seed) * frequency, octaves);
                unsigned char vColor = v * 255;
                pixels[y * worldSize + x] = { vColor, vColor, vColor, 255 };
            }
        }

        noiseTexture->data = pixels;
        noiseTexture->width = worldSize;
        noiseTexture->height = worldSize;
        noiseTexture->format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        noiseTexture->mipmaps = 1;
    }
};

inline Entity* QuickAnimated(const char* tag, const char* textureName, EntityData* entityData, Rectangle src, Rectangle rec, const char* name) {
    Entity* heart = entityData->createEntity(name);
    AnimatedRenderer2D* heartRen = heart->AddComponent<AnimatedRenderer2D>();
    heartRen->Init(textureName);
    heartRen->srcRect = src;
    heartRen->rec = rec;
    heart->tag = tag;

    return heart;
}

inline Entity* QuickAnimated(const char* tag, TextureRes* texture, EntityData* entityData, Rectangle src, Rectangle rec, const char* name) {
    Entity* heart = entityData->createEntity(name);
    AnimatedRenderer2D* heartRen = heart->AddComponent<AnimatedRenderer2D>();
    heartRen->Init(texture);
    heartRen->srcRect = src;
    heartRen->rec = rec;
    heart->tag = tag;

    return heart;
}

class TestScene : public Scene {
public:
    void scene_init() {


        originalSetup();
        Entity* player = entityData->createEntity("Player");
        AnimatedRenderer2D* renderer = player->AddComponent<AnimatedRenderer2D>();
        renderer->Init("resources/test/characters_packed.png");
        renderer->srcRect = { 48, 0, 24, 24 };
        renderer->rec = { 0,0,60,60 };
        Player* playerController = player->AddComponent<Player>();
        playerController->Init();

        player->localPosition = { 0,0,0 };
        //Rigidbody2D* rigidBody = player->AddComponent<Rigidbody2D>();
        b2PolygonShape plrBox;
        plrBox.SetAsBox(1.0f, 1.0f);
        //rigidBody->Init(engine->physicsMgr->world, player, &plrBox);

        Camera2D camera = { 0 };
        camera.target = { player->localPosition.x+20.0f,player->localPosition.y+20.0f };
        camera.offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
        camera.rotation = 0.0f;
        camera.zoom = 1.0f;
        aurCamera* cam = engine->cameraMgr->loadCamera(camera);
        cam->Attach(player, renderer->rec.width, renderer->rec.height);
        cam->useCam2D = true;
        

        Entity* heart1 = QuickAnimated("ui", "resources/test/tiles_packed.png", entityData, { 18 * 6, 18 * 2, 18, 18 }, { 0,0,50,50 }, "heart1");
        heart1->localPosition = { 0.95f * GetScreenWidth(), 0.0125f * GetScreenHeight() };

        Entity* heart2 = QuickAnimated("ui", engine->textureMgr->textures.at(1), entityData, {18 * 5, 18 * 2, 18, 18}, {0,0,50,50}, "heart2");
        heart2->localPosition = { 0.915f * GetScreenWidth(), 0.0125f * GetScreenHeight() };

        Entity* heart3 = QuickAnimated("ui", engine->textureMgr->textures.at(1), entityData, {18 * 4, 18 * 2, 18, 18}, {0,0,50,50}, "heart3");
        heart3->localPosition = { 0.88f * GetScreenWidth(), 0.0125f * GetScreenHeight() };

        Entity* terrain = entityData->createEntity("terrain");

        float terrainScale = 30;

        terrain->localScale = { terrainScale,terrainScale,terrainScale };
        TerrainGenerator* terrainGenerator = terrain->AddComponent<TerrainGenerator>();
        terrainGenerator->Init(this, "resources/boxgrey.png", "resources/boxbrown.png", "resources/boxgreen.png");
        terrainGenerator->baseLog = LoadTexture("resources/boxbrown.png");
        terrainGenerator->middleLog = LoadTexture("resources/boxbrown.png");
        terrainGenerator->leaf = LoadTexture("resources/boxgreen.png");
        terrainGenerator->GenerateTerrain();

        /*
        Entity* heart = entityData->createEntity();
        AnimatedRenderer2D* heartRen = heart->AddComponent<AnimatedRenderer2D>();
        heartRen->Init("resources/test/tiles_packed.png");
        heartRen->srcRect = { 18 * 4, 18*2, 18, 18 };
        heartRen->rec = { 0,0,50,50 };
        heart->tag = "ui";
        */

    }

    void scene_update(aurCamera* cam) {

        engine->physicsMgr->world->SetGravity({0,0});

        //engine->physicsMgr->gravity.SetZero();

        if (!lockCamera) {

            cam->cam2D.target = { entityData->find("Player")->localPosition.x + 20, entityData->find("Player")->localPosition.y + 20};

        }

        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D((Camera2D)cam->cam2D);

        rlPushMatrix();
        rlTranslatef(0, 25 * 50, 0);
        rlRotatef(90, 1, 0, 0);
        DrawGrid(100, 50);
        rlPopMatrix();

        entityData->Update();

        

        EndMode2D();
        DrawFPS(10, 10);

        entityData->UI_Update();


        rlImGuiBegin();

        Game::UI::Devel::DrawConsole(&cam->cam2D, &lockCamera, engine->physicsMgr->world, engine, entityData->entities.at(0)->GetComponent<Player>(), entityData);

        rlImGuiEnd();

        EndDrawing();
    }

    void scene_unload() {
        entityData->Unload();
        originalUnload();
    }

    bool is2D = true;
    bool lockCamera;

    std::vector<Texture2D> cachedTextures;

};