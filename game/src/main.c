#include "raylib.h"
#include "aurora.h"

int main(void)
{
    InitWindow(800, 450, "Egg");

    Scene scene = CreateScene();
    ModelRes model = LoadModelRes("resources/testmodel.obj");
    LoadModelToScene(model, scene);

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 10.0f, 10.0f };  // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera mode type

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

    

    while (!WindowShouldClose())
    {
        Draw3D(scene);
    }

    CloseWindow();

    return 0;
}