#include "halibutpch.h"
#include "HALIBUTRenderer.h"

namespace HALIBUT
{
    namespace
    {
        template <typename TAcquireResult>
        auto ExtractAcquireResult(const TAcquireResult& value) -> decltype(value.result)
        {
            return value.result;
        }

        template <typename TAcquireResult>
        auto ExtractAcquireResult(const TAcquireResult& value) -> decltype(value.first)
        {
            return value.first;
        }

        template <typename TAcquireResult>
        auto ExtractAcquireImageIndex(const TAcquireResult& value) -> decltype(value.value)
        {
            return value.value;
        }

        template <typename TAcquireResult>
        auto ExtractAcquireImageIndex(const TAcquireResult& value) -> decltype(value.second)
        {
            return value.second;
        }
    }
    HALIBUTRenderer::HALIBUTRenderer(HALIBUTDevice &device, HALIBUTSwapchain &swapchain)
        :m_Device(device), m_Swapchain(swapchain)
    {
        // 获取 swapchain 的 image 数
        const uint32_t swapchainImageCount = static_cast<uint32_t>(m_Swapchain.GetSwapChainImages().size());
        // 设置 frames in flight 的数量范围
        const uint32_t framesInFlight = std::max(1u, std::min(5u, swapchainImageCount)); 
        // 从 Device 获取 queue
        m_QueueIndex = m_Device.GetQueueFamilyIndices().graphicsFamily.value();
        m_Queue = vk::raii::Queue(m_Device.GetDevice(), m_QueueIndex, m_QueueNumber);
        // 根据 frames in flight 数设置 frame 
        m_Frames.reserve(framesInFlight);
        for (uint32_t frameIndex = 0; frameIndex < framesInFlight; ++frameIndex)
        {
            m_Frames.emplace_back(m_Device.GetDevice(), m_QueueIndex, frameIndex);
        }
        // 根据swapchain的数量设置 images in flight 的容量
        m_ImageInFlightFences.resize(swapchainImageCount, vk::Fence{});
        // 设置 image 渲染结束的信号（semaphore）
        m_ImageRenderFinishedSemaphores.reserve(swapchainImageCount);
        for (uint32_t imageIndex = 0; imageIndex < swapchainImageCount; ++imageIndex)
        {
            m_ImageRenderFinishedSemaphores.emplace_back(m_Device.GetDevice(), vk::SemaphoreCreateInfo{});
        }
        // 设置交换链图片的布局 image layouts
        m_SwapchainImageLayouts.resize(swapchainImageCount, vk::ImageLayout::eUndefined);
        // // 设置队列族的属性
        // const auto queueFamilyProperties = m_Device.GetPhysicalDevice().getQueueFamilyProperties();
        // if (m_QueueIndex < queueFamilyProperties.size())
        // {
        //     m_GpuTimestampSupported = queueFamilyProperties[m_QueueIndex].timestampValidBits > 0;
        // }
        // m_GpuTimestampPeriodNs = m_Device.GetPhysicalDevice().getProperties().limits.timestampPeriod;
        // if (m_GpuTimestampSupported)
        // {
        //     m_FrameTimestampQueryPools.reserve(framesInFlight);
        //     vk::QueryPoolCreateInfo queryPoolCreateInfo{
        //         .queryType = vk::QueryType::eTimestamp,
        //         .queryCount = s_TimestampQueryCount
        //     };
        //     for (uint32_t frameIndex = 0; frameIndex < framesInFlight; ++frameIndex)
        //     {
        //         m_FrameTimestampQueryPools.emplace_back(m_Device.GetDevice(), queryPoolCreateInfo);
        //     }
        //     m_FrameHasPendingGpuTimestamps.resize(framesInFlight, false);
        // }
        // else
        // {
        //     std::cout << "[HALIBUT] GPU timestamp query unsupported on selected queue family; GPU timings disabled." << std::endl;
        // }
    }
    HALIBUTRenderer::~HALIBUTRenderer()
    {
        
    }
    void HALIBUTRenderer::BeginFrame()
    {
        // 获取当前帧
        HALIBUTFrame& currentFrame = m_Frames[m_CurrentFrame];
        // 等待帧资源可用
        currentFrame.WaitForFence();
        // 获取 swapchain 图片索引
        vk::Result acquireResult = vk::Result::eSuccess;
        uint32_t acquiredImageIndex = 0;
        try
        {
		    auto acquireResultValue = m_Swapchain.GetSwapchain().acquireNextImage(
                UINT64_MAX,
                *currentFrame.GetImageAvailableSemaphore(),
                nullptr
            );
            // 获取结果
            acquireResult = ExtractAcquireResult(acquireResultValue);
            acquiredImageIndex = ExtractAcquireImageIndex(acquireResultValue);
        }
        catch (const vk::OutOfDateKHRError&)
        {
            b_NeedRecreationSwapchain = true;
            return;
        }
        switch (acquireResult) // 根据结果判断是否要重建 swapchain
        {
        case vk::Result::eSuccess:
            b_NeedRecreationSwapchain = false;
            break;
        case vk::Result::eSuboptimalKHR:
        case vk::Result::eErrorOutOfDateKHR: 
            b_NeedRecreationSwapchain = true;
            return;
        default:
            throw std::runtime_error("failed to acquire swapchain image");
        }

        m_AcquiredImageIndex = acquiredImageIndex;
        if (m_AcquiredImageIndex >= m_Swapchain.GetSwapChainImages().size())
        {
            throw std::runtime_error("acquired swapchain image index out of range");
        }
        if (m_AcquiredImageIndex >= m_Swapchain.GetImageViews().size())
        {
            throw std::runtime_error("acquired swapchain image view index out of range");
        }
        if (m_AcquiredImageIndex >= m_ImageInFlightFences.size())
        {
            throw std::runtime_error("acquired swapchain image fence index out of range");
        }
        if (m_AcquiredImageIndex >= m_ImageRenderFinishedSemaphores.size())
        {
            throw std::runtime_error("acquired swapchain image render-finished semaphore index out of range");
        }
        if (m_ImageInFlightFences[m_AcquiredImageIndex] != vk::Fence{})
        {
            const vk::Result waitResult = m_Device.GetDevice().waitForFences(
                m_ImageInFlightFences[m_AcquiredImageIndex],
                VK_TRUE,
                UINT64_MAX
            );
            if (waitResult != vk::Result::eSuccess)
            {
                throw std::runtime_error("failed to wait for image-in-flight fence");
            }
        }
        m_ImageInFlightFences[m_AcquiredImageIndex] = *currentFrame.GetInFlightFence();
        // 初始化命令提交
        currentFrame.ResetForNextSubmit();
        // 开始命令录制
        currentFrame.BeginCommandBuffer();
    }
    void HALIBUTRenderer::EndFrame()
    {
        if (m_IsRenderingActive)
        {
            throw std::runtime_error("cannot end frame while a rendering pass is still active");
        }
        HALIBUTFrame& currentFrame = m_Frames[m_CurrentFrame];
        // 结束命令录制
        currentFrame.EndCommandBuffer();
        auto advanceFrame = [this]() {
            m_CurrentFrame = (m_CurrentFrame + 1u) % static_cast<uint32_t>(m_Frames.size());
        };
        // 设置 pipeline 写入属性
        vk::PipelineStageFlags waitDestinationStageMask = (vk::PipelineStageFlagBits::eColorAttachmentOutput);
        // 获取当前frame的可写入信号
        const vk::Semaphore imageAvailableSemaphore = *currentFrame.GetImageAvailableSemaphore();
        // 获取当前frame的渲染结束信号
        const vk::Semaphore renderFinishedSemaphore = *m_ImageRenderFinishedSemaphores[m_AcquiredImageIndex];
        // 获取命令缓冲区
        const vk::CommandBuffer commandBuffer = *currentFrame.GetCommandBuffer();
        // 设置提交（submit）配置
        const vk::SubmitInfo submitInfo{
            .waitSemaphoreCount     =   m_SubmitInfo.WaitSemaphoreCount, 
            .pWaitSemaphores        =   &imageAvailableSemaphore, 
            .pWaitDstStageMask      =   & waitDestinationStageMask,
            .commandBufferCount     =   m_SubmitInfo.CommandBufferCount, 
            .pCommandBuffers        =   &commandBuffer,
            .signalSemaphoreCount   =   m_SubmitInfo.SignalSemaphoreCount, 
            .pSignalSemaphores      =   &renderFinishedSemaphore
        };
        // 渲染命令提交到队列
        m_Queue.submit(submitInfo, *currentFrame.GetInFlightFence());
        // 设置图像呈现（present）配置
        const vk::PresentInfoKHR presentInfo{
            .waitSemaphoreCount =   m_PresentInfo.WaitSemaphoreCount, 
            .pWaitSemaphores    =   &renderFinishedSemaphore, 
            .swapchainCount     =   m_PresentInfo.SwapchainCount, 
            .pSwapchains        =   &*m_Swapchain.GetSwapchain(), 
            .pImageIndices      =   &m_AcquiredImageIndex
        };
        // 初始化 present 结果
        vk::Result presentResult = vk::Result::eSuccess;
        // 判断是否要重建 swapchain
        try
        {
		    presentResult = m_Queue.presentKHR(presentInfo);
        }
        catch (const vk::OutOfDateKHRError&)
        {
            b_NeedRecreationSwapchain = true;
            advanceFrame();
            return;
        }
		switch (presentResult)
		{
			case vk::Result::eSuccess:
                b_NeedRecreationSwapchain = false;
                advanceFrame();
				break;
			case vk::Result::eSuboptimalKHR:
            case vk::Result::eErrorOutOfDateKHR:
                b_NeedRecreationSwapchain = true;
                advanceFrame();
                return;
			default:
                throw std::runtime_error("failed to present swap chain image!");
		}
    }
    // void HALIBUTRenderer::BeginOffscreenPass(HALIBUTOffscreenImage &offscreenImage, HALIBUTDepthImage &depthImage, const vec4f &clearColor)
    // {
    //     if (m_IsRenderingActive)
    //     {
    //         throw std::runtime_error("render pass already active");
    //     }
    //     m_ActivePassBarrierPlan = BuildOffscreenPassBarrierPlan(offscreenImage, depthImage);
    //     ApplyImageTransitions(m_ActivePassBarrierPlan.BeginTransitions);
    //     offscreenImage.SetCurrentLayout(vk::ImageLayout::eColorAttachmentOptimal);
    //     depthImage.SetCurrentLayout(vk::ImageLayout::eDepthAttachmentOptimal);

