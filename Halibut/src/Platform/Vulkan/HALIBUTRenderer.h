#pragma once
#include "Core/Core.h"
#include "HALIBUTDevice.h"
#include "HALIBUTSwapchain.h"
#include <glm/glm.hpp>

namespace HALIBUT
{
    class HALIBUT_API HALIBUTRenderer
    {
        public:
            HALIBUTRenderer(HALIBUTDevice& device, HALIBUTSwapchain& swapchain);
            ~HALIBUTRenderer();

            void BeginFrame();
            void EndFrame();

            bool IsNeedRecreationSwapchain();
            void SetClearColor(glm::vec4& clearcolor);

            inline glm::vec4 GetClearColor() const { return m_ClearColor; };

        private:
            HALIBUTDevice& m_Device;
            HALIBUTSwapchain& m_Swapchain;
            glm::vec4 m_ClearColor = {0.1f,0.1f,0.1f,1.0f};
    };
}