#pragma once
#include "Core/Core.h"
#include <vulkan/vulkan_raii.hpp>

namespace HALIBUT
{
    class HALIBUT_API HALIBUTGraphicPipeline
    {
    public:
        inline vk::raii::Pipeline& GetPipeline() { return m_GraphicsPipeline; }
    private:
        vk::raii::Pipeline m_GraphicsPipeline = nullptr;
    };
}