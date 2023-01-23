// THIS ALL WILL BE MADE INTO ONE HEADER FILE, JUST WAIT FOR ESSENTIAL CONTENT TO BE DONE

#include "raylib.h"
#include "rlgl.h"


#include "aurora/aurora.hpp"

void drawCallback() {
    DrawGrid(10,1.0f);
}

int main(void)
{
    InitWindow(1280, 720, "KnyteBangers");
    SetWindowState(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);

    InitAudioDevice();              // Initialize audio device

    
    Engine engine = Engine();
    engine.Init();
    
    TestScene* scene = engine.sceneMgr->LoadScene<TestScene>();
    scene->isActive = true;
    scene->scene_init();
        

    //Scene scene = CreateScene();
    //ModelRes model = LoadModelRes("resources/testmodel.fbx");
    //LoadModelToScene(&model, scene);
    //model.model.materials->maps[MATERIAL_MAP_ALBEDO].texture = LoadTexture("resources/mecha.png");

    Rectangle player = { 400, 280, 40, 40 };

    Camera2D camera = { 0 };
    camera.target = { player.x + 20.0f, player.y + 20.0f };
    camera.offset = { GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

    //SetCameraMode(camera, CAMERA_ORBITAL);  // Set a orbital camera mode    

    Music music = LoadMusicStream("resources/Cyborg Ninja.mp3");        // Load OGG audio file

    PlayMusicStream(music);

    // Initialize physics and default physics bodies
    b2Vec2 gravity(0.0f, -100.0f);
    b2World world(gravity);

    // create ground
    b2BodyDef groundBodyDef;
    groundBodyDef.position.Set(0.0f, -10.0f);
    b2Body* groundBody = world.CreateBody(&groundBodyDef);
    b2PolygonShape groundBox;
    groundBox.SetAsBox(50.0f, 10.0f);
    groundBody->CreateFixture(&groundBox, 0.0f);

    // create player body
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(0.0f, 100.0f);
    b2Body* body = world.CreateBody(&bodyDef);
    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(1.0f, 1.0f);
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &dynamicBox;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.3f;
    body->CreateFixture(&fixtureDef);

    // simulation pt1
    float timeStep = 1.0f / 60.0f;
    int32 velocityIterations = 6;
    int32 positionIterations = 2;

    float movementMultiplier = 0;

    while (!WindowShouldClose())
    {

        engine.Update();

                //----------------------------------------------------------------------------------
        UpdateMusicStream(music);   // Update music buffer with new stream datap
        SetMusicVolume(music, 0.0f);
        

        //UpdateCamera(&camera);

        movementMultiplier = 0;

        world.Step(timeStep, velocityIterations, positionIterations);
        b2Vec2 position = body->GetPosition();
        float angle = body->GetAngle();

        player.x = position.x;
        player.y = -position.y;

        

        //printf("%4.2f %4.2f %4.2f\n", position.x, position.y, angle);

        //printf("%.2f\n", GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X));

        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)|| GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) > 0.0f) movementMultiplier = 1;
        else if (IsKeyDown(KEY_LEFT)|| IsKeyDown(KEY_A)|| GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) < 0.0f) movementMultiplier = -1;
        //if (IsKeyDown(KEY_UP)|| IsKeyDown(KEY_W)|| GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y) < 0.0f) player.y -= 2;
        //else if (IsKeyDown(KEY_DOWN)|| IsKeyDown(KEY_S)|| GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y) > 0.0f) player.y += 2;
        body->SetLinearVelocity({(50*movementMultiplier),body->GetLinearVelocity().y});

        camera.target = { player.x + 20, player.y + 20 };

        /*
        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        DrawRectangle(-25,40,100,20,BLUE);

        //rlPushMatrix();
        //rlRotatef(angle,0,1,0);
        DrawRectangleRec(player, RED);
        //rlPopMatrix();
        EndMode2D();

        //DrawText(TextFormat("%.2f, %.2f", player.x, player.y), 250,250,31,BLACK);

        EndDrawing();
        */

        //Draw3DCallback(scene, camera, drawCallback);
    }

    UnloadMusicStream(music);
    CloseAudioDevice();

    CloseWindow();

    return 0;
}