
#include "aurora.hpp"
//#include "aurora.hpp"

#if !defined(MAX_MODEL_COUNT)
#define MAX_MODEL_COUNT 10000
#endif // MACRO

#if !defined(MAX_Entity_COUNT)
#define MAX_Entity_COUNT 10000
#endif // MACRO

const std::size_t Component::Type = std::hash<const char*>()(TO_STRING(Component));

CLASS_DEFINITION(Component, Renderer2D)
CLASS_DEFINITION(Component, Rigidbody2D)
CLASS_DEFINITION(Component, Collider2D)
CLASS_DEFINITION(Collider2D, BoxCollider2D)
CLASS_DEFINITION(Renderer2D, AnimatedRenderer2D)

Camera3D Load3DCamera(float fovy, Vector3 position, CameraProjection projection, Vector3 target, Vector3 up) {
    Camera3D cam = {0};
    cam.fovy = fovy;
    cam.position = position;
    cam.projection = projection;
    cam.target = target;
    cam.up = up;
    return cam;
}

Camera2D Load2DCamera(Vector2 offset, float rotation, Vector2 target, float zoom) {
    Camera2D cam = {0};
    cam.offset = offset;
    cam.rotation = rotation;
    cam.target = target;
    cam.zoom = zoom;
    return cam;
}

void Draw3D(Scene scene, Camera3D camera)
{
    BeginDrawing();

    ClearBackground(RAYWHITE);

    BeginMode3D(camera);

    for (ModelRes* model : scene.modelData->models) {
        //ModelRes* model = scene.modelData->models.at(i);
        DrawModel(model->model, {0,0,0}, 1.0f, RED);
    }

    EndMode3D();

    EndDrawing();
}

void Draw3DCallback(Scene scene, Camera3D camera, void(*callback)())
{
    BeginDrawing();

    ClearBackground(RAYWHITE);

    BeginMode3D(camera);

    for (ModelRes* model : scene.modelData->models) {
        //ModelRes* model = scene.modelData->models.at(i);
        DrawModel(model->model, {0,0,0}, 1.0f, RED);
    }

    callback();

    EndMode3D();

    EndDrawing();
}

void LoadSceneFromJson(const char *path){

}

void SaveSceneToJson(const char *path, Scene scene){

}

Scene InitializeScene(){
    Scene scene = Scene();

    scene.modelData = new ModelData();
    scene.entityData = new EntityData();

    //scene.modelData = (ModelData *)RL_CALLOC(1, sizeof(ModelData));

    //scene.modelData->modelCount = 0;

    //scene.modelData->models = (ModelRes* )RL_CALLOC(MAX_MODEL_COUNT, sizeof(ModelRes));

    //scene.entityData = (EntityData *)RL_CALLOC(1, sizeof(EntityData));

    //scene.entityData->entityCount = 0;

    //scene.entityData->entities = (Entity* )RL_CALLOC(MAX_Entity_COUNT, sizeof(Entity));

    return scene;
}
ModelRes LoadModelRes(const char *path)
{

    ModelRes model = {0};

    model.model = LoadModel(path);
    model.path = path;

    model.model.materials[0].shader = LoadShader("resources/pbr.vs", "resources/pbr.fs");

    return model;
}
void LoadModelToScene(ModelRes* model, Scene scene){
    scene.modelData->models.push_back(model);
};

template <class CompType, typename... Args>
void ComponentHolder::Attach(Args&&... params) {
    components.emplace_back(std::make_unique< CompType >(std::forward< Args >(params)...));
}

void ComponentHolder::Update(Entity* Entity) {
    for (auto const& component : components) {

        auto comp = component.get();

        comp->Update(Entity);
    }
};

template< class ComponentType >
ComponentType* Entity::GetComponent() {
    for (auto&& component : componentHolder->components) {
        if (component->IsClassType(ComponentType::Type))
            return static_cast<ComponentType*>(component.get());
    }

    return new ComponentType();
}

