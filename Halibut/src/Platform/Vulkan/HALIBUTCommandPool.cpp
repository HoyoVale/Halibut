#include "halibutpch.h"
#include "HALIBUTCommandPool.h"

namespace HALIBUT
{
    HALIBUTCommandPool::HALIBUTCommandPool(
        vk::raii::Device &device,
        uint32_t queueFamilyIndex
    )
        :m_Device(&device),
        m_QueueIndex(queueFamilyIndex)
    {
        CreateCommandPool();
    }
    void HALIBUTCommandPool::CreateCommandPool()
    {
         vk::CommandPoolCreateInfo poolInfo{
            .flags            = m_PoolInfo.CommandPoolCreateFlag,
            .queueFamilyIndex = m_QueueIndex
        };
        m_CommandPool = vk::raii::CommandPool(*m_Device, poolInfo);
    }
}