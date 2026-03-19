#include "halibutpch.h"
#include "HALIBUTFrame.h"

namespace HALIBUT
{
    HALIBUTFrame::HALIBUTFrame(vk::raii::Device& device, uint32_t queueFamilyIndex, uint32_t frameIndex)
        : m_Device(device),
          m_FrameIndex(frameIndex)
    {
        m_CommandPool = MakeUPtr<HALIBUTCommandPool>(m_Device, queueFamilyIndex);
        m_CommandBuffer = MakeUPtr<HALIBUTCommandBuffer>(m_Device, m_CommandPool->GetCommandPool());

        m_ImageAvailableSemaphore = vk::raii::Semaphore(m_Device, vk::SemaphoreCreateInfo{});
        m_RenderFinishedSemaphore = vk::raii::Semaphore(m_Device, vk::SemaphoreCreateInfo{});
        m_InFlightFence = vk::raii::Fence(
            m_Device,
            vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled }
        );
    }

    HALIBUTFrame::~HALIBUTFrame()
    {
        try
        {
            if (m_IsCommandRecording)
            {
                m_CommandBuffer->GetCommandBuffer().end();
                m_IsCommandRecording = false;
            }

            FlushDeferredReleases();
        }
        catch (...)
        {
            // 析构阶段不抛异常
        }
    }

    void HALIBUTFrame::WaitForFence() const
    {
        const vk::Result waitResult = m_Device.waitForFences(*m_InFlightFence, VK_TRUE, UINT64_MAX);
        if (waitResult != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to wait for frame in-flight fence");
        }
    }

    void HALIBUTFrame::ResetForNextSubmit()
    {
        if (m_IsCommandRecording)
        {
            throw std::runtime_error("cannot reset frame while command buffer is recording");
        }

        // 到这里通常意味着上一轮使用这个 frame 的 GPU 工作已经结束，
        // 所以可以安全释放这个 frame 上挂着的延迟释放资源。
        FlushDeferredReleases();

        m_Device.resetFences(*m_InFlightFence);
        m_CommandPool->GetCommandPool().reset(vk::CommandPoolResetFlags{});
    }

    void HALIBUTFrame::BeginCommandBuffer(vk::CommandBufferUsageFlags usageFlags)
    {
        if (m_IsCommandRecording)
        {
            throw std::runtime_error("command buffer already recording for current frame");
        }

        vk::CommandBufferBeginInfo beginInfo{
            .flags = usageFlags
        };

        m_CommandBuffer->GetCommandBuffer().begin(beginInfo);
        m_IsCommandRecording = true;
    }

    void HALIBUTFrame::EndCommandBuffer()
    {
        if (!m_IsCommandRecording)
        {
            throw std::runtime_error("command buffer is not recording");
        }

        m_CommandBuffer->GetCommandBuffer().end();
        m_IsCommandRecording = false;
    }

    void HALIBUTFrame::EnqueueDeferredRelease(CleanupTask cleanupTask)
    {
        if (!cleanupTask)
        {
            return;
        }

        m_DeferredReleaseQueue.emplace_back(std::move(cleanupTask));
    }

    void HALIBUTFrame::FlushDeferredReleases()
    {
        // 逆序释放，比较符合“栈式资源”的直觉
        for (auto it = m_DeferredReleaseQueue.rbegin(); it != m_DeferredReleaseQueue.rend(); ++it)
        {
            if (*it)
            {
                (*it)();
            }
        }

        m_DeferredReleaseQueue.clear();
    }
}