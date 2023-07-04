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

	void Render(std::vector<std::shared_ptr<Camera>> cameras) {
		//SortCameras(cameras);

		for (int i = 0; i < cameras.size(); ++i) {
			auto camera = cameras[i];
			std::shared_ptr<CameraData> cameraData = std::make_shared<CameraData>();
			InitializeCameraData(camera, cameraData);
			//LOG_INFO("Camear Data:{}", cameraData.camera->type);
			RenderSingleCamera(cameraData);
		}
	}

	static void InitializeCameraData(std::shared_ptr<Camera> camera, std::shared_ptr<CameraData> cameraData) {
		//test
		cameraData->camera = camera;
		cameraData->renderer = std::make_shared<ForwardRenderer>();
	}

	static std::shared_ptr<RenderingData> InitializeRenderingData(std::shared_ptr<CameraData> cameraData, std::shared_ptr<CullingResults> camera) {
		return std::make_shared<RenderingData>();
	}

	static void RenderSingleCamera(std::shared_ptr<CameraData> cameraData) {
		auto camera = cameraData->camera;
		auto renderer = cameraData->renderer;

		if (renderer == nullptr) {
			LOG_WARN("Trying to render {} with an invalid renderer. Camera rendering will be skipped.", camera->name);
			return;
		}

		auto renderingData = InitializeRenderingData(cameraData, std::make_shared<CullingResults>());

		renderer->Setup(renderingData);

		renderer->Execute(renderingData);
	}
};