
#include "../aurora.hpp"
//#include "aurora.hpp"

#if !defined(MAX_MODEL_COUNT)
#define MAX_MODEL_COUNT 10000
#endif // MACRO

#if !defined(MAX_ACTOR_COUNT)
#define MAX_ACTOR_COUNT 10000
#endif // MACRO

const std::size_t Component::Type = std::hash<const char*>()(TO_STRING(Component));

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
void SaveSceneToJson(const char *path){

}
Scene CreateScene(){
    Scene scene = Scene();

    scene.modelData = new ModelData();
    scene.entityData = new EntityData();

    //scene.modelData = (ModelData *)RL_CALLOC(1, sizeof(ModelData));

    //scene.modelData->modelCount = 0;

    //scene.modelData->models = (ModelRes* )RL_CALLOC(MAX_MODEL_COUNT, sizeof(ModelRes));

    //scene.entityData = (EntityData *)RL_CALLOC(1, sizeof(EntityData));

    //scene.entityData->entityCount = 0;

    //scene.entityData->entities = (Entity* )RL_CALLOC(MAX_ACTOR_COUNT, sizeof(Entity));

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

bool Component::IsClassType(const std::size_t classType) const {
    return classType == Type;
}

template <class CompType, typename... Args>
void ComponentHolder::Attach(Args&&... params) {
    components.emplace_back(std::make_unique< CompType >(std::forward< Args >(params)...));
}

void ComponentHolder::Update(Actor* actor) {
    for (auto const& component : components) {

        auto comp = component.get();

        comp->Update(actor);
    }
};

template< class ComponentType, typename... Args >
void Entity::AddComponent(Args&&... params) {
    componentHolder->components.emplace_back(std::make_unique< ComponentType >(std::forward< Args >(params)...));
}

template< class ComponentType >
ComponentType& Entity::GetComponent() {
    for (auto&& component : componentHolder->components) {
        if (component->IsClassType(ComponentType::Type))
            return *static_cast<ComponentType*>(component.get());
    }

    return *std::unique_ptr< ComponentType >(nullptr);
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