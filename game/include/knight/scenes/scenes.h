
#if !defined(AURORA_IMPLEMENTATION)
#include "aurora/aurora.hpp"
#endif // AURORA_IMPLEMENTATION

class TestScene : public Scene {
public:
    void scene_init() {
        originalSetup();
        Entity* player = entityData->createEntity();
        Renderer2D* renderer = player->AddComponent<Renderer2D>();
        renderer->Init();
        renderer->texture = LoadTexture("resources/player.png");
        Camera2D camera = { 0 };
        camera.target = { 40 + 20.0f, 40 + 20.0f };
        camera.offset = { GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };
        camera.rotation = 0.0f;
        camera.zoom = 1.0f;
        aurCamera cam = engine->cameraMgr->loadCamera(camera);
    }

    void scene_update(aurCamera* cam) {
        originalBegin2D(cam);
        DrawCircle(GetScreenWidth()/2, GetScreenHeight()/2, 10, RED);
        originalEnd();
    }

    void scene_unload() {
        originalUnload();
    }

    bool is2D = true;
};