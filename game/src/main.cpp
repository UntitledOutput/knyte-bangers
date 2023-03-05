// THIS ALL WILL BE MADE INTO ONE HEADER FILE, JUST WAIT FOR ESSENTIAL CONTENT TO BE DONE

#include "raylib.h"
#include "rlgl.h"


#include "aurora/aurora.hpp"

void drawCallback() {
    DrawGrid(10,1.0f);
}

int main(int argc, char* argv[])
{


    InitWindow(1280, 720, "KnyteBangers");
    SetWindowState(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);

    Texture2D icon = LoadTexture("resources/icon/main.png");

    SetWindowIcon(LoadImageFromTexture(icon));

    InitAudioDevice();              // Initialize audio device
    
    engine = new Engine();
    engine->Init();
    
    Scene* scene = engine->sceneMgr->LoadScene<Boot>();
    //scene->scene_init();

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {

        engine->Update();
    }

    engine->Unload();
    CloseAudioDevice();

    CloseWindow();

    return 0;
}