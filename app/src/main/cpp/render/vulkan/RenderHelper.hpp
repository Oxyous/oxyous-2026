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
        glm::mat4 inverseProjeView = glm::inverse(projectionMatrix * viewMatrix);
        std::array<glm::vec4, 8> corners;
        uint32_t index = 0;

        for (float x: {-1.0f, 1.0f}) {
            for (float y: {-1.0f, 1.0f}) {
                for (float z: {nearPlane, farPlane}) {
                    glm::vec4 point = inverseProjeView * glm::vec4(x, y, z, 1.0f);
                    corners[index++] = point / point.w;
                }
            }
        }

        return corners;
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

        glm::vec3 lightPos = center - lightDir * radius * 2.0f;

        glm::mat4 lightView = glm::lookAt(lightPos, center, up);

        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float minZ = std::numeric_limits<float>::max();

        float maxX = -std::numeric_limits<float>::min();
        float maxY = -std::numeric_limits<float>::min();
        float maxZ = -std::numeric_limits<float>::min();

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

        // Expand Z range a bit to avoid clipping shadow casters.
        // Tune these for your scene scale.
        const float zPadding = 100.0f;
        minZ -= zPadding;
        maxZ += zPadding;

        glm::mat4 lightProj = glm::ortho(
                minX,
                maxX,
                minY,
                maxY,
                minZ,
                maxZ
        );

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
