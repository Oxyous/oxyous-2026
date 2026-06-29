//
// Created by Mr Steven J Baldwin on 29/06/2026.
//

#ifndef OXYOUS_2026_RENDERHELPER_HPP
#define OXYOUS_2026_RENDERHELPER_HPP

#include "../../includes.hpp"
#include "../../DataStructures.hpp"

class RenderHelper {
public:
    /* Get Frustum Corners */
    inline static std::array<glm::vec4, 8> getFrustumCornersWorldSpace(
            const glm::mat4 &projectionMatrix,
            const glm::mat4 &viewMatrix,
            float nearPlane,
            float farPlane
    ) {
        glm::mat4 invViewProj = glm::inverse(projectionMatrix * viewMatrix);

        // We need the corners of the full frustum in world space first
        std::array<glm::vec4, 8> corners;
        for (unsigned int i = 0; i < 8; ++i) {
            glm::vec4 corner = invViewProj * glm::vec4(
                (i & 1) ? 1.0f : -1.0f,
                (i & 2) ? 1.0f : -1.0f,
                (i & 4) ? 1.0f : 0.0f, // NDC Z: 0 is near, 1 is far for Vulkan
                1.0f
            );
            corners[i] = corner / corner.w;
        }

        // Now interpolate between near and far corners to get the sub-frustum
        // Note: linear interpolation in world space isn't perfectly accurate for perspective,
        // but for CSM splits it's usually acceptable if lambda is high.
        // A better way is to do it in view space.

        glm::mat4 invProj = glm::inverse(projectionMatrix);
        std::array<glm::vec4, 8> viewCorners;
        for (unsigned int i = 0; i < 8; ++i) {
            glm::vec4 pt = invProj * glm::vec4(
                (i & 1) ? 1.0f : -1.0f,
                (i & 2) ? 1.0f : -1.0f,
                (i & 4) ? 1.0f : 0.0f,
                1.0f
            );
            viewCorners[i] = pt / pt.w;
        }

        // Find full frustum depth range in view space
        float fullNear = viewCorners[0].z; // usually negative in GLM
        float fullFar = viewCorners[4].z;

        // Cascade planes are distances from camera (positive)
        // In view space, Z is usually negative (pointing away)
        float zNear = -nearPlane;
        float zFar = -farPlane;

        std::array<glm::vec4, 8> subCorners;
        glm::mat4 invView = glm::inverse(viewMatrix);

        for (unsigned int i = 0; i < 4; ++i) {
            // Interpolate near corners
            float lerpNear = (zNear - fullNear) / (fullFar - fullNear);
            glm::vec4 vNear = glm::mix(viewCorners[i], viewCorners[i+4], lerpNear);
            subCorners[i] = invView * vNear;

            // Interpolate far corners
            float lerpFar = (zFar - fullNear) / (fullFar - fullNear);
            glm::vec4 vFar = glm::mix(viewCorners[i], viewCorners[i+4], lerpFar);
            subCorners[i+4] = invView * vFar;
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
            bool stabilize = true
    ) {
        glm::vec3 center(0.0f);

        for (const auto &corner: frustumCorners) {
            center += glm::vec3(corner);
        }

        center /= float(frustumCorners.size());

        float radius = 0.0f;

        for (const auto &corner: frustumCorners) {
            float dist = glm::length(glm::vec3(corner) - center);
            radius = std::max(radius, dist);
        }

        radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 lightDir = glm::normalize(lightDirection);

        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        if (std::abs(glm::dot(lightDir, up)) > 0.9999f) {
            up = glm::vec3(0.0f, 0.0f, 1.0f);
        }

        // lightDirection is direction TO the light
        glm::vec3 lightPos = center - lightDir * radius * 2.0f;

        glm::mat4 lightView = glm::lookAt(lightPos, center, up);

        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float minZ = std::numeric_limits<float>::max();

        float maxX = -std::numeric_limits<float>::max();
        float maxY = -std::numeric_limits<float>::max();
        float maxZ = -std::numeric_limits<float>::max();

        for (const auto &corner: frustumCorners) {
            glm::vec4 lightSpaceCorner = lightView * corner;
            minX = std::min(minX, lightSpaceCorner.x);
            minY = std::min(minY, lightSpaceCorner.y);
            minZ = std::min(minZ, lightSpaceCorner.z);
            maxX = std::max(maxX, lightSpaceCorner.x);
            maxY = std::max(maxY, lightSpaceCorner.y);
            maxZ = std::max(maxZ, lightSpaceCorner.z);
        }

        float extentX = maxX - minX;
        float extentY = maxY - minY;
        float extent = std::max(extentX, extentY);

        float centerX = (minX + maxX) * 0.5f;
        float centerY = (minY + maxY) * 0.5f;

        minX = centerX - extent * 0.5f;
        maxX = centerX + extent * 0.5f;
        minY = centerY - extent * 0.5f;
        maxY = centerY + extent * 0.5f;

        if (stabilize)
        {
            const float unitsPerTexel = extent / float(shadowMapSize);

            minX = std::floor(minX / unitsPerTexel) * unitsPerTexel;
            minY = std::floor(minY / unitsPerTexel) * unitsPerTexel;

            maxX = minX + extent;
            maxY = minY + extent;
        }

        // Expand Z range to avoid clipping objects between light and frustum
        const float zPadding = 200.0f;
        minZ -= zPadding;
        maxZ += zPadding;

        // Vulkan Ortho for Depth [0, 1]
        // near maps to 0, far maps to 1.
        // In view space, negative Z is forward.
        // So maxZ (least negative) is closer to camera (near),
        // and minZ (most negative) is farther from camera (far).
        glm::mat4 lightProj = glm::ortho(
                minX,
                maxX,
                minY,
                maxY,
                maxZ,
                minZ
        );

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
        auto splits = computeCascadeSplits(nearPlane, farPlane, 0.95f);

        float lastSplit = nearPlane;

        for(uint32_t i = 0; i < 4; ++i) {
            float cascadeNear = lastSplit;
            float cascadeFar = splits[i];

            auto frustumCorners = getFrustumCornersWorldSpace(projectionMatrix, viewMatrix, cascadeNear, cascadeFar);

            auto lightMatrix = computeCascadeLightMatrix(frustumCorners, lightDirection, shadowMapSize);

            data.lightProjection[i] = lightMatrix;
            data.cascadeSplits[i] = cascadeFar;

            lastSplit = cascadeFar;
        }

        return data;
    }
};


#endif //OXYOUS_2026_RENDERHELPER_HPP
