

// main renderer
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#define NO_FONT_AWESOME

#include "rlImGui.h"	// include the API header
#include "rlImGuiColors.h"
#include "imgui.h"

#define MADE_WITH_RAYLIB_IMPLEMENTATION

#include "madewithraylib.h"
//#include "imgui/misc/cpp/imgui_stdlib.h"

#include "discord/discord_rpc.h"
#include "discord/discord_register.h"

static bool gInit, gRPC = true;

// utilities
#include "cJSON/cJSON.h"
#include "PerlinNoise.hpp"
#include "network/anet.h"

// required headers
#include <typeinfo> 
#include <memory>
#include <algorithm>
#include <map>
#include <vector>
#include <string>
#include <filesystem>
#include <chrono>//We will use chrono for the elapsed time.

// physics
#include "box2d/box2d.h"


#if !defined(VISUALIZE2DBOUNDS)
#define VISUALIZE2DBOUNDS false
#endif // VISUALIZE2DBOUNDS

#if !defined(SHOWCONSOLE)
#define SHOWCONSOLE false
#endif // SHOWCONSOLE

Camera3D Load3DCamera(float fovy, Vector3 position, CameraProjection projection, Vector3 target, Vector3 up);
Camera2D Load2DCamera(Vector2 offset, float rotation, Vector2 target, float zoom);

class Entity;
class Engine;

static const char* selectedName;

#define TO_STRING( x ) #x

//****************
#define CLASS_DECLARATION( classname )                                                      \
public:                                                                                     \
    static const std::size_t Type;                                                          \
    virtual bool IsClassType( const std::size_t classType ) const override;                 \

//****************
// CLASS_DEFINITION
// 
// This macro must be included in the class definition to properly initialize 
// variables used in type checking. Take special care to ensure that the 
// proper parentclass is indicated or the run-time type information will be
// incorrect. Only works on single-inheritance RTTI.
//****************
#define CLASS_DEFINITION( parentclass, childclass )                                         \
const std::size_t childclass::Type = std::hash< const char* >()( TO_STRING( childclass ) ); \
bool childclass::IsClassType( const std::size_t classType ) const {                         \
        if ( classType == childclass::Type )                                                \
            return true;                                                                    \
        return parentclass::IsClassType( classType );                                       \
}                                                                                           \

class Engine;
class Player;
class EntityData;

namespace Game {
    namespace UI {
        namespace Devel {

            static Entity* m_selectionContext;

            void DrawConsole(Camera2D* cam, bool* lock, b2World* world, Engine* engine, Player* player, EntityData* entityData, b2Vec2 playerPos = b2Vec2_zero);
            void DrawEntityNode(Entity* entity);
        }
    }
};

namespace ML {
    namespace UI {

        static void DrawTextBoxedSelectable(Font font, const char* text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint, int selectStart, int selectLength, Color selectTint, Color selectBackTint)
        {
            int length = TextLength(text);  // Total length in bytes of the text, scanned by codepoints in loop

            float textOffsetY = 0;          // Offset between lines (on line break '\n')
            float textOffsetX = 0.0f;       // Offset X to next character to draw

            float scaleFactor = fontSize / (float)font.baseSize;     // Character rectangle scaling factor

            // Word/character wrapping mechanism variables
            enum { MEASURE_STATE = 0, DRAW_STATE = 1 };
            int state = wordWrap ? MEASURE_STATE : DRAW_STATE;

            int startLine = -1;         // Index where to begin drawing (where a line begins)
            int endLine = -1;           // Index where to stop drawing (where a line ends)
            int lastk = -1;             // Holds last value of the character position

            for (int i = 0, k = 0; i < length; i++, k++)
            {
                // Get next codepoint from byte string and glyph index in font
                int codepointByteCount = 0;
                int codepoint = GetCodepoint(&text[i], &codepointByteCount);
                int index = GetGlyphIndex(font, codepoint);

                // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
                // but we need to draw all of the bad bytes using the '?' symbol moving one byte
                if (codepoint == 0x3f) codepointByteCount = 1;
                i += (codepointByteCount - 1);

                float glyphWidth = 0;
                if (codepoint != '\n')
                {
                    glyphWidth = (font.glyphs[index].advanceX == 0) ? font.recs[index].width * scaleFactor : font.glyphs[index].advanceX * scaleFactor;

                    if (i + 1 < length) glyphWidth = glyphWidth + spacing;
                }

                // NOTE: When wordWrap is ON we first measure how much of the text we can draw before going outside of the rec container
                // We store this info in startLine and endLine, then we change states, draw the text between those two variables
                // and change states again and again recursively until the end of the text (or until we get outside of the container).
                // When wordWrap is OFF we don't need the measure state so we go to the drawing state immediately
                // and begin drawing on the next line before we can get outside the container.
                if (state == MEASURE_STATE)
                {
                    // TODO: There are multiple types of spaces in UNICODE, maybe it's a good idea to add support for more
                    // Ref: http://jkorpela.fi/chars/spaces.html
                    if ((codepoint == ' ') || (codepoint == '\t') || (codepoint == '\n')) endLine = i;

                    if ((textOffsetX + glyphWidth) > rec.width)
                    {
                        endLine = (endLine < 1) ? i : endLine;
                        if (i == endLine) endLine -= codepointByteCount;
                        if ((startLine + codepointByteCount) == endLine) endLine = (i - codepointByteCount);

                        state = !state;
                    }
                    else if ((i + 1) == length)
                    {
                        endLine = i;
                        state = !state;
                    }
                    else if (codepoint == '\n') state = !state;

                    if (state == DRAW_STATE)
                    {
                        textOffsetX = 0;
                        i = startLine;
                        glyphWidth = 0;

                        // Save character position when we switch states
                        int tmp = lastk;
                        lastk = k - 1;
                        k = tmp;
                    }
                }
                else
                {
                    if (codepoint == '\n')
                    {
                        if (!wordWrap)
                        {
                            textOffsetY += (font.baseSize + font.baseSize / 2) * scaleFactor;
                            textOffsetX = 0;
                        }
                    }
                    else
                    {
                        if (!wordWrap && ((textOffsetX + glyphWidth) > rec.width))
                        {
                            textOffsetY += (font.baseSize + font.baseSize / 2) * scaleFactor;
                            textOffsetX = 0;
                        }

                        // When text overflows rectangle height limit, just stop drawing
                        if ((textOffsetY + font.baseSize * scaleFactor) > rec.height) break;

                        // Draw selection background
                        bool isGlyphSelected = false;
                        if ((selectStart >= 0) && (k >= selectStart) && (k < (selectStart + selectLength)))
                        {
                            DrawRectangleRec({ rec.x + textOffsetX - 1, rec.y + textOffsetY, glyphWidth, (float)font.baseSize * scaleFactor }, selectBackTint);
                            isGlyphSelected = true;
                        }

                        // Draw current character glyph
                        if ((codepoint != ' ') && (codepoint != '\t'))
                        {
                            DrawTextCodepoint(font, codepoint, { rec.x + textOffsetX, rec.y + textOffsetY }, fontSize, isGlyphSelected ? selectTint : tint);
                        }
                    }

                    if (wordWrap && (i == endLine))
                    {
                        textOffsetY += (font.baseSize + font.baseSize / 2) * scaleFactor;
                        textOffsetX = 0;
                        startLine = endLine;
                        endLine = -1;
                        glyphWidth = 0;
                        selectStart += lastk - k;
                        k = lastk;

                        state = !state;
                    }
                }

                if ((textOffsetX != 0) || (codepoint != ' ')) textOffsetX += glyphWidth;  // avoid leading spaces
            }
        }

        static void DrawTextBoxed(Font font, const char* text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint)
        {
            DrawTextBoxedSelectable(font, text, rec, fontSize, spacing, wordWrap, tint, 0, 0, WHITE, WHITE);
        }

        struct UIElement {
            Vector2 position;
            float rotation;
            virtual void Update();
            virtual void Draw();
            Color color;
        };

        struct TextButton : public UIElement {
            Rectangle rect;
            bool mouseDown, mouseHover, mouseIn, rounded, lined, wrapWords;
            const char* text;
            float lineThickness, roundness, segments, fontSize, spacing;
            Font font;
            void Update() {
                rect.x = position.x;
                rect.y = position.y;
                if (CheckCollisionPointRec(GetMousePosition(), rect)) {
                    mouseIn = true;
                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                        mouseDown = true;
                    }
                    if (IsMouseButtonUp(MOUSE_LEFT_BUTTON)) {
                        mouseHover = true;
                    }
                }
                else {
                    mouseDown = false;
                    mouseHover = false;
                    mouseIn = false;
                }
            }
            void Draw() {
                if (rounded) {
                    if (lined) {
                        DrawRectangleRoundedLines(rect, roundness, segments, lineThickness, color);
                    }
                    else {
                        DrawRectangleRounded(rect, roundness, segments, color);
                    }
                }
                else {
                    if (lined) {
                        DrawRectangleLinesEx(rect, lineThickness, color);
                    }
                    else {
                        DrawRectangleRec(rect, color);
                    }
                }
                DrawTextBoxed(font, text, rect, fontSize, spacing, wrapWords, color);
            }
        };

        struct Text : public UIElement {
            const char* text;
            Font font;
            float fontSize, spacing;
            bool wrapWords;
            void Draw() {
                DrawTextEx(font, text, position, fontSize, spacing, color);
            }
        };

        class UIMgr {
        public:

            std::vector<UIElement*> ui;

            void Update() {
                for (UIElement* element : ui) {
                    element->Update();
                }
            }

            void Draw() {
                for (UIElement* element : ui) {
                    element->Draw();
                }
            }



        };
    };
};

inline void SetupDiscord()
{
    if (gRPC)
    {
        DiscordEventHandlers handlers;
        memset(&handlers, 0, sizeof(handlers));
        Discord_Initialize("1076660278118322289", &handlers, 1, "0");
    }
    else
    {
        Discord_Shutdown();
    }
}

inline static void UpdateDiscord(const char* sceneName, const char* internalSceneName)
{
    static int64_t StartTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    if (gRPC)
    {
        DiscordRichPresence discordPresence;
        memset(&discordPresence, 0, sizeof(discordPresence));
        discordPresence.state = "Playing";
        discordPresence.details = "";
        discordPresence.startTimestamp = StartTime;
        discordPresence.endTimestamp = NULL;
        discordPresence.largeImageKey = internalSceneName;
        discordPresence.largeImageText = sceneName;
        discordPresence.smallImageKey = "dev";
        discordPresence.smallImageText = "Developer Mode";
        discordPresence.instance = 1;

        Discord_UpdatePresence(&discordPresence);
    }
    else
    {
        Discord_ClearPresence();
    }
}

    // Input definitions

typedef enum InputAction {
    LEFT = 0,
    RIGHT,
    UP,
    DOWN,
    SECLEFT,
    SECRIGHT,
    SECUP,
    SECDOWN,
    MOTION,
    HORIZONTAL1,
    HORIZONTAL2,
    VERTICAL1,
    VERTICAL2,
    DPADUP,
    DPADDOWN,
    DPADLEFT,
    DPADRIGHT,
    START,
    SELECT,
    LEFTTRIGGER,
    RIGHTTRIGGER,
    LEFTBUMBER,
    RIGHTBUMPER,
    BUTTONUP,
    BUTTONDOWN,
    BUTTONLEFT,
    BUTTONRIGHT,
    LEFTSTICK,
    RIGHTSTICK,
};

class InputManager {
public:
    static int GetInput(InputAction action);
    static float GetInputAxis(InputAction action);
    static bool GetMouseDown(MouseButton mb);
    static Vector2 GetMousePosition();
};

class TextureRes {
public:

    TextureRes();

    TextureRes(Texture2D texture);

public:

     Texture2D tex;

    virtual void Update();
};


class aurCamera {
public:
    aurCamera() = default;

    Camera2D cam2D;
    Camera3D cam3D;

    Vector3 targetOffset3D;
    Vector2 targetOffset2D;

    bool useCam2D;

    inline void Attach(Entity* entity, float width, float height) {
        entW = width;
        entH = height;
        attachedEntity = entity;
        printf("%.2f, %.2f\n", entW, entH);
    };

    void Update();

private:
    friend class Scene;

    Entity* attachedEntity;
    float entW;
    float entH;
};

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


