#include "raylibCpp/raylib-cpp.hpp"
#include "cJSON/cJSON.h"

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

class Entity {
public:
    Entity() = default;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
};

class EntityData {
public:
    EntityData() = default;
    std::vector<Entity*> entities;
    int entityCount;
};

class Scene {
public:
    Scene() = default;
    ModelData* modelData;
    EntityData* entityData;
};

void Draw3D(Scene scene, Camera3D camera); // Draw 3D scene to window/screen
void Draw3DCallback(Scene scene, Camera3D camera, void(*callback)()); // Draw 3D scene to window/screen (with callbacks)

void LoadSceneFromJson(const char* path); // Get scene from JSON
Scene CreateScene(); // Create scene from scratch
ModelRes LoadModelRes(const char* path); // Load ModelRes from path
void LoadModelToScene(ModelRes* model, Scene scene); // Load ModelRes into scene
void SaveSceneToJson(const char* path); // Save scene to JSON