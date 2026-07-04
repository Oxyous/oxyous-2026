//
// Created by Mr Steven J Baldwin on 29/06/2026.
//

#ifndef OXYOUS_2026_RENDERHELPER_HPP
#define OXYOUS_2026_RENDERHELPER_HPP

#include "../../includes.hpp"
#include "../../DataStructures.hpp"

class RenderHelper {
public:
    inline static void extractPerspectiveNearFar(
            const glm::mat4 &projectionMatrix,
            float &nearPlane,
            float &farPlane
    ) {
        // GLM perspective matrix with GLM_FORCE_DEPTH_ZERO_TO_ONE:
        // m22 = f / (n - f), m32 = (f * n) / (n - f)
        const float m22 = projectionMatrix[2][2];
        const float m32 = projectionMatrix[3][2];

        if (std::abs(m22) < 1e-6f || std::abs(m22 + 1.0f) < 1e-6f) {
            nearPlane = 0.1f;
            farPlane = 1000.0f;
            return;
        }

        nearPlane = m32 / m22;
        farPlane = m32 / (m22 + 1.0f);

        if (!(nearPlane > 0.0f) || !(farPlane > nearPlane)) {
            nearPlane = 0.1f;
            farPlane = 1000.0f;
        }
    }

    /* Get Frustum Corners */
    inline static std::array<glm::vec4, 8> getFrustumCornersWorldSpace(
            const glm::mat4 &projectionMatrix,
            const glm::mat4 &viewMatrix,
            float nearPlane,
            float farPlane
    ) {
        glm::mat4 invProj = glm::inverse(projectionMatrix);
        glm::mat4 invView = glm::inverse(viewMatrix);

        std::array<glm::vec4, 8> frustumCorners;
        for (unsigned int i = 0; i < 8; ++i) {
            glm::vec4 pt = invProj * glm::vec4(
                    (i & 1) ? 1.0f : -1.0f,
                    (i & 2) ? 1.0f : -1.0f,
                    (i & 4) ? 1.0f : 0.0f, // ZERO_TO_ONE
                    1.0f
            );
            frustumCorners[i] = pt / pt.w;
        }

        float fNear = frustumCorners[0].z;
        float fFar  = frustumCorners[4].z;

        std::array<glm::vec4, 8> subCorners;
        for (unsigned int i = 0; i < 4; ++i) {
            float tNear = (-nearPlane - fNear) / (fFar - fNear);
            float tFar  = (-farPlane - fNear) / (fFar - fNear);

            subCorners[i]   = invView * glm::mix(frustumCorners[i], frustumCorners[i+4], tNear);
            subCorners[i+4] = invView * glm::mix(frustumCorners[i], frustumCorners[i+4], tFar);
        }

        return subCorners;
    }

    /* Compute Cascade Splits */
    inline static std::array<float, 4> computeCascadeSplits(
            float nearPlane,
            float farPlane,
            float lambda)
    {
        std::array<float, 4> splits{};

        const float clipRange = farPlane - nearPlane;

        for (uint32_t i = 0; i < 4; ++i)
        {
            float p = float(i + 1) / float(4);

            float logSplit =
                    nearPlane * std::pow(farPlane / nearPlane, p);

            float linearSplit =
                    nearPlane + clipRange * p;

            float split =
                    glm::mix(linearSplit, logSplit, lambda);

            splits[i] = split;
        }

        return splits;
    }

    /* computes Cascade Light Matrix */
    inline static glm::mat4 computeCascadeLightMatrix(
            const std::array<glm::vec4, 8> &frustumCorners,
            const glm::vec3 &lightDirection,
            uint32_t shadowMapSize,
            bool stabilize = true,
            float cascadeIndex = 0.0f,
            float cascadeCount = 4.0f
    ) {
        // 1. Calculate center of the frustum slice
        glm::vec3 center(0.0f);
        for (const auto &corner: frustumCorners) center += glm::vec3(corner);
        center /= 8.0f;

        // 2. Calculate radius of bounding sphere
        float radius = 0.0f;
        for (const auto &corner: frustumCorners) radius = std::max(radius, glm::length(glm::vec3(corner) - center));

        // Round radius to avoid flickering as frustum shape changes
        radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 lightDir = glm::normalize(lightDirection);
        glm::vec3 up = (std::abs(lightDir.y) > 0.999f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);

        // 3. Create a stable view matrix that only rotates
        // Using lightDir as the "eye" and origin as "center" just for orientation
        glm::mat4 rotationMatrix = glm::lookAt(lightDir, glm::vec3(0.0f), up);

        // 4. Project sphere center into light-view space
        glm::vec3 centerLS = glm::vec3(rotationMatrix * glm::vec4(center, 1.0f));

        if (stabilize)
        {
            // 5. Snap the center in light-view space to the texel grid
            float texelSize = (radius * 2.0f) / (float)shadowMapSize;
            centerLS.x = std::floor(centerLS.x / texelSize) * texelSize;
            centerLS.y = std::floor(centerLS.y / texelSize) * texelSize;
        }

        // 6. Final eye position: Move back from the snapped center along light direction
        glm::mat4 invRotation = glm::inverse(rotationMatrix);
        glm::vec3 snappedCenterWS = glm::vec3(invRotation * glm::vec4(centerLS, 1.0f));

        glm::vec3 eye = snappedCenterWS + lightDir * radius * 2.0f;
        glm::mat4 lightView = glm::lookAt(eye, snappedCenterWS, up);

        // 7. Ortho projection covering the bounding sphere
        // Near=0, Far=radius*10 to capture objects far behind camera
        glm::mat4 lightProj = glm::ortho(-radius, radius, -radius, radius, 0.0f, radius * 10.0f);

        // Vulkan Y-flip
        lightProj[1][1] *= -1;

        return lightProj * lightView;
    }

    /* Compute CSM Matrices */
    inline static CSMData computeCSMMatrices(
            const glm::mat4 &projectionMatrix,
            const glm::mat4 &viewMatrix,
            float nearPlane,
            float farPlane,
            uint32_t shadowMapSize,
            const glm::vec3& lightDirection
            ) {

        CSMData data;
        auto splits = computeCascadeSplits(nearPlane, farPlane, 0.98f);

        float lastSplit = nearPlane;

        for(uint32_t i = 0; i < 4; ++i) {
            float cascadeNear = lastSplit;
            float cascadeFar = splits[i];

            auto frustumCorners = getFrustumCornersWorldSpace(projectionMatrix, viewMatrix, cascadeNear, cascadeFar);

            auto lightMatrix = computeCascadeLightMatrix(frustumCorners, lightDirection, shadowMapSize, true, (float)i, 4.0f);

            data.lightProjection[i] = lightMatrix;
            data.cascadeSplits[i] = cascadeFar;

            lastSplit = cascadeFar;
        }

        return data;
    }
};


#endif //OXYOUS_2026_RENDERHELPER_HPP