// You must free the result if result is non-NULL.
inline char* str_replace(char* orig, char* rep, char* with) {
    char* result; // the return string
    char* ins;    // the next insert point
    char* tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = (char*)malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

class Component {
public:

    static const std::size_t                    Type;
    virtual bool                                IsClassType(const std::size_t classType) const {
        return classType == Type;
    }
public:

    virtual                                ~Component() = default;
    Component(std::string&& initialValue)
        : value(initialValue) {
    }

    Component() = default;

    virtual void imgui_properties() {};

    inline virtual const char* GetName() {
        return typeid(*this).name();
    }

public:
    std::string                             value = "uninitialized";
private:
    friend class ComponentHolder;
    friend class Entity;

    virtual void Update(Entity* Entity);
    virtual void Unload();
    virtual void SetEnabled(bool enb);
    virtual void FixedUpdate(Entity* entity);

public:
    Entity* entity;
    bool enabled = true;
};

class ComponentHolder {
private:

    friend class Entity;
    friend class EntityData;

    ComponentHolder() = default;
public:
    std::vector< std::unique_ptr<Component> > components;
    template <class CompType, typename... Args>
    void Attach(Args&&... params);

    void Update(Entity* Entity);
    void FixedUpdate(Entity* entity);

    inline void SetEnabled(bool tf) {
        for (auto const& component : components) {

            auto comp = component.get();

            comp->enabled = tf;
        }
    }

    void Unload();
};

class Engine;
class Scene;

class Entity {
public:

    Vector3 localPosition;
    Vector3 localRotation;
    Vector3 localScale;

    Vector3 globalPosition;
    Vector3 globalRotation;
    Vector3 globalScale;

    ComponentHolder* componentHolder;

    Engine* engine;
    Scene* scene;

    const char* name;
    bool enabled = true;

    int internalID;

    template< class ComponentType, typename... Args >
    inline ComponentType* AddComponent(Args&&... params) {

        componentHolder->components.emplace_back(std::make_unique< ComponentType >(std::forward< Args >(params)...));

        for (auto&& component : componentHolder->components) {
            if (component->IsClassType(ComponentType::Type)) {
                ComponentType* comp = static_cast<ComponentType*>(component.get());
                comp->entity = this;
                return comp;
            }
        }

        return static_cast<ComponentType*>(nullptr);
    }

    template< class ComponentType >
    inline ComponentType* GetComponent() {
        for (auto&& component : componentHolder->components) {
            if (component->IsClassType(ComponentType::Type))
                return static_cast<ComponentType*>(component.get());
        }

        return static_cast<ComponentType*>(nullptr);
    }

    template< class ComponentType >
    bool RemoveComponent();

    template< class ComponentType >
    std::vector< ComponentType* > GetComponents();

    template< class ComponentType >
    int RemoveComponents();

    virtual void Update();
    virtual void FixedUpdate();

    const char* tag = "none";

    Entity* parent;

    inline void AddChild(Entity* entity) {
        entity->parent = this;

        globalPosition = GetGlobalPosition();

        Vector3 absRot = entity->localRotation;
        for (Entity* p = entity->parent; p != nullptr; p = p->parent)
            absRot = Vector3Add(absRot, p->localRotation);
        globalRotation = absRot;

        globalScale = GetGlobalScale();

        children.push_back(entity);
    }

    inline Entity* GetChild(int index) {
        return children.at(index);
    }

    std::vector<Entity*> children;

    void Unload();

    bool operator==(const Entity& other) const {
        return internalID == other.internalID && scene == other.scene; 
    };

    bool operator!=(const Entity& other) const {
        return !(*this == other);
    }

    inline void imgui_iterate()
    {
        std::vector<Entity*>::const_iterator itr;

        for (itr = children.begin(); itr != children.end(); ++itr)
        {
            // do stuff  breadth-first

            //ImGui::TreePush((void*)(*itr)->internalID);
            Game::UI::Devel::DrawEntityNode((*itr));
            // do stuff  depth-first
        }
    }


    inline Vector3 GetGlobalPosition() {
        globalPosition = this->localPosition;
        for (Entity* p = this->parent; p != nullptr; p = p->parent)
            globalPosition = Vector3Add(globalPosition, p->localPosition);
        return globalPosition;
    }

    inline Vector3 GetGlobalScale() {
        globalScale = this->localScale;
        for (Entity* p = this->parent; p != nullptr; p = p->parent)
            globalScale = Vector3Multiply(globalScale, p->localScale);
        return globalScale;
    }

    void SetEnabled(bool tf) {
        if (enabled) {
            componentHolder->SetEnabled(tf);
        }
    }

    void Destroy();

private:
    friend class EntityData;

    Entity() = default;

};

class Renderer2D : public Component {
    CLASS_DECLARATION( Renderer2D )
public:
    Renderer2D(std::string&& initialValue)
        : Component(std::move(initialValue)) {
    }

    Renderer2D() = default;

    TextureRes* texture;

    void Update(Entity* Entity);
    void Init(TextureRes* res);
    void Unload();
    void imgui_properties() {
        ImGui::Text("Texture"); ImGui::SameLine();

        bool tbp = (imgui_textureButtonPressed ? rlImGuiImageButtonSize("texture", &(texture->tex), { 100, 100 }) : rlImGuiImageButtonSize("texture", &(texture->tex), { 10, 10 }));

        if (tbp) {
            imgui_textureButtonPressed = !imgui_textureButtonPressed;
        }

        ImGui::NewLine();

        float color[4];

        color[0] = tint.r;
        color[1] = tint.g;
        color[2] = tint.b;
        color[3] = tint.a;

        ImGui::Text("Tint"); ImGui::SameLine();
        ImGui::ColorEdit4("##", color);

        tint = { (unsigned char)color[0], (unsigned char)color[1], (unsigned char)color[2], (unsigned char)color[3] };

        ImGui::NewLine();

        ImGui::Text("Rectangle");
        ImGui::InputFloat("Rectangle X", &rec.x);
        ImGui::InputFloat("Rectangle Y", &rec.y);
        ImGui::InputFloat("Rectangle Width", &rec.width);
        ImGui::InputFloat("Rectangle Height", &rec.height);

        ImGui::NewLine();

    };
    Rectangle rec;
    Color tint = WHITE;
private:

    bool imgui_textureButtonPressed;

};

class AnimatedRenderer2D : public Renderer2D {
    CLASS_DECLARATION( AnimatedRenderer2D )
public:
    AnimatedRenderer2D(std::string&& initialValue)
        : Renderer2D(std::move(initialValue)) {
    }

    AnimatedRenderer2D() = default;

    TextureRes* texture;
    Rectangle rec;
    Rectangle srcRect;
    Color tint = WHITE;

    void Update(Entity* Entity);
    void Init(const char* path);
    void Init(TextureRes* tex);
    void Unload();

    void imgui_properties() {
        ImGui::Text("Texture"); ImGui::SameLine();

        bool tbp = (imgui_textureButtonPressed ? rlImGuiImageButtonSize("texture", &(texture->tex), { 100, 100 }) : rlImGuiImageButtonSize("texture", &(texture->tex), { 10, 10 }));

        if (tbp) {
            imgui_textureButtonPressed = !imgui_textureButtonPressed;
        }

        ImGui::NewLine();

        float color[4];

        color[0] = tint.r;
        color[1] = tint.g;
        color[2] = tint.b;
        color[3] = tint.a;

        ImGui::Text("Tint"); ImGui::SameLine();
        ImGui::ColorEdit4("##", color);

        tint.r = color[0];
        tint.g = color[1];
        tint.b = color[2];
        tint.a = 255;

        ImGui::NewLine();

        ImGui::Text("Rectangle");
        ImGui::DragFloat("Rectangle X", &rec.x);
        ImGui::DragFloat("Rectangle Y", &rec.y);
        ImGui::DragFloat("Rectangle Width", &rec.width);
        ImGui::DragFloat("Rectangle Height", &rec.height);

        ImGui::NewLine();

        ImGui::Text("Source Rectangle");
        ImGui::DragFloat("Rectangle X", &srcRect.x);
        ImGui::DragFloat("Rectangle Y", &srcRect.y);
        ImGui::DragFloat("Rectangle Width", &srcRect.width);
        ImGui::DragFloat("Rectangle Height", &srcRect.height);

        ImGui::NewLine();

    };

    //Rectangle rec;
private:
    bool imgui_textureButtonPressed;
};


class Actor2D : public Entity {
public:
    Actor2D() = default;
    Renderer2D* renderer;

    void Init();

    void Update();
};

class EntityData {
public:
    EntityData() = default;
    std::vector<Entity*> entities;
    std::vector<Entity*> unloadedEntities;
    Entity* createEntity(const char* name);
    inline Entity* find(const char* name) {
        for (Entity* entity : entities) {
            if (strcmp(entity->name, name) == 0) {
                return entity;
                break;
            }
        }
    };

    inline void DestroyEntity(Entity* ent) {
        ent->enabled = false;
        entityCount -= 1;

        int deleteInd = ent->internalID;

        for (Entity* entity : entities) {
            if (entity->internalID > deleteInd) {
                entity->internalID -= 1;
            }
        }

        for (int i = 0; i < entities.size(); i++)
        {
            if (entities[i]->internalID == deleteInd)
            {
                entities.erase(entities.begin() + i);
                break;
            }
        }

        ent->SetEnabled(false);

        unloadedEntities.push_back(ent);

    }

    int entityCount;
    void Update();
    void FixedUpdate();
    void UI_Update();
    void Unload();
    Engine* engine;
    Scene* scene;
};

class Scene {
public:
    Scene() = default;
    ModelData* modelData;
    EntityData* entityData;
    ML::UI::UIMgr uiMgr;
    virtual void scene_init();
    virtual void scene_update(aurCamera* cam);
    virtual void scene_unload();
    static void originalBegin2D(aurCamera* cam, EntityData* entityData);
    static void originalBegin3D(aurCamera* cam, EntityData* entityData);
    static void originalEnd(bool use2D);
    static void originalSetup();
    static void originalUnload();
    bool is2D;
    bool useRenderTexture;
    Engine* engine;
    bool isActive;
    int sceneIndex;
private:
    friend class SceneMgr;
    RenderTexture2D renderTexture;
};

class CameraMgr;

class SceneMgr {
public:
    void Init(Engine* eng);
    void Update(CameraMgr* camMgr);
    void Unload();

    //void LoadScene(const char* path);

    template<class SceneType>
    inline SceneType* LoadScene()
    {
        scenes.emplace_back(std::make_unique< SceneType >());

        SceneType* scene = static_cast<SceneType*>(scenes.back().get());

        scene->entityData = new EntityData();
        scene->entityData->engine = engine;
        scene->entityData->scene = scene;
        scene->engine = engine;
        scene->sceneIndex = scenes.size()-1;
        UnloadCurrentScene();
        scene->isActive = true;
        scene->scene_init();
        //currentScene = scene;
        return scene;
    }
    
    void UnloadCurrentScene();

    int getSceneCount();
    void SetCurrentScene(int position);

private:

    Engine* engine;

    //Scene currentScene;

    std::vector<std::unique_ptr<Scene>> scenes;
};

class Collider2D : public Component {
    CLASS_DECLARATION( Collider2D )
public:
    Collider2D(std::string&& initialValue)
        : Component(std::move(initialValue)) {
    }

    Collider2D() = default;

    void Update(Entity* Entity);
    void Init(b2World world);

    b2Shape* getShape();
private:
    b2Shape* shape;
};

class BoxCollider2D : public Collider2D {
    CLASS_DECLARATION( BoxCollider2D )
public:
    BoxCollider2D(std::string&& initialValue)
        : Collider2D(std::move(initialValue)) {
    }

    BoxCollider2D();

    void Update(Entity* Entity);
    void Init(b2World* world, float sx, float sy);

    b2PolygonShape* getShape();
private:
    b2PolygonShape shape;
};

class Rigidbody2D : public Component {
    CLASS_DECLARATION( Rigidbody2D )
public:
    Rigidbody2D(std::string&& initialValue)
        : Component(std::move(initialValue)) {
    }

    Rigidbody2D() = default;

    b2Body* body;

    void SetEnabled(bool boo) {
        body->SetEnabled(boo);
    }

    Vector2 size = {1,1};
    Vector2 offset;

    void Update(Entity* Entity);
    void Init(b2World* world, Entity* entity, b2Shape& shape, b2BodyType type, bool useLocal = false);

    inline void imgui_properties() {
        ImGui::Text("Position: %.2f, %.2f", body->GetPosition().x, body->GetPosition().y);
    }

private:
};

inline Color b2TorlColor(const b2Color& color) {
    return { (unsigned char)(color.r * 255),(unsigned char)(color.g * 255),(unsigned char)(color.b * 255),(unsigned char)(color.a * 255) };
}

class DebugDraw : public b2Draw
{
public:

    DebugDraw() = default;

    DebugDraw(DebugDraw const&) = delete;
    DebugDraw& operator=(DebugDraw const&) = delete;

    DebugDraw(DebugDraw&&) = default;
    DebugDraw& operator=(DebugDraw&&) = default;

    virtual ~DebugDraw() noexcept override;

    virtual void DrawPolygon(
        b2Vec2 const* pVertices,
        int32 vertexCount,
        b2Color const& colour
    ) override;

    virtual void DrawSolidPolygon(
        b2Vec2 const* pVertices,
        int32 vertexCount,
        b2Color const& colour
    ) override;

    virtual void DrawCircle(
        b2Vec2 const& centre,
        float radius,
        b2Color const& colour
    );

    virtual void DrawSolidCircle(
        b2Vec2 const& centre,
        float radius,
        b2Vec2 const& axis,
        b2Color const& colour
    ) override;

    virtual void DrawSegment(
        b2Vec2 const& begin,
        b2Vec2 const& end,
        b2Color const& colour
    ) override;

    virtual void DrawPoint(
        b2Vec2 const& point,
        float size,
        b2Color const& colour
    ) override;

    virtual void DrawTransform(b2Transform const& xf) override;
};

class PhysicsMgr {
public:
    PhysicsMgr();

    void Init();
    void Update();

    b2Vec2 gravity;
    b2World* world;
};

class CameraMgr {
public:
    CameraMgr() = default;

    aurCamera* loadCamera();
    aurCamera* loadCamera(Camera2D cam);
    aurCamera* loadCamera(Camera3D cam);

    aurCamera* currentCamera;
};

class TextureMgr {
public:
    // Load a texture
    TextureRes* LoadTextureM(const char* path);

    // Load a texture
    TextureRes* LoadTextureM(Texture2D tex);

    // Unload a texture.
    void UnloadTexture(int tindex);

    void UnloadAll();

    std::vector<TextureRes*> textures;

};

struct Light {
    Vector3 position;
    Color color;
    float radius;
    float power;

    // Shader locations
    unsigned int positionLoc;
    unsigned int innerLoc;
    unsigned int radiusLoc;
};

class LightMgr {
public:
    RenderTexture renderTexture;
    aurCamera* cam;

    std::vector<Light*> lights;

    inline void Init() {
        renderTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

        BeginTextureMode(renderTexture);

        ClearBackground(WHITE);

        EndTextureMode();

    }

    inline void Update() {
        if (cam) {
            BeginTextureMode(renderTexture);

            ClearBackground(WHITE);

            BeginMode2D((Camera2D)cam->cam2D);

            for (Light* light : lights) {
                DrawCircle(light->position.x, light->position.y, light->radius, light->color);
            }

            EndMode2D();

            EndTextureMode();
        }
    }

    inline void Unload() {
        UnloadRenderTexture(renderTexture);
    }


    inline void AddLight(Vector3 position, Color color, float radius, float power) {
        Light* light = new Light();
        
        light->position = position;
        light->color = color;
        light->radius = radius;
        light->power = power;

        lights.push_back(light);
    }

};

enum AudioType
{
    SoundType = 0,
    MusicType,
};

struct Audio {
    Sound sound;
    Music music;
    AudioType type;
};

class AudioMgr {
public:

    void Init() {
        
    }

    void LoadAudio(const char* path, AudioType type, const char* name) {
        Audio audio;
        if (type == SoundType) {
            audio.sound = LoadSound(path);
        }
        else {
            audio.music = LoadMusicStream(path);
        }
        audio.type = type;

        audios.push_back({name, &audio});
    }

    std::vector<std::pair<const char*, Audio*>> audios;

    void PlayAudio(const char* name) {
        Audio* currentAudio;

        for (std::pair<const char*, Audio*> audio : audios) {
            if (strcmp(audio.first, name) == 0) {
                currentAudio = audio.second;
            }
        }

        if (currentAudio->type == SoundType) {
            PlaySoundMulti(currentAudio->sound);
        }
        else {
            currentMusic = currentAudio->music;
            PlayMusicStream(currentAudio->music);
        }

    }

    void PauseAudio(const char* name) {
        Audio* currentAudio;

        for (std::pair<const char*, Audio*> audio : audios) {
            if (strcmp(audio.first, name) == 0) {
                currentAudio = audio.second;
            }
        }

        if (currentAudio->type == SoundType) {
            PauseSound(currentAudio->sound);
        }
        else {
            PauseMusicStream(currentAudio->music);
        }

    }

    void StopAudio(const char* name) {
        Audio* currentAudio;

        for (std::pair<const char*, Audio*> audio : audios) {
            if (strcmp(audio.first, name) == 0) {
                currentAudio = audio.second;
            }
        }

        if (currentAudio->type == SoundType) {
            StopSound(currentAudio->sound);
        }
        else {
            StopMusicStream(currentAudio->music);
        }

    }

    void Unload() {
        for (std::pair<const char*, Audio*> audio : audios) {
            if (audio.second->type == SoundType) {
                UnloadSound(audio.second->sound);
            }
            else {
                UnloadMusicStream(audio.second->music);
            }
        }
    }

    Music currentMusic;

    void Update() {
        UpdateMusicStream(currentMusic);
    }
};

class Engine {
public:
    Engine() = default;

    SceneMgr* sceneMgr;
    PhysicsMgr* physicsMgr;
    CameraMgr* cameraMgr;
    TextureMgr* textureMgr;
    LightMgr* lightMgr;
    AudioMgr* audioMgr;

    void Init();
    void Update();
    void Unload();
};

inline void Entity::Destroy() {
    scene->entityData->DestroyEntity(this);
}

static Engine* engine;

void Draw3D(Scene scene, Camera3D camera); // Draw 3D scene to window/screen
void Draw3DCallback(Scene scene, Camera3D camera, void(*callback)()); // Draw 3D scene to window/screen (with callbacks)

void LoadSceneFromJson(const char* path); // Get scene from JSON
Scene CreateScene(); // Create scene from scratch
ModelRes LoadModelRes(const char* path); // Load ModelRes from path
void LoadModelToScene(ModelRes* model, Scene scene); // Load ModelRes into scene
void SaveSceneToJson(const char* path); // Save scene to JSON

// KNIGHTS IMPLEMENTATION

class TerrainGenerator;
class Tile;

class Player : public Component {
    CLASS_DECLARATION(Player)
public:
    Player(std::string&& initialValue)
        : Component(std::move(initialValue)) {
    }

    Player() = default;

    float horizontalAxis;
    bool jumping;
    float jumpDelay, jumpDelayAmt = 1.0f, playerRange;
    Tile* selectedTile;
    Camera2D* cam;
    TerrainGenerator* terrainGenerator;

    inline void FixedUpdate(Entity* entity) {
        /*
        if (entity->GetComponent<Rigidbody2D>()) {
            Rigidbody2D* rb = entity->GetComponent<Rigidbody2D>();

            //rb->body->SetLinearVelocity({ horizontalAxis * speed, rb->body->GetLinearVelocity().y});

            b2Vec2 vel = rb->body->GetLinearVelocity();
            float desiredVel = 0;

            desiredVel = horizontalAxis * speed;

            float velChange = desiredVel - vel.x;
            float impulse = rb->body->GetMass() * velChange; //disregard time factor
            float grav = rb->body->GetMass() * engine->physicsMgr->gravity.y; //disregard time factor
            rb->body->ApplyLinearImpulse(b2Vec2(impulse, 0), rb->body->GetWorldCenter(), true);

            //printf("%.2f\n", horizontalAxis);

        }
        */
    }

    void Update(Entity* entity);

    inline void Init() {
        health = 1.0f;
    }

    float health;
    bool direction;
    bool wb;
    float wbDelay;
    float speed = 3;
};

static int perlinSEED = 0;

static int perlinhash[] = { 208,34,231,213,32,248,233,56,161,78,24,140,71,48,140,254,245,255,247,247,40,
                     185,248,251,245,28,124,204,204,76,36,1,107,28,234,163,202,224,245,128,167,204,
                     9,92,217,54,239,174,173,102,193,189,190,121,100,108,167,44,43,77,180,204,8,81,
                     70,223,11,38,24,254,210,210,177,32,81,195,243,125,8,169,112,32,97,53,195,13,
                     203,9,47,104,125,117,114,124,165,203,181,235,193,206,70,180,174,0,167,181,41,
                     164,30,116,127,198,245,146,87,224,149,206,57,4,192,210,65,210,129,240,178,105,
                     228,108,245,148,140,40,35,195,38,58,65,207,215,253,65,85,208,76,62,3,237,55,89,
                     232,50,217,64,244,157,199,121,252,90,17,212,203,149,152,140,187,234,177,73,174,
                     193,100,192,143,97,53,145,135,19,103,13,90,135,151,199,91,239,247,33,39,145,
                     101,120,99,3,186,86,99,41,237,203,111,79,220,135,158,42,30,154,120,67,87,167,
                     135,176,183,191,253,115,184,21,233,58,129,233,142,39,128,211,118,137,139,255,
                     114,20,218,113,154,27,127,246,250,1,8,198,250,209,92,222,173,21,88,102,219 };

inline int noise2(int x, int y)
{
    int tmp = perlinhash[(y + perlinSEED) % 256];
    return perlinhash[(tmp + x) % 256];
}

inline float lin_inter(float x, float y, float s)
{
    return x + s * (y - x);
}

inline float smooth_inter(float x, float y, float s)
{
    return lin_inter(x, y, s * s * (3 - 2 * s));
}

inline float noise2d(float x, float y)
{
    int x_int = x;
    int y_int = y;
    float x_frac = x - x_int;
    float y_frac = y - y_int;
    int s = noise2(x_int, y_int);
    int t = noise2(x_int + 1, y_int);
    int u = noise2(x_int, y_int + 1);
    int v = noise2(x_int + 1, y_int + 1);
    float low = smooth_inter(s, t, x_frac);
    float high = smooth_inter(u, v, x_frac);
    return smooth_inter(low, high, y_frac);
}

inline float perlin2d(float x, float y, float freq, int depth)
{
    float xa = x * freq;
    float ya = y * freq;
    float amp = 1.0;
    float fin = 0;
    float div = 0.0;

    int i;
    for (i = 0; i < depth; i++)
    {
        div += 256 * amp;
        fin += noise2d(xa, ya) * amp;
        amp /= 2;
        xa *= 2;
        ya *= 2;
    }

    return fin / div;
}

inline void Game::UI::Devel::DrawEntityNode(Entity* entity) {
    ImGuiTreeNodeFlags flags = ((m_selectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, entity->name);

    if (ImGui::IsItemClicked()) {

        m_selectionContext = entity;
    }

    if (opened) {
        entity->imgui_iterate();
        ImGui::TreePop();
    }

    if (m_selectionContext == entity) {

        ImGui::Begin("Properties Window");

        if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen)) {

            ImGui::Text("Global Position"); ImGui::SameLine();
            ImGui::Text("X: %.3f Y: %.3f Z: %.3f", entity->globalPosition.x, entity->globalPosition.y, entity->globalPosition.z);

            ImGui::Text("Local Position");
            ImGui::DragFloat("Local pX", &entity->localPosition.x);
            ImGui::DragFloat("Local pY", &entity->localPosition.y);
            ImGui::DragFloat("Local pZ", &entity->localPosition.z);

            ImGui::Text("Global Rotation"); ImGui::SameLine();
            ImGui::Text("X: %.3f Y: %.3f Z: %.3f", entity->globalRotation.x, entity->globalRotation.y, entity->globalRotation.z);

            ImGui::Text("Local Rotation");
            ImGui::DragFloat("Local rX", &entity->localRotation.x);
            ImGui::DragFloat("Local rY", &entity->localRotation.y);
            ImGui::DragFloat("Local rZ", &entity->localRotation.z);

            ImGui::Text("Global Scale"); ImGui::SameLine();
            ImGui::Text("X: %.3f Y: %.3f Z: %.3f", entity->globalScale.x, entity->globalScale.y, entity->globalScale.z);

            ImGui::Text("Local Scale");
            ImGui::DragFloat("Local sX", &entity->localScale.x);
            ImGui::DragFloat("Local sY", &entity->localScale.y);
            ImGui::DragFloat("Local sZ", &entity->localScale.z);

            ImGui::Checkbox("Enabled", &entity->enabled);

            ImGui::TreePop();
        }

        for (auto const& component : entity->componentHolder->components) {

            auto comp = component.get();

            if (ImGui::TreeNodeEx(comp->GetName(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen)) {

                comp->imgui_properties();

                ImGui::TreePop();
            }
        }

        ImGui::End();
    }
}

inline void Game::UI::Devel::DrawConsole(Camera2D* cam, bool* lock, b2World* world, Engine* engine, Player* player, EntityData* entityData, b2Vec2 pos)
{
    ImGui::Begin("Game Properties");

    ImGui::DragFloat("Camera Target - X", &cam->target.x);
    ImGui::DragFloat("Camera Target - Y", &cam->target.y);
    ImGui::Text("Camera Target: %.2f, %.2f", cam->target.x, cam->target.y);
    ImGui::Checkbox("Lock Camera", lock);
    ImGui::Text("Gravity: %.2f, %.2f", world->GetGravity().x, world->GetGravity().y);
    ImGui::Text("Pos: %.2f, %.2f", pos.x, pos.y);
    ImGui::SliderFloat("Camera Zoom", &cam->zoom, 0.05, 20);
    ImGui::SliderFloat("Player Health", &player->health, 0.0f, 1.0f);
    ImGui::Checkbox("RPC", &gRPC);

    //rlImGuiImage(&rt.texture);

    ImGui::End();

    ImGui::Begin("Scene Heirarchy");

    for (Entity* entity : entityData->entities) {

        if (!entity->parent) {
            DrawEntityNode(entity);
        }
    }

    ImGui::End();

    //ImGui::SliderFloat("Gravity - X", &engine->physicsMgr->gravity.x, -1000, 1000);
    //ImGui::SliderFloat("Gravity - Y", &engine->physicsMgr->gravity.y, -1000000, 1000);
}

// Draw part of a texture (defined by a rectangle) with rotation and scale tiled into dest.
inline void DrawTextureTiled(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, float scale, Color tint)
{
    if ((texture.id <= 0) || (scale <= 0.0f)) return;  // Wanna see a infinite loop?!...just delete this line!
    if ((source.width == 0) || (source.height == 0)) return;

    int tileWidth = (int)(source.width * scale), tileHeight = (int)(source.height * scale);
    if ((dest.width < tileWidth) && (dest.height < tileHeight))
    {
        // Can fit only one tile
        DrawTexturePro(texture, Rectangle { source.x, source.y, ((float)dest.width / tileWidth)* source.width, ((float)dest.height / tileHeight)* source.height },
             {
            dest.x, dest.y, dest.width, dest.height
        }, origin, rotation, tint);
    }
    else if (dest.width <= tileWidth)
    {
        // Tiled vertically (one column)
        int dy = 0;
        for (; dy + tileHeight < dest.height; dy += tileHeight)
        {
            DrawTexturePro(texture, Rectangle { source.x, source.y, ((float)dest.width / tileWidth)* source.width, source.height },  { dest.x, dest.y + dy, dest.width, (float)tileHeight }, origin, rotation, tint);
        }

        // Fit last tile
        if (dy < dest.height)
        {
            DrawTexturePro(texture, Rectangle{ source.x, source.y, ((float)dest.width / tileWidth)* source.width, ((float)(dest.height - dy) / tileHeight)* source.height },
                 {
                dest.x, dest.y + dy, dest.width, dest.height - dy
            }, origin, rotation, tint);
        }
    }
    else if (dest.height <= tileHeight)
    {
        // Tiled horizontally (one row)
        int dx = 0;
        for (; dx + tileWidth < dest.width; dx += tileWidth)
        {
            DrawTexturePro(texture,  { source.x, source.y, source.width, ((float)dest.height / tileHeight)* source.height },  { dest.x + dx, dest.y, (float)tileWidth, dest.height }, origin, rotation, tint);
        }

        // Fit last tile
        if (dx < dest.width)
        {
            DrawTexturePro(texture,  { source.x, source.y, ((float)(dest.width - dx) / tileWidth)* source.width, ((float)dest.height / tileHeight)* source.height },
                 {
                dest.x + dx, dest.y, dest.width - dx, dest.height
            }, origin, rotation, tint);
        }
    }
    else
    {
        // Tiled both horizontally and vertically (rows and columns)
        int dx = 0;
        for (; dx + tileWidth < dest.width; dx += tileWidth)
        {
            int dy = 0;
            for (; dy + tileHeight < dest.height; dy += tileHeight)
            {
                DrawTexturePro(texture, source,  { dest.x + dx, dest.y + dy, (float)tileWidth, (float)tileHeight }, origin, rotation, tint);
            }

            if (dy < dest.height)
            {
                DrawTexturePro(texture,  { source.x, source.y, source.width, ((float)(dest.height - dy) / tileHeight)* source.height },
                     {
                    dest.x + dx, dest.y + dy, (float)tileWidth, dest.height - dy
                }, origin, rotation, tint);
            }
        }

        // Fit last column of tiles
        if (dx < dest.width)
        {
            int dy = 0;
            for (; dy + tileHeight < dest.height; dy += tileHeight)
            {
                DrawTexturePro(texture,  { source.x, source.y, ((float)(dest.width - dx) / tileWidth)* source.width, source.height },
                     {
                    dest.x + dx, dest.y + dy, dest.width - dx, (float)tileHeight
                }, origin, rotation, tint);
            }

            // Draw final tile in the bottom right corner
            if (dy < dest.height)
            {
                DrawTexturePro(texture,  { source.x, source.y, ((float)(dest.width - dx) / tileWidth)* source.width, ((float)(dest.height - dy) / tileHeight)* source.height },
                     {
                    dest.x + dx, dest.y + dy, dest.width - dx, dest.height - dy
                }, origin, rotation, tint);
            }
        }
    }
}

inline bool ColorEquals(Color c1, Color c2) {
    if (c1.r == c2.r && c1.g == c2.g && c1.b == c2.b) {
        return true;
    }
    else {
        return false;
    }
}

struct compare
{
    Vector2 vkey;
    Color ckey;
    compare(Vector2 const& i) : vkey(i) {}
    compare(Color const& i) : ckey(i) {}

    bool operator()(Vector2 const& i) {
        return (Vector2Equals(i, vkey));
    }

    bool operator()(const Color& i) {
        return (ColorEquals(i, ckey));
    }
};

struct MLColor {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;

    MLColor() = default;

    MLColor(Color color) {
        r = color.r;
        g = color.g;
        b = color.b;
        a = color.a;
    }

    MLColor operator()(Color const& color) {
        r = color.r;
        g = color.g;
        b = color.b;
        a = color.a;
    }

};

#define csprivate(x) private: x public:
#define cspublic(x) public: x private:

template <class TemplateClass> struct List {
private:
    std::vector<TemplateClass> data;

public:

    int GetLength() {
        return data.size();
    }

    void Insert(TemplateClass object, int position) {
        
        printf("inserted at %d\n", position);

        data.insert(data.begin() + position, object);
    }

    TemplateClass& Get(int index) {
        return data.at(index)
    }

    int IndexOf(TemplateClass& object) {
        auto itr = std::find_if(data.begin(), data.end(), [object](const TemplateClass& v1) {return compare(object)(v1); });

        if (itr == data.end()) {
            printf("nope\n");
            return 0;
        }
        else {
            printf("yep\n");
            return std::distance(data.begin(), itr);
        }
    }

    void Add(TemplateClass object) {
        //printf("pushed back at %d\n", data.size());
        data.push_back(object);
    }

    bool Contains(TemplateClass& object) {
        for (TemplateClass t : data) {
            if (compare(object)(t)) {
                return true;
                break;
            }
        }
        return false;
    }

    void Unload() {
        data.erase();
    }

    void Remove(int index) {
        data.erase(data.begin() + index);
    }

    TemplateClass& operator[](int i) {

        return data[i];
    }
};

struct Tile {
    const char* tileName;
    std::vector<Texture2D> tileSprites;

    bool background;

    float rarity;

    inline void Unload() {
        for (Texture2D tex : tileSprites) {
            UnloadTexture(tex);
        }
    }

    inline static Tile* Load(std::vector<const char*> texs, const char* name) {
        Tile* tile = new Tile();

        for (const char* path : texs) {
            tile->tileSprites.push_back(LoadTexture(path));
        }
        tile->tileName = name;

        return tile;
    }

    inline static Tile* Load(std::vector<Texture2D> texs, const char* name) {
        Tile* tile = new Tile();

        tile->tileSprites = texs;

        tile->tileName = name;

        return tile;
    }

};



struct TileAtlas {
public:
    std::vector<Tile*> tiles;

    inline void Unload() {

        for (Tile* tile : tiles) {
            tile->Unload();
            delete tile;
        }
    }

    Tile* getByName(const char* name) {

        //printf("%d\n", tiles.size());

        for (int i = 0; i < tiles.size(); i++) {
            Tile* tile = tiles.at(i);
            if (strcmp(tile->tileName, name) == 0) {
                return tile;
            }
        }
    }

    static TileAtlas* LoadDefault() {

        TileAtlas* tileAtlas = new TileAtlas();

        tileAtlas->tiles.push_back(Tile::Load({"resources/test/trunk_side.png"}, "log"));
        tileAtlas->tiles.push_back(Tile::Load({ "resources/test/leaves_transparent.png" }, "leaf"));
        tileAtlas->tiles.push_back(Tile::Load({ "resources/test/sand.png" }, "sand"));
        tileAtlas->tiles.push_back(Tile::Load({ "resources/test/dirt_snow.png" }, "snow"));
        tileAtlas->tiles.push_back(Tile::Load({ "resources/test/stone.png" }, "stone"));
        tileAtlas->tiles.push_back(Tile::Load({ "resources/test/dirt.png" }, "dirt"));
        tileAtlas->tiles.push_back(Tile::Load({ "resources/test/sand.png" }, "sand"));
        tileAtlas->tiles.push_back(Tile::Load({ "resources/test/dirt_grass.png" }, "grass"));
        tileAtlas->tiles.push_back(Tile::Load({ "resources/test/bedrock.png" }, "bedrock"));
        tileAtlas->tiles.push_back(Tile::Load({ "resources/test/grass1.png", "resources/test/grass2.png", "resources/test/grass3.png", "resources/test/grass3.png" }, "tallGrass"));
        tileAtlas->tiles.push_back(Tile::Load({ "resources/blocks/water/tile000.png" }, "water"));

        Tile* waterTile = tileAtlas->getByName("water");

        if (waterTile) {

            //printf("%d\n", (int)waterTile->tileSprites.size());

            using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
            for (const auto& dirEntry : recursive_directory_iterator("resources/blocks/water/")) {
                Texture2D tex2D = LoadTexture(dirEntry.path().string().c_str());
                waterTile->tileSprites.push_back(tex2D);
            }
        }

        return tileAtlas;
    }
};

struct Ore {
    float rarity, size;
    int maxSpawnHeight;
    const char* name;
    Tile* sprite;

    inline void Unload() {
        UnloadImage(spreadTexture);
        sprite->Unload();
        delete sprite;
    }

    static Ore Load(float rarity, float size, int maxSpawnHeight, const char* name, const char* path) {
        Ore ore = {};
        ore.rarity = rarity;
        ore.size = size;
        ore.maxSpawnHeight = maxSpawnHeight;
        ore.name = name;

        ore.sprite = Tile::Load({ path }, name);

        return ore;
    }

    Image spreadTexture;
};

inline float distance(int x1, int y1, int x2, int y2)
{
    // Calculating distance
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2) * 1.0);
}

