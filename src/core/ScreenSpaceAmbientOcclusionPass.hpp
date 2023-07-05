#pragma once

#include "ScriptableRenderPass.hpp"

class ScreenSpaceAmbientOcclusionPass : public ScriptableRenderPass
{
public:

	ScreenSpaceAmbientOcclusionPass() {
		LOG_INFO("Create ScreenSpaceAmbientOcclusionPass");
		name = "ScreenSpaceAmbientOcclusionPass";
	}

	~ScreenSpaceAmbientOcclusionPass() {
		LOG_INFO("Delete ScreenSpaceAmbientOcclusionPass");
	}

	void Configure() {
		
	}

	void Setup() {
		
	}

    void Execute(std::shared_ptr<RenderingData> renderingData) {
		LOG_INFO("Execute ScreenSpaceAmbientOcclusionPass");
	}
};