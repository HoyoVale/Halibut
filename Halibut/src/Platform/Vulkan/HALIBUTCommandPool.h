#pragma once
#include "Core/Core.h"
#include "vulkan/vulkan_raii.hpp"

namespace HALIBUT
{

    struct PoolInfo
    {
        vk::CommandPoolCreateFlagBits CommandPoolCreateFlag =  vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    };

    class HALIBUT_API HALIBUTCommandPool
    {
    public:
        HALIBUTCommandPool(vk::raii::Device& device,  uint32_t queueFamilyIndex);
        ~HALIBUTCommandPool(){};
        inline vk::raii::CommandPool& GetCommandPool() { return m_CommandPool; }
    private:
        void CreateCommandPool();

    private:
        vk::raii::Device* m_Device;
        uint32_t m_QueueIndex = 0;
        PoolInfo m_PoolInfo;
        vk::raii::CommandPool m_CommandPool = nullptr;
    };
}