struct Biome {

    Color biomeCol;
    const char* biomeName;

    TileAtlas* tileAtlas;

    float caveFreq = 0.06f;
    float terrainFreq = 0.08f;
    Image caveNoiseTexture;

    bool generateCaves = true;
    int dirtHeight = 5;
    float surfaceValue = 0.25f;
    float heightMultiplier = 25;

    float minTreeHeight = 3;
    float maxTreeHeight = 6;
    int treeChance = 15;
    int tallGrassChance = 10;
    int treeHeight = 36;
    int tallGrassHeight = 40;
    int waterLevel = 35;

    std::vector<Ore> ores;

    static Biome Load(const char* name, Color color) {
        Biome biome = Biome();
        biome.biomeName = name;
        biome.biomeCol = color;

        biome.tileAtlas = TileAtlas::LoadDefault();

        biome.ores.push_back((Ore::Load(0.1, 0.76, 50, "Coal", "resources/test/coal_ore.png")));
        biome.ores.push_back((Ore::Load(0.08, 0.8, 30, "Iron", "resources/test/iron_ore.png")));
        biome.ores.push_back((Ore::Load(0.07, 0.85, 20, "Gold", "resources/test/gold_ore.png")));
        biome.ores.push_back((Ore::Load(0.075, 0.9, 17, "Diamond", "resources/test/diamond_ore.png")));

        return biome;
    }

