
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
CLASS_DEFINITION(Component, Player)
CLASS_DEFINITION(Component, TerrainGenerator)
CLASS_DEFINITION(Component, Button)
CLASS_DEFINITION(Component, Block)

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

        if (comp->enabled) {
            comp->Update(Entity);
        }
    }
}
void ComponentHolder::FixedUpdate(Entity* entity)
{
}
void ComponentHolder::Unload()
{
    for (auto const& component : components) {

        auto comp = component.get();

        comp->Unload();
    }

    components.clear();
}
;

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

void Component::Update(Entity* Entity) {}
void Component::Unload()
{
}
void Component::SetEnabled(bool enb)
{
}
void Component::FixedUpdate(Entity* entity)
{
}
;
void Entity::Update() {
    //DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2, 10, RED);

    Vector3 absPos = this->localPosition;
    for (Entity* p = this->parent; p != nullptr; p = p->parent)
        absPos = Vector3Add(absPos, p->localPosition);
    globalPosition = absPos;

    Vector3 absRot = this->localRotation;
    for (Entity* p = this->parent; p != nullptr; p = p->parent)
        absRot = Vector3Add(absRot, p->localRotation);
    globalRotation = absRot;

    Vector3 absScl = this->localScale;
    for (Entity* p = this->parent; p != nullptr; p = p->parent)
        absScl = Vector3Add(absScl, p->localScale);
    globalScale = absScl;

    if (enabled) {
        componentHolder->Update(this);
    }

    for (Entity* child : children) {
        child->enabled = enabled;
    }
}
void Entity::FixedUpdate()
{
    if (enabled) {
        componentHolder->FixedUpdate(this);
    }
}
void Entity::Unload()
{
    enabled = false;
    componentHolder->Unload();
}
;

void Renderer2D::Update(Entity* entity) {
    //rec = { 0,0,(float)texture->tex.width, (float)texture->tex.height };
    texture->Update();


    DrawTexturePro(texture->tex, { 0,0,(float)texture->tex.width, (float)texture->tex.height }, {-entity->globalPosition.x * entity->globalScale.x,-entity->globalPosition.y * entity->globalScale.y, rec.width* entity->globalScale.x, rec.height*entity->globalScale.y}, { 0,0 }, entity->globalRotation.y, tint);

}

void AnimatedRenderer2D::Update(Entity* entity) 
{

    DrawTexturePro(
        texture->tex,
        srcRect,
        {
            entity->globalPosition.x,
            entity->globalPosition.y,
            rec.width*entity->globalScale.x,
            rec.height*entity->globalScale.y,
        },
        { 0,0 },
        entity->globalRotation.y,
        tint
    );
}

void AnimatedRenderer2D::Init(const char* path)
{
    texture = entity->engine->textureMgr->LoadTextureM(path);
}

void AnimatedRenderer2D::Init(TextureRes* tex)
{
    texture = tex;
}

void AnimatedRenderer2D::Unload()
{
    UnloadTexture(texture->tex);
}

void Renderer2D::Init(TextureRes* res)
{
    texture = res;
    tint = WHITE;
}

void Renderer2D::Unload()
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
void SceneMgr::Unload()
{
    UnloadCurrentScene();
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
            
            curScene->isActive = false;

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
    rlImGuiSetup(true);
    sceneMgr = new SceneMgr();
    physicsMgr = new PhysicsMgr();
    cameraMgr = new CameraMgr();
    textureMgr = new TextureMgr();
    lightMgr = new LightMgr();
    audioMgr = new AudioMgr();
    
    sceneMgr->Init(this);
    physicsMgr->Init();
    lightMgr->Init();
    audioMgr->Init();

}

void Engine::Update()
{
    physicsMgr->Update();
    lightMgr->Update();
    audioMgr->Update();
    sceneMgr->Update(cameraMgr);
}

