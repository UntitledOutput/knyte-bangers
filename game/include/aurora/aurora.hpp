// main renderer
#include "raylibCpp/raylib-cpp.hpp"
#include "rlgl.h"
#include "rlImGui.h"	// include the API header
#include "rlImGuiColors.h"
#include "imgui.h"

// utilities
#include "cJSON/cJSON.h"

// required headers
#include <typeinfo> 
#include <memory>
#include <algorithm>

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

class aurCamera {
public:
    aurCamera() = default;

    Camera2D cam2D;
    Camera3D cam3D;

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
};

class Entity {
public:

    Vector3 position;
    Vector3 rotation;
    Vector3 scale;

    ComponentHolder* componentHolder;

    template< class ComponentType, typename... Args >
    ComponentType* AddComponent(Args&&... params);

    template< class ComponentType >
    ComponentType* GetComponent();

    template< class ComponentType >
    bool RemoveComponent();

    template< class ComponentType >
    std::vector< ComponentType* > GetComponents();

    template< class ComponentType >
    int RemoveComponents();

    virtual void Update();
};

class Renderer2D : public Component {
    CLASS_DECLARATION( Renderer2D )
public:
    Renderer2D(std::string&& initialValue)
        : Component(std::move(initialValue)) {
    }

    Renderer2D() = default;

    Texture2D texture;

    void Update(Entity* Entity);
    void Init();
    Rectangle rec;
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
    Entity* createEntity();
    int entityCount;
    void Update();
};

class Scene {
public:
    Scene() = default;
    ModelData* modelData;
    EntityData* entityData;
    virtual void scene_init();
    virtual void scene_update(aurCamera* cam);
    virtual void scene_unload();
    void originalBegin2D(aurCamera* cam);
    void originalBegin3D(aurCamera* cam);
    void originalEnd();
    void originalSetup();
    void originalUnload();
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

    BoxCollider2D() = default;

    void Update(Entity* Entity);
    void Init(b2World world, float sx, float sy);

    b2PolygonShape* getShape();
private:
    b2PolygonShape* shape;
};

class Rigidbody2D : public Component {
    CLASS_DECLARATION( Rigidbody2D )
public:
    Rigidbody2D(std::string&& initialValue)
        : Component(std::move(initialValue)) {
    }

    Rigidbody2D() = default;

    void Update(Entity* Entity);
    void Init(b2World world, Entity* entity);
private:
    b2Body* body;
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

    aurCamera loadCamera();
    aurCamera loadCamera(Camera2D cam);
    aurCamera loadCamera(Camera3D cam);

    aurCamera* currentCamera;
};

class Engine {
public:
    Engine() = default;

    SceneMgr* sceneMgr;
    PhysicsMgr* physicsMgr;
    CameraMgr* cameraMgr;

    void Init();
    void Update();
};

void Draw3D(Scene scene, Camera3D camera); // Draw 3D scene to window/screen
void Draw3DCallback(Scene scene, Camera3D camera, void(*callback)()); // Draw 3D scene to window/screen (with callbacks)

void LoadSceneFromJson(const char* path); // Get scene from JSON
Scene CreateScene(); // Create scene from scratch
ModelRes LoadModelRes(const char* path); // Load ModelRes from path
void LoadModelToScene(ModelRes* model, Scene scene); // Load ModelRes into scene
void SaveSceneToJson(const char* path); // Save scene to JSON


// KNIGHTS IMPLEMENTATION

class TestScene : public Scene {
public:
    void scene_init() {
        originalSetup();
        Entity* player = entityData->createEntity();
        Renderer2D* renderer = player->AddComponent<Renderer2D>();
        renderer->Init();
        renderer->texture = LoadTexture("resources/mecha.png");
        renderer->rec = { 400, 280, 40, 40 };
        player->position = { 0,0,0 };


        Camera2D camera = { 0 };
        camera.target = { player->position.x + 20.0f, player->position.y + 20.0f };
        camera.offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
        camera.rotation = 0.0f;
        camera.zoom = 1.0f;
        aurCamera cam = engine->cameraMgr->loadCamera(camera);
    }

    void scene_update(aurCamera* cam) {
        cam->cam2D.target = { 0,0 };
        cam->cam2D.offset = { 0,0 };
        originalBegin2D(cam);
        originalEnd();
    }

    void scene_unload() {
        originalUnload();
    }

    bool is2D = true;
};