    void Unload() {
        UnloadImage(caveNoiseTexture);
        for (int o = 0; o < ores.size(); o++) {
            UnloadImage(ores.at(o).spreadTexture);
        }
        tileAtlas->Unload();
        delete tileAtlas;
    }
};

inline int GetIndexOfColor(std::vector<Color> vec, Color col) {
    auto itr = std::find_if(vec.begin(), vec.end(), [col](const Color& v1) {return v1.r == col.r && v1.g == col.g && v1.b == col.b; });

    if (itr == vec.end()) {
        //printf("nope\n");
        return 0;
    }
    else {
        //printf("%d\n", std::distance(vec.begin(), itr));
        return std::distance(vec.begin(), itr);
    }
}

class Button : public Component {
    CLASS_DECLARATION(Button)
public:
    Button(std::string&& initialValue)
        : Component(std::move(initialValue)) {
    }

    Button() = default;

    Camera2D* cam;
    Rectangle rec;
    Rectangle nrec;
    bool mouseUp, mouseDown;

    inline void Update(Entity* entity) {
        
        Vector2 pos = GetWorldToScreen2D({ -entity->globalPosition.x * entity->globalScale.x, -entity->globalPosition.y * entity->globalScale.y }, *cam);

        rec.x = pos.x;
        rec.y = pos.y;

        nrec = rec;
        nrec.width = nrec.width * entity->globalScale.x;
        nrec.height = nrec.height * entity->globalScale.y;

        //DrawRectangleRec(nrec, GREEN);

        

        if (CheckCollisionPointRec(GetMousePosition(), nrec))
        {
            mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            mouseUp = IsMouseButtonUp(MOUSE_BUTTON_LEFT);
        }
        else {
            mouseDown = false;
            mouseUp = false;
        }
        

    }

