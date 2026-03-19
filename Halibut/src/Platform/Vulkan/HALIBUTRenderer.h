#pragma once
#include "Core/Core.h"
#include "HALIBUTDevice.h"
#include "HALIBUTSwapchain.h"
#include "HALIBUTFrame.h"
#include "HALIBUTGraphicPipeline.h"

#include <glm/glm.hpp>

namespace HALIBUT
{
    struct SubmitInfo
    {
        uint32_t WaitSemaphoreCount = 1;
        uint32_t CommandBufferCount = 1;
        uint32_t SignalSemaphoreCount = 1;
    };

    struct PresentInfo
    {
        uint32_t WaitSemaphoreCount = 1; 
        uint32_t SwapchainCount = 1; 
    };
    struct ImageTransition
    {
        vk::Image Image = vk::Image{};
        vk::ImageLayout OldLayout = vk::ImageLayout::eUndefined;
        vk::ImageLayout NewLayout = vk::ImageLayout::eUndefined;
        vk::AccessFlags2 SrcAccessMask{};
        vk::AccessFlags2 DstAccessMask{};
        vk::PipelineStageFlags2 SrcStageMask{};
        vk::PipelineStageFlags2 DstStageMask{};
        vk::ImageAspectFlags AspectMask = vk::ImageAspectFlagBits::eColor;
    };

    struct PassBarrierPlan
    {
        std::vector<ImageTransition> BeginTransitions;
        std::vector<ImageTransition> EndTransitions;

        void Clear()
        {
            BeginTransitions.clear();
            EndTransitions.clear();
        }
    };
    enum class ActivePassType
    {
        None,
        Offscreen,
        Swapchain
    };
    class HALIBUT_API HALIBUTRenderer
    {
        public:
            HALIBUTRenderer(HALIBUTDevice& device, HALIBUTSwapchain& swapchain);
            ~HALIBUTRenderer();

            void BeginFrame();
            void EndFrame();
            //void BeginOffscreenPass(HALIBUTOffscreenImage& offscreenImage, HALIBUTDepthImage& depthImage, const vec4f& clearColor);
            //void EndOffscreenPass(HALIBUTOffscreenImage& offscreenImage);
            void BeginSwapchainPass();
            void EndSwapchainPass();
            void SetClearColor(glm::vec4& clearcolor);
            void BindPipeline(HALIBUTGraphicPipeline& pipeline);
            void SetViewport(uint32_t position, vk::Viewport& viewport);
            void SetViewport(uint32_t position, const float& x, const float& y, const float& minDepth, const float& maxDepth);
            void SetScissor(uint32_t position, vk::Rect2D& scissor);
            void SetScissor(uint32_t position, const float& OffsetX, const float& OffsetY);
            void Draw(uint32_t vertexCount,uint32_t instanceCount,uint32_t firstVertex,uint32_t firstInstance);

            inline bool IsNeedRecreationSwapchain() { return b_NeedRecreationSwapchain; };
            inline vk::raii::CommandBuffer& GetCommandBuffer() { return m_Frames[m_CurrentFrame].GetCommandBuffer(); }
            inline uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
            inline uint32_t GetFramesInFlight() const { return static_cast<uint32_t>(m_Frames.size()); }
            inline glm::vec4 GetClearColor() const { return m_ClearColor; };
        private:
            static ImageTransition CreateImageTransition(
                vk::Image image,
                vk::ImageLayout oldLayout,
                vk::ImageLayout newLayout,
                vk::AccessFlags2 srcAccessMask,
                vk::AccessFlags2 dstAccessMask,
                vk::PipelineStageFlags2 srcStageMask,
                vk::PipelineStageFlags2 dstStageMask,
                vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor
            );
            void TransitionImageLayout(
                vk::Image image,
                vk::ImageLayout oldLayout,
                vk::ImageLayout newLayout,
                vk::AccessFlags2 srcAccessMask,
                vk::AccessFlags2 dstAccessMask,
                vk::PipelineStageFlags2 srcStageMask,
                vk::PipelineStageFlags2 dstStageMask,
                vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor
            );
            void ApplyImageTransitions(const std::vector<ImageTransition>& transitions);
            //PassBarrierPlan BuildOffscreenPassBarrierPlan(HALIBUTOffscreenImage& offscreenImage, HALIBUTDepthImage& depthImage) const;
            PassBarrierPlan BuildSwapchainPassBarrierPlan(vk::Image image, vk::ImageLayout oldLayout) const;
        private:
            HALIBUTDevice& m_Device;
            HALIBUTSwapchain& m_Swapchain;
            uint32_t                m_QueueIndex            = 0;
            uint32_t                m_QueueNumber           = 0;
            vk::raii::Queue         m_Queue                 = nullptr;
            uint32_t                m_AcquiredImageIndex    = 0;
            uint32_t                m_CurrentFrame          = 0;
            bool                    m_IsRenderingActive     = false;

            bool b_NeedRecreationSwapchain = false;

            std::vector<HALIBUTFrame> m_Frames;
            std::vector<vk::Fence> m_ImageInFlightFences;
            std::vector<vk::raii::Semaphore> m_ImageRenderFinishedSemaphores;
            std::vector<vk::ImageLayout> m_SwapchainImageLayouts;
            ActivePassType m_ActivePassType = ActivePassType::None;
            PassBarrierPlan m_ActivePassBarrierPlan;
            uint64_t m_FrameCounter = 0;

            //info
            SubmitInfo m_SubmitInfo;
            PresentInfo m_PresentInfo;
            glm::vec4 m_ClearColor = {0.1f,0.1f,0.1f,1.0f};
    };
}