    //     vk::RenderingAttachmentInfo attachmentInfo{
    //         .imageView = *offscreenImage.GetImageView(),
    //         .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
    //         .loadOp = vk::AttachmentLoadOp::eClear,
    //         .storeOp = vk::AttachmentStoreOp::eStore,
    //         .clearValue = vk::ClearColorValue(std::array<float, 4>{clearColor.x, clearColor.y, clearColor.z, clearColor.w})
    //     };
    //     vk::RenderingAttachmentInfo depthAttachmentInfo{
    //         .imageView = *depthImage.GetImageView(),
    //         .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
    //         .loadOp = vk::AttachmentLoadOp::eClear,
    //         .storeOp = vk::AttachmentStoreOp::eStore,
    //         .clearValue = vk::ClearDepthStencilValue(1.0f, 0)
    //     };
    //     vk::RenderingInfo renderingInfo{
    //         .renderArea = {.offset = {0, 0}, .extent = offscreenImage.GetExtent()},
    //         .layerCount = 1,
    //         .colorAttachmentCount = 1,
    //         .pColorAttachments = &attachmentInfo,
    //         .pDepthAttachment = &depthAttachmentInfo
    //     };
    //     GetCommandBuffer().beginRendering(renderingInfo);
    //     GetCommandBuffer().setViewport(0, vk::Viewport(
    //         0.0f,
    //         0.0f,
    //         static_cast<float>(offscreenImage.GetExtent().width),
    //         static_cast<float>(offscreenImage.GetExtent().height),
    //         0.0f,
    //         1.0f
    //     ));
    //     GetCommandBuffer().setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), offscreenImage.GetExtent()));
    //     m_ActivePassType = ActivePassType::Offscreen;
    //     m_IsRenderingActive = true;
    // }
    // void HALIBUTRenderer::EndOffscreenPass(HALIBUTOffscreenImage &offscreenImage)
    // {
    //     if (!m_IsRenderingActive)
    //     {
    //         throw std::runtime_error("no active render pass to end");
    //     }
    //     if (m_ActivePassType != ActivePassType::Offscreen)
    //     {
    //         throw std::runtime_error("active pass type mismatch for offscreen pass end");
    //     }