    void Init(Camera2D* cam) {
        this->cam = cam;
    }
};

class Block : public Component {
    CLASS_DECLARATION(Block)
public:
    Block(std::string&& initialValue)
        : Component(std::move(initialValue)) {
    }

    Block() = default;

    float amountDown;
    float strength = 1.0;
    Tile* tile;
    Vector2 tilePos;

    inline void Update(Entity* entity) {
        Button* button = entity->GetComponent<Button>();
        if (button) {
            if (button->mouseUp) {
                selectedName = entity->name;
                DrawRectangleLines(-entity->globalPosition.x * entity->globalScale.x, -entity->globalPosition.y * entity->globalScale.y, button->nrec.width, button->nrec.height, WHITE);
            }
            if (button->mouseDown) {
                amountDown += 0.1;
                if (amountDown < strength) {
                    unsigned char f = 255*(amountDown / strength);
                    Color col = ColorTint({ f,f,f,255 }, GREEN);
                    DrawRectangleLines(-entity->globalPosition.x * entity->globalScale.x, -entity->globalPosition.y * entity->globalScale.y, button->nrec.width, button->nrec.height, col);
                }
                else {
                    if (entity->GetComponent<Button>()) {
                        entity->GetComponent<Button>()->enabled = false;
                    }
                    this->enabled = false;
                    if (entity->GetComponent<Renderer2D>()) {
                        entity->tag = "bg";
                        entity->GetComponent<Renderer2D>()->tint = DARKGRAY;
                    }
                    if (entity->GetComponent<Rigidbody2D>()) {
                        entity->GetComponent<Rigidbody2D>()->body->SetEnabled(false);
                    }
                }
            }
            else {
                amountDown = 0;
            }
        }
    }
};

inline b2Vec2 rlToB2Vec2(Vector2 v2) {
    return { v2.x, v2.y };
}

inline Vector2 b2toRlVec2(b2Vec2 v2) {
    return { v2.x, v2.y };
}

inline bool v2InRec(Vector2 firstpos, Vector2 secondpos, Vector2 pos)
{
    Vector2 loPoint = firstpos;
    Vector2 hiPoint = secondpos;

    // swap x if needed
    if (loPoint.x > hiPoint.x)
    {
        float tempX = loPoint.x;
        loPoint.x = hiPoint.x;
        hiPoint.x = tempX;
    }

    // swap y if needed
    if (loPoint.y > hiPoint.y)
    {
        float tempY = loPoint.y;
        loPoint.y = hiPoint.y;
        hiPoint.y = tempY;
    }

    // just check if point is inside
    return pos.x > loPoint.x && pos.x < hiPoint.x
        && pos.y > loPoint.y && pos.y < hiPoint.y;
}

class TerrainGenerator : public Component {
    CLASS_DECLARATION(TerrainGenerator)
public:
    TerrainGenerator(std::string&& initialValue)
        : Component(std::move(initialValue)) {
    }

    TerrainGenerator() = default;

    // Tile Atlas

    // Ores
    std::vector<Ore> ores;

    // Biomes
    Image biomeMap;
    Texture biomeMapTexture;
    float biomeFrequency = 0.01; 

    std::vector<Biome> biomes;

    // Generation Settings
    int worldSize = 100;
    int chunkSize = 16;
    //bool generateCaves = true;
    //int dirtHeight = 5;
    float surfaceValue = 0.25f;
    //float heightMultiplier = 25;
    int heightAddition = 25;

    float lightThreshold, lightRadius = 7.0f;

    // Addons

    //Trees
    //float minTreeHeight = 3;
    //float maxTreeHeight = 10;
    //int treeChance = 15;

    // Noise Settings
    float seed;
    float caveFreq = 0.06f;
    float terrainFreq = 0.08f; 
    Image caveNoiseTexture;

    // C++ Specifics
    std::int32_t octaves = 8;
    float perlinFreq = 8.0f;

    List<Vector2> tiles;
    std::vector<Entity*> worldChunks;
    std::vector<Color> biomeColors;
    List<Tile*> tileClasses;
    List<Entity*> tileObjects;

    Camera2D* cam;
    Entity* player;

    inline void imgui_properties() {
        rlImGuiImageSize(&biomeMapTexture, 100, 100);

    }

    inline void Init(Scene* connectedScene, Camera2D* camera, Entity* player) {
        seed = GetRandomValue(-10000,10000);
        scene = connectedScene;
        cam = camera;
        this->player = player;

        CreateChunks();
        //GenerateTerrain();
    };

    Vector2 GetTilePosition(Entity* tile) {
        Vector2 tpos = tile->GetComponent<Block>()->tilePos;
        return tpos;
    }

    Entity* GetPositionTile(Vector2 pos) {
        for (Entity* chunk : entity->children) {
            for (Entity* tile : chunk->children) {
                if (tile->GetComponent<Block>()) {
                    Vector2 tpos = tile->GetComponent<Block>()->tilePos;
                    if (Vector2Equals(pos, tpos)) {
                        return tile;
                    }
                }
            }
        }
    }

    inline void RefreshChunks() {
        for (Entity* chunk : entity->children) {
            for (Entity* tile : chunk->children) {
                Vector2 scrOrg = GetScreenToWorld2D({ -100,-100 }, *cam);
                Vector2 scr = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, *cam);
                bool in = v2InRec(scr, scrOrg, { -tile->globalPosition.x * tile->GetGlobalScale().x, -tile->globalPosition.y * tile->GetGlobalScale().y });
                if (in) {
                    //DrawText("disabled", -(tile->globalPosition.x*tile->GetGlobalScale().x), -(tile->globalPosition.y*tile->GetGlobalScale().y), 10, RED);
                    tile->SetEnabled(true);
                }
                else {
                    tile->SetEnabled(false);
                }
                /*
                if (tile->GetComponent<Block>()) {
                    Vector2 tpos = GetTilePosition(tile);
                    Entity* tileAbove = GetPositionTile(Vector2Add(tpos, { 0,1 }));
                    Biome biome = GetCurrentBiome(tpos.x, tpos.y);
                    if (tileAbove && tileAbove->GetComponent<Block>()) {
                        printf("%d\n", tileAbove->internalID);
                        if (strcmp(tileAbove->GetComponent<Block>()->tile->tileName, "water") == 0) {
                            tile->GetComponent<Renderer2D>()->texture->tex = biome.tileAtlas->getByName("sand")->tileSprites.at(GetRandomValue(1, biome.tileAtlas->getByName("sand")->tileSprites.size()) - 1);
                        }
                    }
                }
                */
            }
        }
    }

    inline void AddBiomeColors() {

        biomeColors.clear();

        for (int i = 0; i < biomes.size(); i++) {
            biomeColors.emplace_back(biomes.at(i).biomeCol);
        }

        DrawTextures();

    }

    inline Image DrawBiomeTexture() {

        Image image;

        Color* pixels = (Color*)RL_MALLOC(worldSize * worldSize * sizeof(Color));

        for (int y = 0; y < worldSize; y++)
        {
            for (int x = 0; x < worldSize; x++)
            {
                float v = perlin.octave2D_01((x + seed) * biomeFrequency, (y+seed) * biomeFrequency, octaves);
                unsigned char vColor = v * 255;
                pixels[y * worldSize + x] = { vColor, vColor, vColor, 255 };

                float div = 1.0f / biomes.size();

                for (int i = 0; i < biomes.size(); i++) {
                    if (v > div * i && v < div * (i + 1)) {
                        pixels[y * worldSize + x] = biomes.at(i).biomeCol;
                    }
                }
            }
        }

        image.data = pixels;
        image.width = worldSize;
        image.height = worldSize;
        image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        image.mipmaps = 1;

        return image;
    }

    inline void DrawTextures() {

        UnloadImage(biomeMap);

        biomeMap = DrawBiomeTexture();
        biomeMapTexture = LoadTextureFromImage(biomeMap);

        caveNoiseTexture = GenerateNoiseTexture(caveFreq, surfaceValue);

        for (int i = 0; i < biomes.size(); i++) {

            for (int o = 0; o < biomes.at(i).ores.size(); o++) {
                biomes.at(i).ores.at(o).spreadTexture = GenerateNoiseTexture(biomes.at(i).ores.at(o).rarity, biomes.at(i).ores.at(o).size);
            }

            biomes.at(i).caveNoiseTexture = GenerateNoiseTexture(biomes.at(i).caveFreq, biomes.at(i).surfaceValue);

        }
    }

    void CreateChunks() {
        int numChunks = worldSize / chunkSize;
        worldChunks.clear();
        for (int chunkN = 0; chunkN < numChunks; chunkN++) {
            Entity* newChunk = scene->entityData->createEntity(TextFormat("%d", chunkN));
            entity->AddChild(newChunk);
            worldChunks.push_back(newChunk);

        }
    }

    inline void GenerateTerrain() {

        AddBiomeColors();

        Tile* curTile;

        for (int x = 0; x < worldSize; x++) {


            for (int y = 0; y < worldSize; y++) {
                //printf("%.2f\n",GetImageColor(noiseTexture, x, y).r);

                if (y <= 5) {
                    curBiome = GetCurrentBiome(x, y);
                    curTile = curBiome.tileAtlas->getByName("bedrock");
                    PlaceTile(curTile, x, y);
                }
                else {

                    Color tint = WHITE;

                    curBiome = GetCurrentBiome(x, y);
                    float height = Clamp((perlin.octave2D_01((x + seed) * curBiome.terrainFreq, seed * curBiome.terrainFreq, octaves)), 0, 1) * curBiome.heightMultiplier + heightAddition;

                    if (y >= height) {
                        //printf("%s, %.2f\n", curBiome.biomeName, height);

                        if (y < curBiome.waterLevel) {
                            curTile = curBiome.tileAtlas->getByName("water");
                            PlaceTile(curTile, x, y,BLUE, false);
                        }
                    }
                    else {

                        if (y < height - curBiome.dirtHeight) {

                            curTile = curBiome.tileAtlas->getByName("stone");

                            for (Ore ore : curBiome.ores) {
                                if (GetImageColor(ore.spreadTexture, x, y).r > 0.5f * 255 && height - y < ore.maxSpawnHeight) {
                                    curTile = ore.sprite;
                                }
                            }
                        }
                        else if (y < height - 1) {
                            curTile = curBiome.tileAtlas->getByName("dirt");
                        }
                        else {

                            curTile = curBiome.tileAtlas->getByName("grass");

                            // we got the top layer
                        }

                        if (curBiome.generateCaves) {
                            if (GetImageColor(curBiome.caveNoiseTexture, x, y).r > 255 * 0.5f)
                            {
                                PlaceTile(curTile, x, y);
                            }
                            else {
                                PlaceTile(curTile, x, y, DARKGRAY, false, true);
                            }
                        }
                        else {
                            PlaceTile(curTile, x, y);
                        }

                        if (y >= height - 1) {
                            int t = GetRandomValue(0, curBiome.treeChance);
                            if (t == 1 && height >= curBiome.treeHeight) {
                                // generate a tree

                                if (tiles.Contains(Vector2 {(float)x, (float)y})) {
                                    //printf("contains\n");
                                    if (ColorEquals(curBiome.biomeCol, ORANGE)) {

                                    }
                                    else {
                                        GenerateTree(x, y + 1, curBiome);
                                    }
                                }
                                else {
                                    //printf("doesnt contain\n");
                                }
                            }
                            else {
                                int i = GetRandomValue(0, curBiome.tallGrassChance);

                                if (i == 1 && height >= curBiome.treeHeight) {

                                    if (tiles.Contains(Vector2 {(float)x,(float)y})) {
                                        PlaceTile(curBiome.tileAtlas->getByName("tallGrass"), x, y + 1);
                                    }

                                }

                            }
                        }
                    }
                }
            }
        }
    }

    Biome curBiome;

    Biome GetCurrentBiome(int x, int y) {

        for (Biome biome : biomes) {
            if (ColorEquals(biome.biomeCol, GetImageColor(biomeMap, x, y))) {
                return biome;
                break;
            }
        }

        return curBiome;
    }

    const siv::PerlinNoise perlin{(siv::PerlinNoise::seed_type)12345};

