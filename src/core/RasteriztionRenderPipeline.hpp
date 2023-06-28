#pragma once

#include "ScriptableRenderPipeline.hpp"

class RasteriztionRenderPipeline : public ScriptableRenderPipeline
{
private:
public:
	RasteriztionRenderPipeline() {
		name = "RasteriztionRenderPipeline";
	}

	~RasteriztionRenderPipeline() {
	}

	void Render(Camera cameras[]) {}

	static void RenderSingleCamera(Camera camera) {}
};