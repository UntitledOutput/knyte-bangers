
#include "../aurora.hpp"
//#include "aurora.hpp"

#if !defined(MAX_MODEL_COUNT)
#define MAX_MODEL_COUNT 10000
#endif // MACRO

#if !defined(MAX_ACTOR_COUNT)
#define MAX_ACTOR_COUNT 10000
#endif // MACRO

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