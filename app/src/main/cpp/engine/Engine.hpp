//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#ifndef OXYOUS_2026_ENGINE_HPP
#define OXYOUS_2026_ENGINE_HPP

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include "../includes.hpp"
#include "../system/OGSingleton.hpp"
#include "../render/vulkan/pipelines/IRenderPipeline.hpp"
#include "input/Input.hpp"
#include "actors/OGActor.hpp"
#include "actors/OGCamera.hpp"
#include "../DataStructures.hpp"
#include "GameView.hpp"
#include "algorithms/OGBVH.hpp"
#include "engine/algorithms/OGOctree.hpp"

class Engine {
public:
    Engine() = default;

    virtual ~Engine() = default;

    JNIEnv *getJniEnv();

public:
    /* Initialize Game Engine */
    virtual bool initialize(android_app *app);

    /* Destroy Game Engine */
    virtual void destroy();

    /*  Update Game Engine */
    virtual void update(float deltaTime);

    /*  Render Game Engine */
    virtual void render();

    /* handle input */
    virtual void handleInput();

    /* */
    virtual void prepareInput();

    virtual bool postInitialize();

    virtual glm::mat4 preRotation();

    virtual ThumbStick *getThumbStick(ThumbStickType type);

public:
    template<typename T, typename... TArgs>
    T *createPipeline(const std::string &name, TArgs &&... args) {
        T *renderPipeline(new T(std::forward<TArgs>(args)...));

        std::unique_ptr<IRenderPipeline> uPtr{renderPipeline};

        m_pipelines[name] = std::move(uPtr);

        m_pipelines[name]->initialize();

        return (T *) m_pipelines[name].get();
    }

    template<typename T>
    T *getPipeline(const std::string &name) {
        auto it = m_pipelines.find(name);
        if (it == m_pipelines.end()) {
            return nullptr;
        }

        return dynamic_cast<T *>(it->second.get());
    }

    /* Get Android App */
    android_app *getApp() {
        return m_app;
    }

    glm::mat4 getCameraView() {
        if (!m_gameModeFly) {
            const auto player = dynamic_cast<OGPlayerActor *>(GAME_VIEW->getActivePlayer().get());
            if (player) {
                return player->getViewMatrix();
            }
        }

        return m_camera->getViewMatrix();
    }

    glm::mat4 getCameraProjection() {
        return m_camera->getProjectionMatrix();
    }

    glm::mat4 getFlyingCameraProjection() {
        return m_camera->getProjectionMatrix();
    }

    glm::mat4 getFlyingCameraView() {
        return m_camera->getViewMatrix();
    }

    glm::vec3 getCameraPosition() {

        if (m_gameModeFly) {
            return m_camera->getPosition();
        } else {
            const auto player = dynamic_cast<OGPlayerActor *>(GAME_VIEW->getActivePlayer().get());
            if (player) {
                return player->getCameraPosition();
            }
        }

        return m_camera->getPosition();
    }

    void setCameraProjection(glm::mat4 projection) {
        if (!m_gameModeFly) {
            const auto player = dynamic_cast<OGPlayerActor *>(GAME_VIEW->getActivePlayer().get());
            if (player) {
                player->setProjectionMatrix(projection);
            }
        }
        m_camera->setProjectionMatrix(projection);
    }

    void setCameraPosition(glm::vec3 position) {
        if (!m_gameModeFly) {
            const auto player = dynamic_cast<OGPlayerActor *>(GAME_VIEW->getActivePlayer().get());
            if (player) {
                player->setTranslation(position);
            }
        }
        m_camera->setTranslation(position);
    }

    Frustum getCameraFrustum();

    void setSharedCSMData(const CSMData &data) { m_sharedCSMData = data; }

    CSMData getSharedCSMData() const { return m_sharedCSMData; }

    void setCurrentFrame(uint32_t frame) { m_currentFrame = frame; }

    uint32_t getCurrentFrame() const { return m_currentFrame; }

    void setGameModeFly(bool fly) { m_gameModeFly = fly; }

    bool isGameModeFly() const { return m_gameModeFly; }

    void setDemoCulling(bool cull) {
        m_cullView = cull;
    }

    bool isDemoCulling() {
        return m_cullView;
    }

    OGCamera *getCamera() {
        return m_camera;
    }

    void setCamera(OGCamera *camera) {
        m_camera = camera;
    }

    Renderer *getRenderer() { return m_renderer; }

    void setRenderer(Renderer *renderer) { m_renderer = renderer; }

    bool isExecuting() {
        return m_isExecuting;
    }

    /** Build BVH from polygon list */
    void computeCollisionBHV(const std::vector<OGPolygon> &polygons);

    /** Get Possible Polygon intersection */
    void
    getCapsuleIntersectionByBHV(const CapsuleVolume &capsule, std::vector<OGPolygon> &polygons);

    /** Get Possible Polygons intersection with sphere */
    void getSphereIntersectionByBHV(const SphereVolume &sphere, std::vector<OGPolygon> &polygons);

    /** */
    void getObbIntersectionByBHV(const OBBVolume &obb, std::vector<OGPolygon> &polygons);

    /** Get Possible Polygons intersection with segment */
    void getSegmentIntersectionByBHV(const OGSegment &segment, std::vector<OGPolygon> &polygons);

    /** Build BVH Static objects*/
    void buildStaticBVH(std::vector<AABBVolume> &primitives);

    /** Get static entities intersecting with the given frustum */
    void
    getStaticFrustumIntersectionByBVH(const Frustum &frustum, std::vector<AABBVolume> &entities);

    /**  */
    void
    getStaticIntersectionByBVH(const AABBVolume& volume, std::vector<AABBVolume>& entities);

    /**  */
    void buildLevelOctree();

    /** Insert a volume into the static octree */
    void insertStaticOctree(const AABBVolume& volume);

    /** Query the octree with a frustum */
    void queryOctreeFrustum(const Frustum& frustum, std::vector<AABBVolume>& volumes);

    /** Query the octree with an AABB volume */
    void queryOctree(const AABBVolume& volume, std::vector<AABBVolume>& volumes);

    /** Update visible objects for the current frame */
    void updateVisibleObjects();

    /** Get cached visible objects for the main camera */
    const std::vector<OGEntity*>& getCachedVisibleObjects() const { return m_visibleObjects; }

protected:
    bool m_isExecuting = false;
    Renderer *m_renderer = nullptr;
    OGCamera *m_camera = nullptr;
    std::unordered_map<std::string, std::unique_ptr<IRenderPipeline>> m_pipelines;
    android_app *m_app;
    Input m_input;
    CSMData m_sharedCSMData;
    uint32_t m_currentFrame = 0;
    bool m_gameModeFly = true;
    bool m_cullView = false;
    std::unique_ptr<BVH> m_collisionBHV;
    std::unique_ptr<OGBVH<AABBVolume>> m_staticBVH;
    std::unique_ptr<OGOctree<AABBVolume>> m_staticOctree;

    std::vector<OGEntity*> m_visibleObjects;
};

#define ENGINE OGSingleton<Engine>::getInstance()


#endif //OXYOUS_2026_ENGINE_HPP
