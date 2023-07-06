#pragma once

#include "ScriptableRenderPass.hpp"

class DrawObjectsPass : public ScriptableRenderPass
{
public:

	DrawObjectsPass() {
		LOG_INFO("Create DrawObjectsPass");
		name = "DrawObjectsPass";
		renderPassEvent = BeforeRenderingShadows;
	}

	~DrawObjectsPass() {
		LOG_INFO("Delete DrawObjectsPass");
	}

	void Configure() {
		
	}

    void Execute(std::shared_ptr<RenderingData> renderingData) {
		LOG_INFO("Execute DrawObjectsPass");
	}
};