private:

    friend Player;

    void Update(Entity* entity) {
        //printf("updated\n");
        //RefreshChunks();
    }

    inline void Unload() {
        printf("Unloaded terrain generator");

        for (Ore ore : ores) {
            ore.Unload();
        }

        UnloadImage(caveNoiseTexture);
        UnloadImage(biomeMap);

        for (int i = 0; i < biomes.size(); i++) {

            biomes[i].Unload();
        }
    }

    Scene* scene;

    void GenerateTree(float x, float y, Biome biome) {

        //printf("generating a tree\n");

        int treeHeight = GetRandomValue(biome.minTreeHeight, biome.maxTreeHeight);

        // generate log
        for (int i = 0; i <= treeHeight; i++) {
            if (i == 0) {
                PlaceTile(biome.tileAtlas->getByName("log"), x, y + i);
            }
            else {
                PlaceTile(biome.tileAtlas->getByName("log"), x, y + i);
            }
        }

        // generate leaves
        PlaceTile(biome.tileAtlas->getByName("leaf"), x, y + treeHeight, biome.biomeCol);
        PlaceTile(biome.tileAtlas->getByName("leaf"), x, y + treeHeight+1, biome.biomeCol);

        PlaceTile(biome.tileAtlas->getByName("leaf"), x-1, y + treeHeight, biome.biomeCol);
        PlaceTile(biome.tileAtlas->getByName("leaf"), x-1, y + treeHeight+1, biome.biomeCol);

        PlaceTile(biome.tileAtlas->getByName("leaf"), x + 1, y + treeHeight, biome.biomeCol);
        PlaceTile(biome.tileAtlas->getByName("leaf"), x + 1, y + treeHeight + 1, biome.biomeCol);

        PlaceTile(biome.tileAtlas->getByName("leaf"), x, y + treeHeight + 2, biome.biomeCol);
        PlaceTile(biome.tileAtlas->getByName("leaf"), x, y + treeHeight - 1, biome.biomeCol);
    }

    void RemoveTile(int x, int y) {
        if (tiles.Contains(Vector2 { (float)x,(float)y }) && x >= 0 && x <= worldSize && y >= 0 && y <= worldSize) {
            scene->entityData->DestroyEntity(tileObjects[tiles.IndexOf(Vector2 { (float)x,(float)y })]);
            tileObjects.Remove(tiles.IndexOf(Vector2 { (float)x, (float)y }));
            tileClasses.Remove(tiles.IndexOf(Vector2  { (float)x, (float)y }));
            tiles.Remove(tiles.IndexOf(Vector2 { (float)x, (float)y }));

        }
    }


    void CheckTile(Tile* tile, float x, float y, Color color = WHITE, bool breakable = true, bool background = false) {
        if (x >= 0 && x <= worldSize && y >= 0 && y <= worldSize) {

            if (!tiles.Contains(Vector2 { x,y })) {
                PlaceTile(tile, x, y, color, breakable, background);
            }
            else {
                if (tileClasses[tiles.IndexOf(Vector2 { x,y })]->background) {
                    RemoveTile(x, y);
                    PlaceTile(tile, x, y, color, breakable, background);
                }
            }
        }
    }

    void PlaceTile(Tile* tile, float x, float y, Color color = WHITE, bool breakable = true, bool background = false) {


        if (x >= 0 && x<= worldSize && y >= 0 && y <= worldSize) {


            int chunkCoord = round((int)x / chunkSize) * chunkSize;
            chunkCoord /= chunkSize;
            chunkCoord = Clamp(chunkCoord, 0, (worldSize / chunkSize) - 1);

            //printf("%d\n", chunkCoord);

            Entity* chunk = worldChunks.at(chunkCoord);

            float tileSize = 1;

            int spriteIndex; 
            spriteIndex = GetRandomValue(1, tile->tileSprites.size()) - 1;

            //printf("%d, %d\n", sprites.size(), spriteIndex);

            Texture2D sprite = tile->tileSprites.at(spriteIndex);

            Entity* newTile = scene->entityData->createEntity(tile->tileName);

            newTile->localPosition = { ((float)x) + ((sprite.width / 2)), ((float)y) + ((sprite.height / 2)) };
            worldChunks.at(chunkCoord)->AddChild(newTile);
            Renderer2D* renderer = newTile->AddComponent<Renderer2D>();
            renderer->rec = { 0,0,tileSize, tileSize };
            renderer->Init(engine->textureMgr->LoadTextureM(sprite));
            renderer->tint = color;
            tiles.Add(Vector2Subtract({ newTile->localPosition.x, newTile->localPosition.y }, Vector2Multiply(Vector2One(), { ((float)(sprite.width / 2)),((float)(sprite.height / 2)) })));
            tileObjects.Add(newTile);
            tileClasses.Add(tile);


            Block* block = newTile->AddComponent<Block>();
            block->tile = tile;
            block->tilePos = { x,y };

            if (breakable) {
                /*
                Button* button = newTile->AddComponent<Button>();
                button->Init(cam);
                button->rec = { 0,0,tileSize,tileSize };
                */
            }

            if (background) {
                newTile->tag = "bg";
            }
            else {

                /*

                newTile->GetGlobalPosition();
                newTile->GetGlobalScale();



                b2BodyDef groundBodyDef;
                groundBodyDef.type = b2_staticBody;
                groundBodyDef.position.Set(x, y);
                b2Body* groundBody = engine->physicsMgr->world->CreateBody(&groundBodyDef);
                b2PolygonShape groundBox;
                groundBox.SetAsBox(tileSize/2, tileSize/2);

                float a = tileSize / 2;

                //groundBox.SetAsBox(a, a, { (newTile->globalPosition.x*newTile->globalScale.x)*a,(newTile->globalPosition.y*newTile->globalScale.y)*a }, 0.0f);
                groundBody->CreateFixture(&groundBox, 0.0f);

                Rigidbody2D* rb = newTile->AddComponent<Rigidbody2D>();
                rb->body = groundBody;
                */
            }
            
        }
    }

    void LightBlock(int x, int y, float intensity, int iteration) {
        Image img = LoadImageFromTexture(engine->lightMgr->renderTexture.texture);
        if (iteration < lightRadius) {

            SetPixelColor(img.data, ColorBrightness(WHITE, intensity), PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

            for (int nx = x - 1; nx < x + 2; nx++) {
                for (int ny = x - 1; ny < x + 2; ny++) {
                    if (nx != x || ny != y) {
                        float dist = Vector2Distance({ (float)x,(float)y }, { (float)nx,(float)ny });
                        float targetIntensity = pow(0.7f, dist) * intensity;
                        if (!ColorEquals(GetPixelColor(img.data, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8), BLACK)) {
                            if (GetPixelColor(img.data, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8).r < targetIntensity) {
                                LightBlock(nx, ny, targetIntensity, iteration + 1);
                            }
                        }
                    }
                }
            }
        }

    }

    Image GenerateNoiseTexture(float frequency, float limit) {

        Image noiseTexture;

        Color* pixels = (Color*)RL_MALLOC(worldSize * worldSize * sizeof(Color));
        
        for (int y = 0; y < worldSize; y++)
        {
            for (int x = 0; x < worldSize; x++)
            {
                float v = perlin.octave2D_01((x + seed) * frequency, (y + seed) * frequency, octaves);
                if (v > limit) {
                    pixels[y * worldSize + x] = WHITE;
                }
                else {
                    pixels[y * worldSize + x] = BLACK;
                }
                //unsigned char vColor = v * 255;
                //pixels[y * worldSize + x] = { vColor, vColor, vColor, 255 };
            }
        }

        noiseTexture.data = pixels;
        noiseTexture.width = worldSize;
        noiseTexture.height = worldSize;
        noiseTexture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        noiseTexture.mipmaps = 1;

        return noiseTexture;
    }
};

inline Entity* QuickAnimated(const char* tag, const char* textureName, EntityData* entityData, Rectangle src, Rectangle rec, const char* name) {
    Entity* heart = entityData->createEntity(name);
    AnimatedRenderer2D* heartRen = heart->AddComponent<AnimatedRenderer2D>();
    heartRen->Init(textureName);
    heartRen->srcRect = src;
    heartRen->rec = rec;
    heart->tag = tag;

    return heart;
}

inline Entity* QuickAnimated(const char* tag, TextureRes* texture, EntityData* entityData, Rectangle src, Rectangle rec, const char* name) {
    Entity* heart = entityData->createEntity(name);
    AnimatedRenderer2D* heartRen = heart->AddComponent<AnimatedRenderer2D>();
    heartRen->Init(texture);
    heartRen->srcRect = src;
    heartRen->rec = rec;
    heart->tag = tag;

    return heart;
}

class TestScene : public Scene {
public:
    void scene_init() {


        originalSetup();

        BeginDrawing();

        ClearBackground(BLACK);

        EndDrawing();


        Entity* player = entityData->createEntity("Player");
        AnimatedRenderer2D* renderer = player->AddComponent<AnimatedRenderer2D>();
        renderer->Init("resources/player/atlas.png");
        renderer->srcRect = { 32*1, 32*1, 32, 32};
        renderer->rec = { 0,0,100,100 };
        Player* playerController = player->AddComponent<Player>();
        playerController->Init();

        bool useRb = false;
        if (useRb) {
            player->localPosition = { 0,-50,0 };



            b2PolygonShape plrShape;
            plrShape.SetAsBox(1.0f, 2.0f);
            b2BodyDef bodyDef;
            bodyDef.type = b2_dynamicBody;
            bodyDef.position.Set(-player->localPosition.x, -player->localPosition.y);
            b2Body* body = engine->physicsMgr->world->CreateBody(&bodyDef);
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &plrShape;
            fixtureDef.density = 0.25f;
            fixtureDef.friction = 0.3f;
            body->CreateFixture(&fixtureDef);

            Rigidbody2D* rigidBody = player->AddComponent<Rigidbody2D>();
            rigidBody->body = body;
            rigidBody->body->SetGravityScale(1);
        }
        else {
            player->localPosition = { 0,-400,0 };
        }

        Camera2D camera = { 0 };
        camera.target = { player->localPosition.x+20.0f,player->localPosition.y+20.0f };
        camera.offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
        camera.rotation = 0.0f;
        camera.zoom = 1.0f;
        aurCamera* cam = engine->cameraMgr->loadCamera(camera);
        cam->Attach(player, renderer->rec.width, renderer->rec.height);
        cam->useCam2D = true;

        playerController->cam = &cam->cam2D;

        engine->lightMgr->cam = cam;

        Entity* heart1 = QuickAnimated("ui", "resources/test/tiles_packed.png", entityData, { 18 * 6, 18 * 2, 18, 18 }, { 0,0,50,50 }, "heart1");
        heart1->localPosition = { 0.95f * GetScreenWidth(), 0.0125f * GetScreenHeight() };

        Entity* heart2 = QuickAnimated("ui", engine->textureMgr->textures.at(1), entityData, {18 * 5, 18 * 2, 18, 18}, {0,0,50,50}, "heart2");
        heart2->localPosition = { 0.915f * GetScreenWidth(), 0.0125f * GetScreenHeight() };

        Entity* heart3 = QuickAnimated("ui", engine->textureMgr->textures.at(1), entityData, {18 * 4, 18 * 2, 18, 18}, {0,0,50,50}, "heart3");
        heart3->localPosition = { 0.88f * GetScreenWidth(), 0.0125f * GetScreenHeight() };

        Entity* terrain = entityData->createEntity("terrain");

        terrain->localPosition = { -50,-50 };

        //terrain->localPosition = {-225, -100};

        float terrainScale = 60;

        terrain->localScale = { terrainScale,terrainScale,terrainScale };
        terrainGenerator = terrain->AddComponent<TerrainGenerator>();
        terrainGenerator->Init(this, &cam->cam2D, player);

        

        Biome grassland = Biome::Load("Grassland", GREEN);

        terrainGenerator->biomes.push_back(grassland);

        Biome snow = Biome::Load("Snow", WHITE);
        snow.tileAtlas = TileAtlas::LoadDefault();
        snow.tileAtlas->getByName("grass")->Unload();
        snow.tileAtlas->tiles.push_back(Tile::Load({ "resources/test/grass_block_snow.png" }, "grass"));
        snow.treeChance = 10;
        snow.tallGrassChance = 0;
        terrainGenerator->biomes.push_back(snow);

        Biome desert = Biome::Load("Desert", ORANGE);
        desert.tileAtlas = TileAtlas::LoadDefault();
        desert.tileAtlas->getByName("grass")->Unload();
        desert.tileAtlas->tiles.push_back(Tile::Load({ "resources/test/sand.png" }, "grass"));
        desert.tileAtlas->getByName("dirt")->Unload();
        desert.tileAtlas->tiles.push_back(Tile::Load({ "resources/test/sand.png" }, "dirt"));
        desert.tileAtlas->getByName("stone")->Unload();
        desert.tileAtlas->tiles.push_back(Tile::Load({ "resources/test/sand.png" }, "stone"));
        desert.tileAtlas->getByName("tallGrass")->Unload();
        desert.tileAtlas->tiles.push_back(Tile::Load({"resources/test/dead_bush.png"}, "tallGrass"));
        desert.treeChance = 0;
        desert.tallGrassChance = 10;
        desert.ores.clear();
        terrainGenerator->biomes.push_back(desert);

        /*
        Biome ocean = Biome::Load("Ocean", BLUE);
        ocean.tileAtlas = TileAtlas::LoadDefault();
        ocean.tileAtlas->getByName("grass")->Unload();
        ocean.tileAtlas->tiles.push_back(Tile::Load({"resources/blocks/water/tile000.png"}, "grass"));
        ocean.tileAtlas->getByName("dirt")->Unload();
        ocean.tileAtlas->tiles.push_back(Tile::Load({ "resources/blocks/water/tile000.png" }, "dirt"));
        ocean.tileAtlas->getByName("stone")->Unload();
        ocean.tileAtlas->tiles.push_back(Tile::Load({ "resources/blocks/water/tile000.png" }, "stone"));
        ocean.tileAtlas->getByName("tallGrass")->Unload();
        ocean.tileAtlas->tiles.push_back(Tile::Load({ "resources/blocks/water/tile000.png" }, "tallGrass"));
        ocean.treeChance = 0;
        ocean.tallGrassChance = 10;
        ocean.heightMultiplier = 0;
        ocean.waterLevel = 25;
        ocean.ores.clear();
        terrainGenerator->biomes.push_back(ocean);
        */
        

        Biome forest = Biome::Load("Forest", DARKGREEN);
        forest.tileAtlas = TileAtlas::LoadDefault();
        forest.treeChance = 1;
        forest.tallGrassChance = 10;
        desert.ores.clear();
        terrainGenerator->biomes.push_back(forest);

        //terrainGenerator->AddBiomeColors();

        terrainGenerator->GenerateTerrain();
        terrainGenerator->RefreshChunks();

        playerController->terrainGenerator = terrainGenerator;

        /*
        Entity* heart = entityData->createEntity();
        AnimatedRenderer2D* heartRen = heart->AddComponent<AnimatedRenderer2D>();
        heartRen->Init("resources/test/tiles_packed.png");
        heartRen->srcRect = { 18 * 4, 18*2, 18, 18 };
        heartRen->rec = { 0,0,50,50 };
        heart->tag = "ui";
        */

        rt = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

        skyTexture = LoadTexture("resources/textures/sky.png");
        bgTexture = LoadTexture("resources/textures/mountains.png");

        engine->audioMgr->LoadAudio("resources/coin.wav", SoundType, "coin");

        SetupDiscord();

    }

