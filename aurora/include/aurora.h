#include "raylib.h"

typedef struct ModelRes {
    Model model;
} ModelRes;


typedef struct ModelData {
    ModelRes *models;
    int modelCount;
} ModelData;

typedef struct Scene {
    ModelData *modelData;
} Scene;

void Draw3D(Scene scene, Camera3D camera); // Draw 3D scene to window/screen

void LoadSceneFromJson(const char* path); // Get scene from JSON
Scene CreateScene(); // Create scene from scratch
ModelRes LoadModelRes(const char* path); // Load ModelRes from path
void LoadModelToScene(ModelRes model, Scene scene); // Load ModelRes into scene
