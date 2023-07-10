#pragma once

#include "ScriptableRenderPass.hpp"

class ScreenSpaceAmbientOcclusionPass : public ScriptableRenderPass
{
public:

	ScreenSpaceAmbientOcclusionPass() {
		LOG_INFO("Create ScreenSpaceAmbientOcclusionPass");
		name = "ScreenSpaceAmbientOcclusionPass";
		renderPassEvent = AfterRenderingPostProcessing;
	}

	~ScreenSpaceAmbientOcclusionPass() {
		LOG_INFO("Delete ScreenSpaceAmbientOcclusionPass");
	}

	void Configure() {
		
	}

	void Setup() {
		
	}

    void Execute(RenderingData* renderingData) {
		LOG_INFO("Execute ScreenSpaceAmbientOcclusionPass");
	}
};