    void scene_update(aurCamera* cam) {

        selectedName = "";

        engine->physicsMgr->gravity.Set(0, -500);

        //entityData->find("Player")->GetComponent<Rigidbody2D>()->offset = { 0,-30 };

        if (!lockCamera) {

            cam->cam2D.target = { entityData->find("Player")->localPosition.x + 20, entityData->find("Player")->localPosition.y + 20};

        }

        //SetupDiscord();
        UpdateDiscord("Test Scene", "testscene");

        BeginTextureMode(rt);

        ClearBackground(SKYBLUE);

        parallaxSpeed = .075;

        parallaxOffset = { cam->cam2D.target.x * parallaxSpeed, 0 };

        DrawTexturePro(skyTexture, { 0,0,(float)skyTexture.width, (float)skyTexture.height }, { 0,0,(float)GetScreenWidth(), (float)GetScreenHeight() }, {0,0}, 0, WHITE);
        DrawTexturePro(bgTexture, { parallaxOffset.x,0,(float)bgTexture.width, (float)bgTexture.height }, { 0,0,(float)GetScreenWidth(), (float)GetScreenHeight() }, {0,0}, 0, WHITE);

        BeginMode2D((Camera2D)cam->cam2D);

        rlPushMatrix();
        rlTranslatef(0, 25 * 50, 0);
        rlRotatef(90, 1, 0, 0);
        //DrawGrid(100, 50);
        rlPopMatrix();

        entityData->Update();

        EndMode2D();

        EndTextureMode();

        BeginDrawing();

        ClearBackground(SKYBLUE);

        //DrawText("gimme my lime (uwu *nuzzles*)\n", 100, 100, 10, WHITE);


        DrawTextureRec(rt.texture, { 0,0,(float)GetScreenWidth(), (float)-GetScreenHeight() }, { 0,0 }, WHITE);

        BeginBlendMode(BLEND_MULTIPLIED);

        DrawTextureRec(engine->lightMgr->renderTexture.texture, { 0,0,(float)GetScreenWidth(), (float)-GetScreenHeight() }, { 0,0 }, WHITE);

        EndBlendMode();

        if (!IsKeyDown(KEY_APOSTROPHE)) {

            DrawFPS(10, 10);
            DrawText(TextFormat("%.5f ms", GetFrameTime()), 10, 20, 20, LIME);

            DrawText("Please remember: this is NOT FINAL.\n The assets used are placeholders.", 10, 35, 20, RED);

            //DrawText("GIMME MY LIME", GetScreenWidth() / 4, GetScreenHeight() / 2, 100, LIME);

            DrawText(selectedName, GetMousePosition().x, GetMousePosition().y, 30, RED);

            //DrawText(TextFormat("Current biome: %s", terrainGenerator->GetCurrentBiome(entityData->find("Player")->localPosition.x, 0).biomeName), 10, 50, 20, terrainGenerator->GetCurrentBiome(entityData->find("Player")->localPosition.x, 0).biomeCol);



            entityData->UI_Update();

            #ifdef DEBUG
            rlImGuiBegin();

            if (entityData->find("Player")->GetComponent<Rigidbody2D>()) {
                Game::UI::Devel::DrawConsole(&cam->cam2D, &lockCamera, engine->physicsMgr->world, engine, entityData->entities.at(0)->GetComponent<Player>(), entityData, entityData->find("Player")->GetComponent<Rigidbody2D>()->body->GetPosition());
            }
            else {
                Game::UI::Devel::DrawConsole(&cam->cam2D, &lockCamera, engine->physicsMgr->world, engine, entityData->entities.at(0)->GetComponent<Player>(), entityData);
            }

            rlImGuiEnd();
            #endif // DEBUG


        }

        EndDrawing();

        if (IsKeyPressed(KEY_C)) {
            
            TakeScreenshot(TextReplace("screenshots/screenshotGOOD.png", "GOOD", std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()).c_str()));
        }
    }

    void scene_unload() {
        entityData->Unload();
        Discord_ClearPresence();
        Discord_Shutdown();
        originalUnload();
        UnloadTexture(skyTexture);
        UnloadRenderTexture(rt);
    }

    bool is2D = true;
    bool lockCamera;

    float parallaxSpeed = 0.5f; // Adjust this value to control the parallax effect

    Vector2 parallaxOffset;
    Texture2D skyTexture, bgTexture;
    std::vector<Texture2D> cachedTextures;
    TerrainGenerator* terrainGenerator;
    RenderTexture rt;
};

class HomePage : public Scene {
public:
    void scene_init() {


        originalSetup();
    }

    void scene_update(aurCamera* cam) {

        uiMgr.Update();

        BeginDrawing();

        ClearBackground(BLACK);

        uiMgr.Draw();

        Rectangle rec1 = { ((GetScreenWidth() / 2.0f) - (MeasureTextEx(GetFontDefault(), "Singleplayer", 50, 2).x / 2)) - 10, (GetScreenHeight() * 0.35f) - 5, MeasureTextEx(GetFontDefault(), "Singleplayer", 50, 2).x + 25, MeasureTextEx(GetFontDefault(), "Singleplayer", 50, 2).y + 10 };
        Rectangle rec2 = { ((GetScreenWidth() / 2.0f) - (MeasureTextEx(GetFontDefault(), "Multiplayer", 50, 2).x / 2)) - 10, (GetScreenHeight() * 0.45f) - 5, MeasureTextEx(GetFontDefault(), "Multiplayer", 50, 2).x + 25, MeasureTextEx(GetFontDefault(), "Multiplayer", 50, 2).y + 10 };
        Rectangle rec3 = { ((GetScreenWidth() / 2.0f) - (MeasureTextEx(GetFontDefault(), "Singleplayer", 50, 2).x / 2)) - 10, (GetScreenHeight() * 0.35f) - 5, MeasureTextEx(GetFontDefault(), "Singleplayer", 50, 2).x + 25, MeasureTextEx(GetFontDefault(), "Singleplayer", 50, 2).y + 10 };

        switch (windowType) {
        case 0:
            if (CheckCollisionPointRec(GetMousePosition(), rec1)) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    windowType = 1;
                }
                DrawRectangleRec(rec1, GREEN);
                DrawTextEx(GetFontDefault(), "Singleplayer", { (GetScreenWidth() / 2.0f) - (MeasureTextEx(GetFontDefault(), "Singleplayer", 50, 2).x / 2), GetScreenHeight() * 0.35f }, 50, 2, WHITE);
            }
            else {
                DrawRectangleRec(rec1, WHITE);
                DrawTextEx(GetFontDefault(), "Singleplayer", { (GetScreenWidth() / 2.0f) - (MeasureTextEx(GetFontDefault(), "Singleplayer", 50, 2).x / 2), GetScreenHeight() * 0.35f }, 50, 2, GREEN);
            }

            if (CheckCollisionPointRec(GetMousePosition(), rec2)) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    windowType = 2;
                    connected = false;
                    ANet::ConnectToServer("127.0.0.1", 4545);
                }
                DrawRectangleRec(rec2, GREEN);
                DrawTextEx(GetFontDefault(), "Multiplayer", { (GetScreenWidth() / 2.0f) - (MeasureTextEx(GetFontDefault(), "Multiplayer", 50, 2).x / 2), GetScreenHeight() * 0.45f }, 50, 2, WHITE);
            }
            else {
                DrawRectangleRec(rec2, WHITE);
                DrawTextEx(GetFontDefault(), "Multiplayer", { (GetScreenWidth() / 2.0f) - (MeasureTextEx(GetFontDefault(), "Multiplayer", 50, 2).x / 2), GetScreenHeight() * 0.45f }, 50, 2, GREEN);
            }
            break;
        case 1:

            if (CheckCollisionPointRec(GetMousePosition(), rec3)) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    Scene* scene = engine->sceneMgr->LoadScene<TestScene>();
                }
                DrawRectangleRec(rec3, GREEN);
                DrawTextEx(GetFontDefault(), "Singleplayer", { (GetScreenWidth() / 2.0f) - (MeasureTextEx(GetFontDefault(), "Singleplayer", 50, 2).x / 2), GetScreenHeight() * 0.35f }, 50, 2, WHITE);
            }
            else {
                DrawRectangleRec(rec3, WHITE);
                DrawTextEx(GetFontDefault(), "Singleplayer", { (GetScreenWidth() / 2.0f) - (MeasureTextEx(GetFontDefault(), "Singleplayer", 50, 2).x / 2), GetScreenHeight() * 0.35f }, 50, 2, GREEN);
            }

            break;

        case 2:

            if (ANet::Connected()) {
                connected = true;


                Vector2 movement = { 0 };
                float speed = 200;

                // see what axes we move in
                if (IsKeyDown(KEY_UP))
                    movement.y -= speed;
                if (IsKeyDown(KEY_DOWN))
                    movement.y += speed;

                if (IsKeyDown(KEY_LEFT))
                    movement.x -= speed;
                if (IsKeyDown(KEY_RIGHT))
                    movement.x += speed;


                ANet::UpdateLocalPlayer(&movement, GetFrameTime());
            }
            else if (connected) {
                ANet::ConnectToServer("127.0.0.1", 4545);
                connected = false;
            }

            ANet::Update(GetTime(), GetFrameTime());

            if (!ANet::Connected())
            {
                // we are not connected, so just wait until we are, this can take some time
                DrawText("Connecting", 0, 20, 20, RED);
            }
            else
            {
                // we are connected, and know what our player ID is, so show that to the player in our color
                DrawText(TextFormat("Player %d", ANet::GetLocalPlayerId()), 0, 20, 20, GREEN);
                // we are connected, and know what our player ID is, so show that to the player in our color
                DrawText("Connected to server!", 0, 40, 20, GREEN);

                for (int i = 0; i < MAX_PLAYERS; i++)
                {
                    Vector2 pos = { 0 };
                    if (ANet::GetPlayerPos(i, &pos))
                    {
                        DrawRectangle((int)pos.x, (int)pos.y, 10, 10, RED);
                    }
                }
            }

            break;
        }
            

        EndDrawing();
    }

    void scene_unload() {
        ANet::Disconnect();
        originalUnload();
    }

    bool connected = false;
    int windowType = 0;

};

class Boot : public Scene {
public:
    void scene_init() {


        originalSetup();

        InitMadeWithRaylib();

        engL = LoadTexture("resources/icon/UGLOGO.png");
        engF = LoadFontEx("resources/fonts/TiltWarp-Regular.ttf", 100, 0, 250);
        egF = LoadFontEx("resources/fonts/luckiest.ttf", 100, 0, 250);
    }