template< class ComponentType >
bool Entity::RemoveComponent() {
    if (componentHolder->components.empty())
        return false;

    auto& index = std::find_if(componentHolder->components.begin(),
        componentHolder->components.end(),
        [classType = ComponentType::Type](auto& component) {
            return component->IsClassType(classType);
        });

    bool success = index != componentHolder->components.end();

    if (success)
        componentHolder->components.erase(index);

    return success;
}

template< class ComponentType >
std::vector< ComponentType* > Entity::GetComponents() {
    std::vector< ComponentType* > componentsOfType;

    for (auto&& component : componentHolder->components) {
        if (component->IsClassType(ComponentType::Type))
            componentsOfType.emplace_back(static_cast<ComponentType*>(component.get()));
    }

    return componentsOfType;
}

template< class ComponentType >
int Entity::RemoveComponents() {
    if (componentHolder->components.empty())
        return 0;

    int numRemoved = 0;
    bool success = false;

    do {
        auto& index = std::find_if(componentHolder->components.begin(),
            componentHolder->components.end(),
            [classType = ComponentType::Type](auto& component) {
                return component->IsClassType(classType);
            });

        success = index != componentHolder->components.end();

        if (success) {
            componentHolder->components.erase(index);
            ++numRemoved;
        }
    } while (success);

    return numRemoved;
}

void Component::Update(Entity* Entity) {};
void Entity::Update() {
    //DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2, 10, RED);
    componentHolder->Update(this);
};

void Renderer2D::Update(Entity* entity) {
    DrawTextureRec(texture, rec, {entity->position.x,entity->position.y }, WHITE);
}

void AnimatedRenderer2D::Update(Entity* Entity) 
{
    framesCounter++;
    if (framesCounter >= (60/12))
    {
    framesCounter = 0;
    currentFrame++;

    if (currentFrame > frameCounter-1) currentFrame = 0;

    frameRec.x = (float)currentFrame*(float)texture.width/frameCounter;
    }
    DrawTextureRec(texture, frameRec, {Entity->position.x,Entity->position.y }, WHITE);
}

void AnimatedRenderer2D::Init(Texture tex, int frameCount)
{
    texture = tex;
    frameCounter = frameCount;  
    frameRec = { 0.0f, 0.0f, (float)texture.width/frameCount, (float)texture.height/frameCount };
}

void Renderer2D::Init()
{
}

void Actor2D::Init()
{
    renderer = AddComponent<Renderer2D>();
}

void Actor2D::Update()
{
    componentHolder->Update(this);
}

void SceneMgr::Init(Engine* eng)
{
    engine = eng;
}

void SceneMgr::Update(CameraMgr* camMgr)
{
    try
    {
        for (auto const& scene : scenes) {
            if (scene->isActive) {
                auto curScene = scene.get();
                
                curScene->scene_update(camMgr->currentCamera);
            }
        }
    }
    catch (const std::exception&)
    {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawText("Unable to update scene... :(", 250, 200, 20, LIGHTGRAY);

        EndDrawing();
    }
}
/*
void SceneMgr::LoadScene(const char* path)
{

}
*/


void SceneMgr::UnloadCurrentScene()
{
    for (auto const& scene : scenes) {
        if (scene->isActive) {
            auto curScene = scene.get();
            
            curScene->scene_unload();
        }
    }
    //currentScene = nullptr;
}

int SceneMgr::getSceneCount() {
    return scenes.size();
}

void SceneMgr::SetCurrentScene(int position) {

}

void Engine::Init()
{
    sceneMgr = new SceneMgr();
    physicsMgr = new PhysicsMgr();
    cameraMgr = new CameraMgr();


    sceneMgr->Init(this);
    physicsMgr->Init();
}

void Engine::Update()
{
    sceneMgr->Update(cameraMgr);
    physicsMgr->Update();
}

void Scene::scene_init()
{
    printf("initialized scene");
}

