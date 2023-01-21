
#include "aurora.h"

#if !defined(MAX_MODEL_COUNT)
#define MAX_MODEL_COUNT 10000



#endif // MACRO

void Draw3D(Scene scene, Camera3D camera)
{
    BeginDrawing();

    BeginMode3D(camera);

    for (int i = 0; i < sizeof(scene.modelData->models); i++) {
        ModelRes model = scene.modelData->models[i];
        DrawModel(model.model, (Vector3){0,0,0}, 1.0f, WHITE);
    }

    EndMode3D();

    EndDrawing();
}
Scene CreateScene(){
    Scene scene = {0};

    scene.modelData = (ModelData *)RL_CALLOC(1, sizeof(ModelData));

    scene.modelData->modelCount = 0;

    scene.modelData->models = (ModelRes* )RL_CALLOC(MAX_MODEL_COUNT, sizeof(ModelRes));

    return scene;
}
ModelRes LoadModelRes(const char *path)
{

    ModelRes model = {0};

    model.model = LoadModel(path);

    return model;
}
void LoadModelToScene(ModelRes model, Scene scene){
    scene.modelData->models[scene.modelData->modelCount] = model;
};
