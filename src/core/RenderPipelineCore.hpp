#pragma once
#include "Macros.h"
#include "Camera.hpp"
#include "Light.hpp"

class ScriptableRenderer;

class CullingResults
{
public:
    //todo
    int count;
};

class CameraData
{
public:
    glm::mat4x4 m_ViewMatrix;
    glm::mat4x4 m_ProjectionMatrix;

    void SetViewAndProjectionMatrix(glm::mat4x4 viewMatrix, glm::mat4x4 projectionMatrix)
    {
        m_ViewMatrix = viewMatrix;
        m_ProjectionMatrix = projectionMatrix;
    }

    glm::mat4x4 GetViewMatrix(int viewIndex = 0)
    {
        return m_ViewMatrix;
    }

    glm::mat4x4 GetProjectionMatrix(int viewIndex = 0)
    {
        return m_ProjectionMatrix;
    }

    std::shared_ptr<Camera> camera;
    std::shared_ptr<ScriptableRenderer> renderer;
};

class LightData
{
public:
    int mainLightIndex;
    int additionalLightsCount;
    int maxPerObjectAdditionalLightsCount;
    std::vector<Light> visibleLights;
    bool supportsAdditionalLights;
};

class ShadowData
{
public:
    bool supportsMainLightShadows;
    int mainLightShadowmapWidth;
    int mainLightShadowmapHeight;
    bool supportsAdditionalLightShadows;
    int additionalLightsShadowmapWidth;
    int additionalLightsShadowmapHeight;
    bool supportsSoftShadows;
};

class RenderingData
{
public:
    std::shared_ptr<CullingResults> cullResults;
    std::shared_ptr<CameraData> cameraData;
    std::shared_ptr<LightData> lightData;
    std::shared_ptr<ShadowData> shadowData;
};