    void scene_update(aurCamera* cam) {

        

        BeginDrawing();

        ClearBackground(RAYWHITE);

        if (fadeElapsed >= 0 && fadeElapsed < 1 && mwrOpen) {
            MadeWithRaylib({ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f }, 7.5, Fade(BLACK, fadeElapsed), BLANK);
            fadeElapsed += 0.01;
            if (fadeElapsed >= 1) {
                if (mwrOpen) {
                    printf("mwropen\n");
                    fadeElapsed = 1;
                    mwrOpen = false;
                }
            }
        }

        if (!mwrOpen && mwr) {
            if (mwrTimeElapsed < mwrTime) {
                MadeWithRaylib({ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f }, 7.5, BLACK, BLANK);
                mwrTimeElapsed += 0.005;
            }
            else {
                if (fadeElapsed <= 1 && fadeElapsed > 0) {
                    MadeWithRaylib({ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f }, 7.5, Fade(BLACK, fadeElapsed), BLANK);
                    fadeElapsed -= 0.01;
                }
                else if (fadeElapsed <= 0) {
                    fadeElapsed = 0.0;
                    mwr = false;
                    engOpen = true;
                    eng = true;
                }
            }
        }

        if (fadeElapsed >= 0 && fadeElapsed < 1 && engOpen) {
            DrawTexturePro(engL, { 0,0, (float)engL.width, (float)engL.height }, { 0,0, 300,300 }, { -500, -100 }, 0, Fade(WHITE, fadeElapsed));
            DrawTextEx(engF, "UntitledGames", { (GetScreenWidth() / 2.0f) - (MeasureTextEx(engF, "UntitledGames", 100, 2).x / 2), (GetScreenHeight() / 2.0f) + 25 }, 100, 2, Fade(BLACK, fadeElapsed));
            DrawTextEx(engF, "in collaboration with", { (GetScreenWidth() / 2.0f) - (MeasureTextEx(engF, "in collaboration with", 50, 1).x / 2), (GetScreenHeight() / 2.0f) + 110 }, 50, 1, Fade(BLACK, fadeElapsed));
            DrawTextEx(engF, "EggKnyte", { (GetScreenWidth() / 2.0f) - (MeasureTextEx(engF, "EggKnyte", 100, 2).x / 2), (GetScreenHeight() / 2.0f) + 150 }, 100, 2, Fade(BLUE, fadeElapsed));
            fadeElapsed += 0.01;
            if (fadeElapsed >= 1) {
                if (engOpen) {
                    printf("engOpen\n");
                    fadeElapsed = 1;
                    engOpen = false;
                }
            }
        }

        if (!engOpen &&  eng) {
            if (engTimeElapsed < engTime) {
                DrawTexturePro(engL, { 0,0, (float)engL.width, (float)engL.height }, { 0,0, 300,300 }, { -500, -100}, 0, WHITE);
                DrawTextEx(engF, "UntitledGames", { (GetScreenWidth() / 2.0f)-(MeasureTextEx(engF, "UntitledGames", 100, 2).x/2), (GetScreenHeight() / 2.0f)+25}, 100, 2, BLACK);
                DrawTextEx(engF, "in collaboration with", { (GetScreenWidth() / 2.0f) - (MeasureTextEx(engF, "in collaboration with", 50, 1).x / 2), (GetScreenHeight() / 2.0f) + 110 }, 50, 1, Fade(BLACK, 1));
                DrawTextEx(engF, "EggKnyte", { (GetScreenWidth() / 2.0f) - (MeasureTextEx(engF, "EggKnyte", 100, 2).x / 2), (GetScreenHeight() / 2.0f) + 150 }, 100, 2, Fade(BLUE, 1));
                //engTimeElapsed += 0.0000001;
                engTimeElapsed += 0.005;
            }
            else {
                if (fadeElapsed <= 1 && fadeElapsed > 0) {
                    DrawTexturePro(engL, { 0,0, (float)engL.width, (float)engL.height }, { 0,0, 300,300 }, { -500, -100 }, 0, Fade(WHITE, fadeElapsed));
                    DrawTextEx(engF, "UntitledGames", { (GetScreenWidth() / 2.0f) - (MeasureTextEx(engF, "UntitledGames", 100, 2).x / 2), (GetScreenHeight() / 2.0f) + 25 }, 100, 2, Fade(BLACK, fadeElapsed));
                    DrawTextEx(engF, "in collaboration with", { (GetScreenWidth() / 2.0f) - (MeasureTextEx(engF, "in collaboration with", 50, 1).x / 2), (GetScreenHeight() / 2.0f) + 110 }, 50, 1, Fade(BLACK, fadeElapsed));
                    DrawTextEx(engF, "EggKnyte", { (GetScreenWidth() / 2.0f) - (MeasureTextEx(engF, "EggKnyte", 100, 2).x / 2), (GetScreenHeight() / 2.0f) + 150 }, 100, 2, Fade(BLUE, fadeElapsed));
                    fadeElapsed -= 0.01;
                }
                else if (fadeElapsed <= 0) {
                    fadeElapsed = 1;
                    eng = false;
                    Scene* scene = engine->sceneMgr->LoadScene<HomePage>();
                }
            }
        }

        

        EndDrawing();


        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            fadeElapsed = 1;
            eng = false;
            Scene* scene = engine->sceneMgr->LoadScene<HomePage>();
        }

    }

    void scene_unload() {
        UnloadTexture(engL);
        UnloadFont(engF);
        UnloadFont(egF);
        originalUnload();
    }

    float mwrTimeElapsed = 0.0, mwrTime = 1.0;
    float engTimeElapsed = 0.0, engTime = 1.0;
    float fadeElapsed = 0.0;
    bool mwr = true, eng, mwrOpen = true, engOpen;
    Texture2D engL;
    Font engF;
    Font egF;
};

inline void Player::Update(Entity* entity) {

    horizontalAxis = InputManager::GetInputAxis(LEFT);
    float verticalAxis = InputManager::GetInputAxis(UP);



    if (entity->GetComponent<Rigidbody2D>()) {

        jumpDelayAmt = 1.0;

        Rigidbody2D* rb = entity->GetComponent<Rigidbody2D>();

        int jumpButton = InputManager::GetInput(BUTTONDOWN);

        float upVelocity = jumpButton * (3 * 100);

        if (!(jumpDelay < jumpDelayAmt)) {
            jumping = false;
            jumpDelay = 0;
        }

        if (jumping == true) {
            jumpButton = 0;
            jumpDelay += 0.1;
        }

        if (jumpButton == 0) {
            upVelocity = rb->body->GetLinearVelocity().y;
        }
        else {
            jumping = true;
        }
        //rb->body->ApplyLinearImpulse({ 0,upVelocity }, rb->body->GetWorldCenter(), true);

        b2Vec2 vel = rb->body->GetLinearVelocity();
        float desiredVel = 0;

        desiredVel = horizontalAxis * speed;

        float velChange = desiredVel - vel.x;
        float impulse = rb->body->GetMass() * velChange; //disregard time factor
        float grav = rb->body->GetMass() * engine->physicsMgr->gravity.y; //disregard time factor


        if (jumpButton == 0) {
            upVelocity = rb->body->GetLinearVelocity().y;
        }

        rb->body->SetLinearVelocity({ horizontalAxis * -(speed * 100), upVelocity });

        //rb->body->ApplyLinearImpulse(b2Vec2(-(impulse*100), 0), rb->body->GetWorldCenter(), true);




        //printf("%.2f\n", horizontalAxis);

    }
    else {

        entity->localPosition = Vector3Add(entity->localPosition, { horizontalAxis * speed,verticalAxis * speed,0 });

        //printf("%.2f\n", horizontalAxis);
    }

    if (horizontalAxis > 0) {
        AnimatedRenderer2D* renderer = entity->GetComponent<AnimatedRenderer2D>();
        if (renderer) {
            renderer->srcRect.width = abs(renderer->srcRect.width);
        }
    }
    else if (horizontalAxis < 0) {
        AnimatedRenderer2D* renderer = entity->GetComponent<AnimatedRenderer2D>();
        if (renderer) {
            renderer->srcRect.width = -abs(renderer->srcRect.width);
        }
    }

    if (abs(horizontalAxis) > 0) {
        AnimatedRenderer2D* renderer = entity->GetComponent<AnimatedRenderer2D>();
        if (renderer) {
            if (wb) {
                if (abs(horizontalAxis) > 0.5) {
                    renderer->srcRect.x = 32 * 1;
                    renderer->srcRect.y = 32 * 0;
                }
                else {
                    renderer->srcRect.x = 32 * 3;
                    renderer->srcRect.y = 32 * 0;
                }
            }
            else {
                if (abs(horizontalAxis) > 0.5) {
                    if (wbDelay >= 0.00) {
                        renderer->srcRect.x = 32 * 2;
                        renderer->srcRect.y = 32 * 0;
                    }
                    if (wbDelay >= 2.50) {
                        renderer->srcRect.x = 32 * 1;
                        renderer->srcRect.y = 32 * 1;
                    }
                    if (wbDelay >= 5.00) {
                        renderer->srcRect.x = 32 * 1;
                        renderer->srcRect.y = 32 * 0;
                    }
                    if (wbDelay >= 7.50) {
                        renderer->srcRect.x = 32 * 1;
                        renderer->srcRect.y = 32 * 1;
                    }
                }
                else {
                    if (wbDelay >= 0.00) {
                        renderer->srcRect.x = 32 * 3;
                        renderer->srcRect.y = 32 * 0;
                    }
                    if (wbDelay >= 2.50) {
                        renderer->srcRect.x = 32 * 1;
                        renderer->srcRect.y = 32 * 1;
                    }
                    if (wbDelay >= 5.00) {
                        renderer->srcRect.x = 32 * 0;
                        renderer->srcRect.y = 32 * 1;
                    }
                    if (wbDelay >= 7.50) {
                        renderer->srcRect.x = 32 * 1;
                        renderer->srcRect.y = 32 * 1;
                    }
                }
            }
            wbDelay += 0.25f;
            wb = false;

            if (wbDelay >= 10.0f) {
                wbDelay = 0;
                //wb = !wb;
            }
        }
    }
    else {
        AnimatedRenderer2D* renderer = entity->GetComponent<AnimatedRenderer2D>();
        if (renderer) {
            renderer->srcRect.x = 32 * 1;
            renderer->srcRect.y = 32 * 1;
        }
    }

    if (InputManager::GetInput(LEFTSTICK) > 0) {
        AnimatedRenderer2D* renderer = entity->GetComponent<AnimatedRenderer2D>();
        if (renderer) {
            renderer->srcRect.x = 32 * 0;
            renderer->srcRect.y = 32 * 0;
            renderer->rec.height = 80;
        }
    }
    else {
        AnimatedRenderer2D* renderer = entity->GetComponent<AnimatedRenderer2D>();
        if (renderer) {
            renderer->rec.height = 100;
        }
    }

    /*

    if (direction == true) {
        health += 0.005f;
        //direction = false;
    }
    if (direction == false) {
        health -= 0.005f;
        //direction = true;
    }

    if (health <= 0.0f) {
        direction = true;
    }
    if (health >= 1.0f) {
        direction = false;
    }

    */

    health = Clamp(health, 0.0f, 1.0f);

    //printf("%.3f\n", health);

    if (health > 0.59 && health < 0.69) {
        entity->scene->entityData->find("heart1")->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 5, 18 * 2, 18, 18 };
    }
    if (health <= 0.59) {
        entity->scene->entityData->find("heart1")->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 6, 18 * 2, 18, 18 };
    }

    if (health >= 0.69) {
        entity->scene->entityData->find("heart1")->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 4, 18 * 2, 18, 18 };
    }

    if (health > 0.39 && health < 0.49) {
        entity->scene->entityData->find("heart2")->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 5, 18 * 2, 18, 18 };
    }
    if (health <= 0.39) {
        entity->scene->entityData->find("heart2")->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 6, 18 * 2, 18, 18 };
    }

    if (health >= 0.49) {
        entity->scene->entityData->find("heart2")->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 4, 18 * 2, 18, 18 };
    }

    if (health > 0.00 && health < 0.19) {
        entity->scene->entityData->find("heart3")->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 5, 18 * 2, 18, 18 };
    }
    if (health <= 0.00) {
        entity->scene->entityData->find("heart3")->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 6, 18 * 2, 18, 18 };
    }

    if (health >= 0.19) {
        entity->scene->entityData->find("heart3")->GetComponent<AnimatedRenderer2D>()->srcRect = { 18 * 4, 18 * 2, 18, 18 };
    }

    Vector2 mousePos = {};
    mousePos.x = (float)(int)GetScreenToWorld2D(GetMousePosition(), *cam).x;
    mousePos.y = (float)(int)GetScreenToWorld2D(GetMousePosition(), *cam).y;

    if (Vector2Distance({ entity->globalPosition.x, entity->globalPosition.y }, mousePos) <= playerRange) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            terrainGenerator->RemoveTile(mousePos.x, mousePos.y);
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            terrainGenerator->CheckTile(selectedTile, mousePos.x, mousePos.y);
        }
    }

}