    //     GetCommandBuffer().endRendering();
    //     ApplyImageTransitions(m_ActivePassBarrierPlan.EndTransitions);
    //     offscreenImage.SetCurrentLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    //     m_ActivePassBarrierPlan.Clear();
    //     m_ActivePassType = ActivePassType::None;
    //     m_IsRenderingActive = false;
    // }
    void HALIBUTRenderer::BeginSwapchainPass()
    {
        if (m_IsRenderingActive)
        {
            throw std::runtime_error("render pass already active");
        }
        m_ActivePassBarrierPlan = BuildSwapchainPassBarrierPlan(
            m_Swapchain.GetSwapChainImages()[m_AcquiredImageIndex],
            m_SwapchainImageLayouts[m_AcquiredImageIndex]
        );
        ApplyImageTransitions(m_ActivePassBarrierPlan.BeginTransitions);
        m_SwapchainImageLayouts[m_AcquiredImageIndex] = vk::ImageLayout::eColorAttachmentOptimal;

        vk::RenderingAttachmentInfo attachmentInfo{
            .imageView = *m_Swapchain.GetImageViews()[m_AcquiredImageIndex],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = vk::ClearColorValue(std::array<float, 4>{m_ClearColor.x, m_ClearColor.y, m_ClearColor.z, m_ClearColor.w})
        };
        vk::RenderingInfo renderingInfo{
            .renderArea = {.offset = {0, 0}, .extent = m_Swapchain.GetSwapChainExtent()},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentInfo
        };
        GetCommandBuffer().beginRendering(renderingInfo);
        GetCommandBuffer().setViewport(0, vk::Viewport(
            0.0f,
            0.0f,
            static_cast<float>(m_Swapchain.GetSwapChainExtent().width),
            static_cast<float>(m_Swapchain.GetSwapChainExtent().height),
            0.0f,
            1.0f
        ));
        GetCommandBuffer().setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_Swapchain.GetSwapChainExtent()));
        m_ActivePassType = ActivePassType::Swapchain;
        m_IsRenderingActive = true;
    }
    void HALIBUTRenderer::EndSwapchainPass()
    {
        if (!m_IsRenderingActive)
        {
            throw std::runtime_error("no active render pass to end");
        }
        if (m_ActivePassType != ActivePassType::Swapchain)
        {
            throw std::runtime_error("active pass type mismatch for swapchain pass end");
        }
        GetCommandBuffer().endRendering();
        ApplyImageTransitions(m_ActivePassBarrierPlan.EndTransitions);
        m_SwapchainImageLayouts[m_AcquiredImageIndex] = vk::ImageLayout::ePresentSrcKHR;
        m_ActivePassBarrierPlan.Clear();
        m_ActivePassType = ActivePassType::None;
        m_IsRenderingActive = false;
    }
    void HALIBUTRenderer::SetClearColor(glm::vec4 &clearcolor)
    {
        m_ClearColor = clearcolor;
    }
    void HALIBUTRenderer::BindPipeline(HALIBUTGraphicPipeline &pipeline)
    {
        GetCommandBuffer().bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            *pipeline.GetPipeline()
        );
    }
    void HALIBUTRenderer::SetViewport(uint32_t position, vk::Viewport &viewport)
    {
        GetCommandBuffer().setViewport(position, viewport);
    }
    void HALIBUTRenderer::SetViewport(uint32_t position, const float &x, const float &y, const float &minDepth, const float &maxDepth)
    {
        GetCommandBuffer().setViewport(position, vk::Viewport(
            x, y, 
            static_cast<float>(m_Swapchain.GetSwapChainExtent().width),
            static_cast<float>(m_Swapchain.GetSwapChainExtent().height),
            minDepth, maxDepth
        ));
    }
    void HALIBUTRenderer::SetScissor(uint32_t position, vk::Rect2D &scissor)
    {
        GetCommandBuffer().setScissor(position,scissor);
    }
    void HALIBUTRenderer::SetScissor(uint32_t position, const float &OffsetX, const float &OffsetY)
    {
        GetCommandBuffer().setScissor(position,vk::Rect2D(
                vk::Offset2D(OffsetX, OffsetY),
                m_Swapchain.GetSwapChainExtent()
            )
        );
    }
    void HALIBUTRenderer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        GetCommandBuffer().draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }
    ImageTransition HALIBUTRenderer::CreateImageTransition(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask, vk::ImageAspectFlags aspectMask)
    {
        return ImageTransition{
            .Image = image,
            .OldLayout = oldLayout,
            .NewLayout = newLayout,
            .SrcAccessMask = srcAccessMask,
            .DstAccessMask = dstAccessMask,
            .SrcStageMask = srcStageMask,
            .DstStageMask = dstStageMask,
            .AspectMask = aspectMask
        };
    }
    void HALIBUTRenderer::TransitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask, vk::ImageAspectFlags aspectMask)
    {
        vk::ImageMemoryBarrier2 barrier{
            .srcStageMask = srcStageMask,
            .srcAccessMask = srcAccessMask,
            .dstStageMask = dstStageMask,
            .dstAccessMask = dstAccessMask,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                .aspectMask = aspectMask,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        vk::DependencyInfo dependencyInfo{
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
        };
        GetCommandBuffer().pipelineBarrier2(dependencyInfo);
    }
    void HALIBUTRenderer::ApplyImageTransitions(const std::vector<ImageTransition> &transitions)
    {
        for (const ImageTransition& transition : transitions)
        {
            TransitionImageLayout(
                transition.Image,
                transition.OldLayout,
                transition.NewLayout,
                transition.SrcAccessMask,
                transition.DstAccessMask,
                transition.SrcStageMask,
                transition.DstStageMask,
                transition.AspectMask
            );
        }
    }
    // PassBarrierPlan HALIBUTRenderer::BuildOffscreenPassBarrierPlan(HALIBUTOffscreenImage &offscreenImage, HALIBUTDepthImage &depthImage) const
    // {
    //      PassBarrierPlan plan;
    //     const vk::ImageLayout offscreenOldLayout = offscreenImage.GetCurrentLayout();
    //     const bool fromShaderRead = offscreenOldLayout == vk::ImageLayout::eShaderReadOnlyOptimal;
    //     plan.BeginTransitions.push_back(CreateImageTransition(
    //         *offscreenImage.GetImage(),
    //         offscreenOldLayout,
    //         vk::ImageLayout::eColorAttachmentOptimal,
    //         fromShaderRead ? vk::AccessFlagBits2::eShaderRead : vk::AccessFlags2{},
    //         vk::AccessFlagBits2::eColorAttachmentWrite,
    //         fromShaderRead ? vk::PipelineStageFlagBits2::eFragmentShader : vk::PipelineStageFlagBits2::eTopOfPipe,
    //         vk::PipelineStageFlagBits2::eColorAttachmentOutput
    //     ));
    //     if (depthImage.GetCurrentLayout() != vk::ImageLayout::eDepthAttachmentOptimal)
    //     {
    //         plan.BeginTransitions.push_back(CreateImageTransition(
    //             *depthImage.GetImage(),
    //             depthImage.GetCurrentLayout(),
    //             vk::ImageLayout::eDepthAttachmentOptimal,
    //             {},
    //             vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
    //             vk::PipelineStageFlagBits2::eTopOfPipe,
    //             vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
    //             vk::ImageAspectFlagBits::eDepth
    //         ));
    //     }
    //     plan.EndTransitions.push_back(CreateImageTransition(
    //         *offscreenImage.GetImage(),
    //         vk::ImageLayout::eColorAttachmentOptimal,
    //         vk::ImageLayout::eShaderReadOnlyOptimal,
    //         vk::AccessFlagBits2::eColorAttachmentWrite,
    //         vk::AccessFlagBits2::eShaderRead,
    //         vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    //         vk::PipelineStageFlagBits2::eFragmentShader
    //     ));
    //     return plan;
    // }
    PassBarrierPlan HALIBUTRenderer::BuildSwapchainPassBarrierPlan(vk::Image image, vk::ImageLayout oldLayout) const
    {
        PassBarrierPlan plan;
        const vk::PipelineStageFlags2 srcStageMask =
            (oldLayout == vk::ImageLayout::ePresentSrcKHR)
                ? vk::PipelineStageFlagBits2::eColorAttachmentOutput
                : vk::PipelineStageFlagBits2::eTopOfPipe;
        plan.BeginTransitions.push_back(CreateImageTransition(
            image,
            oldLayout,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},
            vk::AccessFlagBits2::eColorAttachmentWrite,
            srcStageMask,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput
        ));
        plan.EndTransitions.push_back(CreateImageTransition(
            image,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            {},
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eBottomOfPipe
        ));
        return plan;
    }
}