void Scene::scene_update(aurCamera* cam)
{
    printf("updated scene");
    //-------------
}

void Scene::scene_unload()
{
    printf("unloaded scene");
}

void Scene::originalBegin2D(aurCamera* cam)
{
    if (cam->attachedEntity) {
        cam->cam2D.target = {cam->attachedEntity->position.x, cam->attachedEntity->position.y};
        cam->cam2D.offset = {0,0};
    }
    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode2D((Camera2D)cam->cam2D);
    entityData->Update();
    DrawFPS(10,10);
}

void Scene::originalBegin3D(aurCamera* cam)
{
    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D((Camera3D)cam->cam3D);
    entityData->Update();
}

void Scene::originalEnd()
{
    rlImGuiBegin();
    ImGui::ShowDemoWindow();
    //ImGui::ShowDebugLogWindow(true);
    rlImGuiEnd();
    if (is2D) {
        EndMode2D();
    }
    EndDrawing();
}

void Scene::originalSetup()
{
    rlImGuiSetup(true);
}

void Scene::originalUnload()
{
    rlImGuiShutdown();
}

void Rigidbody2D::Init(b2World world, Entity* entity)
{
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(entity->position.x, entity->position.y);
    b2Body* body = world.CreateBody(&bodyDef);
    b2FixtureDef fixtureDef;
    fixtureDef.shape = entity->GetComponent<Collider2D>()->getShape();
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.3f;
    body->CreateFixture(&fixtureDef);
}

void Collider2D::Update(Entity *Entity)
{
}

void Collider2D::Init(b2World world)
{

}

b2Shape *Collider2D::getShape()
{
    return shape;
}

void BoxCollider2D::Update(Entity *Entity)
{
    for (int i = 0; i < sizeof(shape->m_vertices); i++) {
        Vector2 vert = {shape->m_vertices[i].x, shape->m_vertices[i].y};
        Vector2 nextVert;
        if (i == sizeof(shape->m_vertices)-1) {
            nextVert = {shape->m_vertices[0].x, shape->m_vertices[0].y};
        } else {
            nextVert = {shape->m_vertices[i+1].x, shape->m_vertices[i+1].y };
        }
        DrawLineV(vert, nextVert, GREEN);
    }
}

void BoxCollider2D::Init(b2World world, float sx, float sy)
{
    shape->SetAsBox(sx/2, sx/2);
}

b2PolygonShape *BoxCollider2D::getShape()
{
    return shape;
}

void PhysicsMgr::Init()
{
    gravity = b2Vec2(0,-100);
    world->SetGravity(gravity);
}

void PhysicsMgr::Update()
{
    float timeStep = 1.0f / GetFPS();
    int32 velocityIterations = 6;
    int32 positionIterations = 2;
    world->Step(timeStep, velocityIterations, positionIterations);
}

Entity *EntityData::createEntity()
{
    Entity* entity = new Entity();
    entity->componentHolder = new ComponentHolder();
    entity->scale = { 1,1,1 };
    entities.push_back(entity);
    return entity;
}

void EntityData::Update()
{
    for (Entity* entity : entities) {
        entity->Update();
    }
}

aurCamera CameraMgr::loadCamera()
{
    aurCamera* camera = new aurCamera();
    delete currentCamera;
    currentCamera = camera;
    //camera->cam2D = cam;
    return *camera;
}

aurCamera CameraMgr::loadCamera(Camera2D cam)
{
    aurCamera* camera = new aurCamera();
    delete currentCamera;
    currentCamera = camera;
    camera->cam2D = cam;
    return *camera;
}

aurCamera CameraMgr::loadCamera(Camera3D cam)
{
    aurCamera* camera = new aurCamera();
    delete currentCamera;
    currentCamera = camera;
    camera->cam3D = cam;
    return *camera;
}

PhysicsMgr::PhysicsMgr() {
    world = new b2World(gravity);
};