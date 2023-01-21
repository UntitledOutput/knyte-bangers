#include "raylibCpp/raylib-cpp.hpp"
#include "cJSON/cJSON.h"

#include <typeinfo> 
#include <memory>

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

class Component {
public:

    static const std::size_t                    Type;
    virtual bool                                IsClassType(const std::size_t classType) const;

public:

    virtual                                ~Component() = default;
    Component(const char*&& initialValue): value(initialValue) {};

    Component() = default;

public:

    const char*                             value = "uninitialized";
private:
    friend class ComponentHolder;
    friend class Entity;

    virtual void Update(Entity* Entity) = default;
};

class ComponentHolder {
private:

    friend class Entity;

    ComponentHolder() = default;
public:
    std::vector< std::unique_ptr<Component> > components;
    template <class CompType, typename... Args>
    void Attach(Args&&... params);

    void Update(Entity* Entity);
};

class Entity {
public:
    Entity() = default;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;

    ComponentHolder* componentHolder;

    template< class ComponentType, typename... Args >
    void AddComponent(Args&&... params);

    template< class ComponentType >
    ComponentType& GetComponent();

    template< class ComponentType >
    bool RemoveComponent();

    template< class ComponentType >
    std::vector< ComponentType* > GetComponents();

    template< class ComponentType >
    int RemoveComponents();
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