void Engine::Unload()
{
    rlImGuiShutdown();
    lightMgr->Unload();
    sceneMgr->Unload();
    //audioMgr->Unload();
    delete physicsMgr->world;
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

void Scene::originalBegin2D(aurCamera* cam, EntityData* entityData)
{



    //printf("x: %.2f y: %.2f\n", cam->cam2D.target.x+cam->cam2D.offset.x, cam->cam2D.target.y+cam->cam2D.offset.y);
    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D((Camera2D)cam->cam2D);
    entityData->Update();
}

void Scene::originalBegin3D(aurCamera* cam, EntityData* entityData)
{
    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D((Camera3D)cam->cam3D);
    entityData->Update();
}

void Scene::originalEnd(bool use2D)
{
    if (use2D) {
        EndMode2D();
        //printf("2d");
    }
    DrawFPS(10, 10);
    
    EndDrawing();
}

void Scene::originalSetup()
{
}

void Scene::originalUnload()
{
}

void DrawDebugLine(b2Vec2 v1, b2Vec2 v2, Vector3 ps, Color col) {
    DrawLine(v1.x + ps.x, v1.y + ps.y, v2.x + ps.x, v2.y + ps.y, col);
}

void Rigidbody2D::Update(Entity* entity) {

    if (body->GetType() != b2_staticBody) {

        entity->localPosition = { -b2toRlVec2(body->GetPosition()).x + offset.x, -b2toRlVec2(body->GetPosition()).y + offset.y };
    }
    if (true == true) {

        if (body->GetFixtureList()->GetType() == b2Shape::e_polygon && body->IsEnabled()) {

            b2PolygonShape* shape = (b2PolygonShape*)body->GetFixtureList()->GetShape();

            Vector3 gp = { -body->GetPosition().x, -body->GetPosition().y };
            for (int i = 0; i < shape->m_count; i++) {
                if (i == shape->m_count - 1) {
                    if (body->GetType() == b2_staticBody) {

                        DrawDebugLine(shape->m_vertices[i], shape->m_vertices[0], gp, RED);
                    }
                    else {

                        DrawDebugLine(shape->m_vertices[i], shape->m_vertices[0], gp, GREEN);
                    }
                }
                else {
                    if (body->GetType() == b2_staticBody) {
                        DrawDebugLine(shape->m_vertices[i], shape->m_vertices[i+1], gp, RED);
                    }
                    else {
                        DrawDebugLine(shape->m_vertices[i], shape->m_vertices[i+1], gp, GREEN);
                    }
                }
            }
        }

        if (body->GetFixtureList()->GetType() == b2Shape::e_circle) {

            b2CircleShape* shape = (b2CircleShape*)body->GetFixtureList()->GetShape();

            b2Vec2 gp = body->GetPosition();

            //DrawCircle(-gp.x, -gp.y, shape->m_radius*100, GREEN);
        }

    }
}

void Rigidbody2D::Init(b2World* world, Entity* entity, b2Shape& shape, b2BodyType type, bool useLocal)
{
    b2BodyDef bodyDef;
    bodyDef.type = type;

    Vector3 pos = entity->GetGlobalPosition();
    Vector3 scl = entity->GetGlobalScale();

    Vector3 gp = Vector3Multiply(pos, scl);

    if (useLocal) {
        
        gp = entity->localPosition;
            
    }

    bodyDef.position.Set(-gp.x, -gp.y);
    body = world->CreateBody(&bodyDef);
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &shape;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.3f;

    

    if (type == b2_staticBody) {
        body->CreateFixture(&shape, 0);
    }
    else if (type == b2_dynamicBody) {
        printf("dynamic\n");
        body->CreateFixture(&fixtureDef);
    }
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

BoxCollider2D::BoxCollider2D() {
    shape = b2PolygonShape();
}

void BoxCollider2D::Update(Entity *Entity)
{
    for (int i = 0; i < sizeof(shape.m_vertices); i++) {
        Vector2 vert = {shape.m_vertices[i].x, shape.m_vertices[i].y};
        Vector2 nextVert;
        if (i == sizeof(shape.m_vertices)-1) {
            nextVert = {shape.m_vertices[0].x, shape.m_vertices[0].y};
        } else {
            nextVert = {shape.m_vertices[i+1].x, shape.m_vertices[i+1].y };
        }
        DrawLineV(vert, nextVert, GREEN);
    }
}

void BoxCollider2D::Init(b2World* world, float sx, float sy)
{
    shape.SetAsBox(sx/2, sx/2);
}

b2PolygonShape *BoxCollider2D::getShape()
{
    return &shape;
}

DebugDraw::~DebugDraw() noexcept
{
}

void DebugDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
    b2Vec2 p1 = vertices[vertexCount - 1];
    Color col = b2TorlColor(color);
    for (int32 i = 0; i < vertexCount; ++i)
    {
        b2Vec2 p2 = vertices[i];
        DrawLine(p1.x + 1000, p1.y + 1000, p2.x + 1000, p2.y + 1000, col);
        p1 = p2;
    }
}
void DebugDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
    b2Color fillColor(0.5f * color.r, 0.5f * color.g, 0.5f * color.b, 0.5f);

    for (int32 i = 1; i < vertexCount - 1; ++i)
    {

        Vector2 v1 = { vertices[0].x, vertices[0].y };
        Vector2 v2 = { vertices[i].x, vertices[i + 1].y };
        Vector2 v3 = { vertices[i].x, vertices[i + 1].y };

        DrawTriangle(v1, v2, v3, b2TorlColor(fillColor));
    }

    b2Vec2 p1 = vertices[vertexCount - 1];
    for (int32 i = 0; i < vertexCount; ++i)
    {
        b2Vec2 p2 = vertices[i];
        DrawLine(p1.x, p1.y, p2.x, p2.y, b2TorlColor(color));
        p1 = p2;
    }
}
void DebugDraw::DrawCircle(const b2Vec2& center, float radius, const b2Color& color) {
    DrawCircleLines(center.x, center.y, radius, b2TorlColor(color));
}
void DebugDraw::DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) {
    DrawCircleV({ center.x, center.y }, radius, b2TorlColor(color));
}
void DebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) {
    DrawLine(p1.x, p1.y, p2.x, p2.y, b2TorlColor(color));
}
void DebugDraw::DrawTransform(const b2Transform& xf) {
    const float k_axisScale = 0.4f;
    b2Color red(1.0f, 0.0f, 0.0f);
    b2Color green(0.0f, 1.0f, 0.0f);
    b2Vec2 p1 = xf.p, p2;

    p2 = p1 + k_axisScale * xf.q.GetXAxis();
    DrawLine(p1.x, p1.y, p2.x, p2.y, b2TorlColor(red));

    p2 = p1 + k_axisScale * xf.q.GetYAxis();
    DrawLine(p1.x, p1.y, p2.x, p2.y, b2TorlColor(red));
}
void DebugDraw::DrawPoint(const b2Vec2& p, float size, const b2Color& color) {
    DrawCircleV({ p.x, p.y }, size, b2TorlColor(color));
}

