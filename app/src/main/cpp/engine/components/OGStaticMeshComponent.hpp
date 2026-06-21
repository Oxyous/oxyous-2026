//
// Created by Mr Steven J Baldwin on 21/06/2026.
//

#ifndef OXYOUS_2026_OGSTATICMESHCOMPONENT_HPP
#define OXYOUS_2026_OGSTATICMESHCOMPONENT_HPP


#include <utility>

#include "../entity/OGEntity.hpp"
#include "../../render/meshes/GPUStaticMesh.hpp"
#include "../../resources/GPUTextureResource.hpp"

class OGStaticMeshComponent : public OGComponent {
public:
    OGStaticMeshComponent() : OGComponent() {
        m_owner = nullptr;
    }
public:
    /* initialize static mesh component*/
    void initialize() override;

    /* update static mesh component*/
    void update(double deltaTime) override;

    /* destroy static mesh component*/
    void destroy() override;

    /* render static mesh component*/
    void render(VkCommandBuffer &commandBuffer) override;
public:
    /* set mesh resource */
    void setMeshResource(std::shared_ptr<GPUStaticMeshResource> mesh);

    /* set texture resource */
    void setTextureResource(TEXTURE_SLOT slot, std::shared_ptr<GPUTextureResource> texture);

protected:
    std::shared_ptr<GPUStaticMeshResource> m_mesh;
    std::unordered_map<TEXTURE_SLOT, std::shared_ptr<GPUTextureResource>> m_textures;
};


#endif //OXYOUS_2026_OGSTATICMESHCOMPONENT_HPP
