#pragma once
#include "Macros.h"
#include "Camera.hpp"
#include "Light.hpp"

struct RenderingData
{
    CullingResults cullResults;
    CameraData cameraData;
    LightData lightData;
    ShadowData shadowData;
};

struct CullingResults 
{
    //todo
    int count;
};

struct CameraData
{
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

    Camera camera;
};

struct LightData
{
    int mainLightIndex;
    int additionalLightsCount;
    int maxPerObjectAdditionalLightsCount;
    std::vector<Light> visibleLights;
    bool supportsAdditionalLights;
};

struct ShadowData
{
    bool supportsMainLightShadows;
    int mainLightShadowmapWidth;
    int mainLightShadowmapHeight;
    bool supportsAdditionalLightShadows;
    int additionalLightsShadowmapWidth;
    int additionalLightsShadowmapHeight;
    bool supportsSoftShadows;
};