void PhysicsMgr::Init()
{
    gravity = b2Vec2(0,0);
    world = new b2World(gravity);
        
    DebugDraw fooDrawInstance = DebugDraw();

    fooDrawInstance.SetFlags(b2Draw::e_shapeBit);

    //in constructor, usually
    world->SetDebugDraw(&fooDrawInstance);

    //somewhere appropriat
}

void PhysicsMgr::Update()
{
    gravity = { 0,-500 };
    world->SetGravity(gravity);
    float timeStep = 1.0f / 60.0f;
    int32 velocityIterations = 8;
    int32 positionIterations = 3;
    world->Step(timeStep, velocityIterations, positionIterations);
    //world->DebugDraw();
}

Entity *EntityData::createEntity(const char* name)
{
    Entity* entity = new Entity();
    entity->componentHolder = new ComponentHolder();
    entity->localScale = { 1,1,1 };
    entity->engine = this->engine;
    entity->scene = this->scene;
    entity->name = name;
    entity->internalID = entities.size();
    entities.push_back(entity);
    return entity;
}

void EntityData::Update()
{

    for (Entity* entity : entities) {
        if (strcmp(entity->tag, "bg") == 0) {
            entity->Update();
        }
    }

    for (Entity* entity : entities) {
        if (strcmp(entity->tag, "ui") != 0 && strcmp(entity->tag, "bg") != 0) {
            entity->Update();
        }
    }

    for (Entity* entity : unloadedEntities) {
        entity->SetEnabled(false);
    }
}

void EntityData::FixedUpdate()
{
    for (Entity* entity : entities) {
        if (strcmp(entity->tag, "ui") != 0) {
            entity->FixedUpdate();
        }
    }
}

void EntityData::UI_Update()
{
    for (Entity* entity : entities) {
        if (strcmp(entity->tag, "ui") == 0) {
            entity->Update();
        }
    }
}

