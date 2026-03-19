#pragma once

#include "Core/Core.h"
#include "HALIBUTCommandBuffer.h"
#include "HALIBUTCommandPool.h"

#include <functional>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace HALIBUT
{
    class HALIBUT_API HALIBUTFrame
    {
    public:
        using CleanupTask = std::function<void()>;

    public:
        HALIBUTFrame(vk::raii::Device& device, uint32_t queueFamilyIndex, uint32_t frameIndex);
        ~HALIBUTFrame();

        HALIBUTFrame(const HALIBUTFrame&) = delete;
        HALIBUTFrame& operator=(const HALIBUTFrame&) = delete;
        HALIBUTFrame(HALIBUTFrame&&) noexcept = default;
        HALIBUTFrame& operator=(HALIBUTFrame&&) noexcept = delete;

        void WaitForFence() const;
        void ResetForNextSubmit();

        void BeginCommandBuffer(
            vk::CommandBufferUsageFlags usageFlags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        );
        void EndCommandBuffer();

        void EnqueueDeferredRelease(CleanupTask cleanupTask);
        void FlushDeferredReleases();

        uint32_t GetFrameIndex() const { return m_FrameIndex; }
        bool IsCommandRecording() const { return m_IsCommandRecording; }

        HALIBUTCommandPool& GetWrappedCommandPool() { return *m_CommandPool; }
        HALIBUTCommandBuffer& GetWrappedCommandBuffer() { return *m_CommandBuffer; }

        vk::raii::CommandBuffer& GetCommandBuffer() { return m_CommandBuffer->GetCommandBuffer(); }

        const vk::raii::Semaphore& GetImageAvailableSemaphore() const { return m_ImageAvailableSemaphore; }
        const vk::raii::Semaphore& GetRenderFinishedSemaphore() const { return m_RenderFinishedSemaphore; }
        const vk::raii::Fence& GetInFlightFence() const { return m_InFlightFence; }

    private:
        vk::raii::Device& m_Device;
        uint32_t m_FrameIndex = 0;
        bool m_IsCommandRecording = false;

        std::vector<CleanupTask> m_DeferredReleaseQueue;

        UPtr<HALIBUTCommandPool> m_CommandPool;
        UPtr<HALIBUTCommandBuffer> m_CommandBuffer;

        vk::raii::Semaphore m_ImageAvailableSemaphore = nullptr;
        vk::raii::Semaphore m_RenderFinishedSemaphore = nullptr;
        vk::raii::Fence m_InFlightFence = nullptr;
    };
}