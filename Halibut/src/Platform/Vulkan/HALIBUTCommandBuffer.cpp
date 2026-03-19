#include "halibutpch.h"
#include "HALIBUTCommandBuffer.h"

namespace HALIBUT
{
    HALIBUTCommandBuffer::HALIBUTCommandBuffer(
		vk::raii::Device &device, 
		vk::raii::CommandPool& commandpool
	)
        :m_Device(&device)
    {
        CreateCommandBuffer(*commandpool);
    }
    void HALIBUTCommandBuffer::CreateCommandBuffer(vk::CommandPool commandpool)
    {
        vk::CommandBufferAllocateInfo allocInfo{
            .commandPool = commandpool, 
            .level = m_AllocInfo.Level,
            .commandBufferCount = m_AllocInfo.CommandBufferCount
        };
        m_CommandBuffer = std::move(vk::raii::CommandBuffers(*m_Device, allocInfo).front());
    }

	void HALIBUTCommandBuffer::Transition_image_layout(
	    uint32_t                imageIndex,
	    vk::ImageLayout         old_layout,
	    vk::ImageLayout         new_layout,
	    vk::AccessFlags2        src_access_mask,
	    vk::AccessFlags2        dst_access_mask,
	    vk::PipelineStageFlags2 src_stage_mask,
	    vk::PipelineStageFlags2 dst_stage_mask,
        std::vector<vk::Image>& swapChainImages
    )
	{
		vk::ImageMemoryBarrier2 barrier = {
		    .srcStageMask        = src_stage_mask,
		    .srcAccessMask       = src_access_mask,
		    .dstStageMask        = dst_stage_mask,
		    .dstAccessMask       = dst_access_mask,
		    .oldLayout           = old_layout,
		    .newLayout           = new_layout,
		    .srcQueueFamilyIndex = m_ImageMemoryBarrierInfo.SrcQueueFamilyIndex,
		    .dstQueueFamilyIndex = m_ImageMemoryBarrierInfo.DrcQueueFamilyIndex,
		    .image               = swapChainImages[imageIndex],
		    .subresourceRange    = {
				.aspectMask     = m_ImageMemoryBarrierInfo.AspectMask,
				.baseMipLevel   = m_ImageMemoryBarrierInfo.BaseMipLevel,
				.levelCount     = m_ImageMemoryBarrierInfo.LevelCount,
				.baseArrayLayer = m_ImageMemoryBarrierInfo.BaseArrayLayer,
				.layerCount     = m_ImageMemoryBarrierInfo.LayerCount}};
		vk::DependencyInfo dependency_info = {
		    .dependencyFlags         = {},
		    .imageMemoryBarrierCount = 1,
		    .pImageMemoryBarriers    = &barrier
		};
		m_CommandBuffer.pipelineBarrier2(dependency_info);
	}
}
