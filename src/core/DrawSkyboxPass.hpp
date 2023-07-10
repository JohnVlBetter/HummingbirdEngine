#pragma once

#include "ScriptableRenderPass.hpp"

class DrawSkyboxPass : public ScriptableRenderPass
{
public:

	DrawSkyboxPass() {
		LOG_INFO("Create DrawSkyboxPass");
		name = "DrawSkyboxPass";
		renderPassEvent = BeforeRenderingSkybox;
	}

	~DrawSkyboxPass() {
		LOG_INFO("Delete DrawSkyboxPass");
	}

	void Configure() {
		
	}

    void Execute(RenderingData* renderingData) {
		LOG_INFO("Execute DrawSkyboxPass");
	}
};