#pragma once

#include "ScriptableRenderPass.hpp"

class DrawObjectsPass : public ScriptableRenderPass
{
public:

	DrawObjectsPass() {
		LOG_INFO("Create DrawObjectsPass");
		name = "DrawObjectsPass";
	}

	~DrawObjectsPass() {
		LOG_INFO("Delete DrawObjectsPass");
	}

    void Execute(RenderingData renderingData) {
		LOG_INFO("Execute DrawObjectsPass");
	}
};