void EntityData::Unload()
{
    for (Entity* entity : entities) {
        entity->Unload();
    }
    for (Entity* entity : unloadedEntities) {
        entity->Unload();
    }
}

aurCamera* CameraMgr::loadCamera()
{
    aurCamera* camera = new aurCamera();
    delete currentCamera;
    currentCamera = camera;
    //camera->cam2D = cam;
    return camera;
}

aurCamera* CameraMgr::loadCamera(Camera2D cam)
{
    aurCamera* camera = new aurCamera();
    delete currentCamera;
    currentCamera = camera;
    camera->cam2D = cam;
    return camera;
}

aurCamera* CameraMgr::loadCamera(Camera3D cam)
{
    aurCamera* camera = new aurCamera();
    delete currentCamera;
    currentCamera = camera;
    camera->cam3D = cam;
    return camera;
}

PhysicsMgr::PhysicsMgr() {
    world = new b2World(gravity);
};

void aurCamera::Update()
{
    if (attachedEntity) {
        cam2D.target = { attachedEntity->globalPosition.x, attachedEntity->globalPosition.y };
        cam2D.offset = { entW / 2.0f, entH / 2.0f };
    }

}

TextureRes::TextureRes()
{
}

TextureRes::TextureRes(Texture2D texture)
{
    tex = texture;
}

void TextureRes::Update()
{
}

int InputManager::GetInput(InputAction action) {
    if (action == DPADUP) {
        return IsGamepadButtonDown(0, 1);
    }
    if (action == DPADDOWN) {
        return IsGamepadButtonDown(0, 3);
    }
    if (action == DPADLEFT) {
        return IsGamepadButtonDown(0, 4);
    }
    if (action == DPADRIGHT) {
        return IsGamepadButtonDown(0, 2);
    }
    if (action == LEFTTRIGGER) {
        return IsGamepadButtonDown(0, 10);
    }
    if (action == LEFTBUMBER) {
        return IsGamepadButtonDown(0, 9);
    }
    if (action == RIGHTTRIGGER) {
        return IsGamepadButtonDown(0, 12);
    }
    if (action == RIGHTBUMPER) {
        return IsGamepadButtonDown(0, 11);
    }
    if (action == BUTTONUP) {
        return IsGamepadButtonDown(0, 5);
    }
    if (action == BUTTONDOWN) {
        return IsGamepadButtonDown(0, 7);
    }
    if (action == BUTTONLEFT) {
        return IsGamepadButtonDown(0, 8);
    }
    if (action == BUTTONRIGHT) {
        return IsGamepadButtonDown(0, 6);
    }
    if (action == START) {
        return IsGamepadButtonDown(0, 13);
    }
    if (action == SELECT) {
        return IsGamepadButtonDown(0, 15);
    }
    if (action == LEFTSTICK) {
        return IsGamepadButtonDown(0, 16);
    }
    if (action == RIGHTSTICK) {
        return IsGamepadButtonDown(0, 17);
    }
}

bool InputManager::GetMouseDown(MouseButton mb)
{
    return IsMouseButtonDown(mb);
}

Vector2 InputManager::GetMousePosition()
{
    return GetMousePosition();
}



float InputManager::GetInputAxis(InputAction action) {
    if (action == LEFT || action == RIGHT || action == HORIZONTAL1) {
        return GetGamepadAxisMovement(0, 0);
    }
    if (action == UP || action == DOWN || action == VERTICAL1) {
        return GetGamepadAxisMovement(0, 1);
    }
    if (action == SECLEFT || action == SECRIGHT || action == HORIZONTAL2) {
        return GetGamepadAxisMovement(0, 2);
    }
    if (action == SECUP || action == SECDOWN || action == VERTICAL2) {
        return GetGamepadAxisMovement(0, 3);
    }
}

TextureRes* TextureMgr::LoadTextureM(const char* path)
{
    TextureRes* tex = new TextureRes(LoadTexture(path));

    textures.push_back(tex);

    return tex;
}

TextureRes* TextureMgr::LoadTextureM(Texture2D tex)
{
    TextureRes* text = new TextureRes(tex);

    textures.push_back(text);

    return text;
}

void TextureMgr::UnloadTexture(int tindex)
{
}

void TextureMgr::UnloadAll()
{
}