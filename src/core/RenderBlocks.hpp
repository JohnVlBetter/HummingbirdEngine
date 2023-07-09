#pragma once

#include "ScriptableRenderPass.hpp"

class RenderBlocks {
public:
	std::vector<int> blockStart;
	std::vector<int> blockLength;
	std::vector<RenderPassEvent> blockEventLimits;

	RenderBlocks(std::vector<std::shared_ptr<ScriptableRenderPass>>& activeRenderPasses)
	{
		blockEventLimits = std::vector<RenderPassEvent>(3);
		blockStart = std::vector<int>(blockEventLimits.size());
		blockLength = std::vector<int>(blockEventLimits.size());

		blockEventLimits[0] = AfterRenderingOpaques;
		blockEventLimits[1] = AfterRenderingPostProcessing;
		blockEventLimits[2] = AfterRendering;

		int currRenderPass = 0;

		for (int i = 0; i < blockEventLimits.size(); ++i)
		{
			blockStart[i] = currRenderPass;

			while (currRenderPass < activeRenderPasses.size() &&
				activeRenderPasses[currRenderPass]->renderPassEvent < blockEventLimits[i])
				++currRenderPass;

			blockLength[i++] = currRenderPass - blockStart[i];
		}

		blockEventLimits.clear();
	}

	~RenderBlocks() { Dispose(); }

	void Dispose()
	{
		blockStart.clear();
		blockLength.clear();
	}

	void GetRange(int& start, int& length, int blockIdx) {
		start = blockStart[blockIdx];
		length = blockLength[blockIdx];
	}
};