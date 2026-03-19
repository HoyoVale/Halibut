#pragma once
#include "Core/Core.h"
#include "vulkan/vulkan_raii.hpp" 

namespace HALIBUT
{
    struct AllocInfo
    {
        vk::CommandBufferLevel Level = vk::CommandBufferLevel::ePrimary;
        uint32_t CommandBufferCount = 1;
    };

    struct TransitionImageLayoutInfo
    {
        vk::ImageLayout OldLayout = vk::ImageLayout::eUndefined;
        vk::ImageLayout NewLayout = vk::ImageLayout::eColorAttachmentOptimal;
        vk::AccessFlagBits2 SrcAccessMask = {};
        vk::AccessFlagBits2 DstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
        vk::PipelineStageFlagBits2 SrcStageMask= vk::PipelineStageFlagBits2::eTopOfPipe;
        vk::PipelineStageFlagBits2 DstStageMask= vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    };

    struct ImageMemoryBarrierInfo
    {
        uint32_t SrcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        uint32_t DrcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk::ImageAspectFlagBits AspectMask = vk::ImageAspectFlagBits::eColor;
        uint32_t BaseMipLevel   = 0;
        uint32_t LevelCount     = 1;
        uint32_t BaseArrayLayer = 0;
        uint32_t LayerCount     = 1;
    };

    class HALIBUT_API HALIBUTCommandBuffer
    {
    public:
        HALIBUTCommandBuffer(
            vk::raii::Device& device, 
            vk::raii::CommandPool& commandpool
        );
        ~HALIBUTCommandBuffer(){};
        void Transition_image_layout(
            uint32_t                imageIndex,
            vk::ImageLayout         old_layout,
            vk::ImageLayout         new_layout,
            vk::AccessFlags2        src_access_mask,
            vk::AccessFlags2        dst_access_mask,
            vk::PipelineStageFlags2 src_stage_mask,
            vk::PipelineStageFlags2 dst_stage_mask,
            std::vector<vk::Image>& swapChainImages
        );

        inline vk::raii::CommandBuffer& GetCommandBuffer() { return m_CommandBuffer; }
    private:
        void CreateCommandBuffer(vk::CommandPool commandpool);
    private:
        vk::raii::Device* m_Device = nullptr;
        
        vk::raii::CommandBuffer m_CommandBuffer = nullptr;
    public:
        // Info
        AllocInfo m_AllocInfo;
        TransitionImageLayoutInfo m_TransitionImageLayoutInfo;
        ImageMemoryBarrierInfo m_ImageMemoryBarrierInfo;
    };
}