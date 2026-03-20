#pragma once
#include "Core/Core.h"
#include "HALIBUTDevice.h"
#include "HALIBUTSwapchain.h"
#include "HALIBUTVertexLayout.h"
#include <vulkan/vulkan_raii.hpp>
#include <vector>
namespace HALIBUT
{
    class HALIBUT_API HALIBUTGraphicPipeline
    {
    public:
        HALIBUTGraphicPipeline(
            const std::string& vertexShaderFileName,
            const std::string& fragmentShaderFileName,
            HALIBUTDevice& device,
            HALIBUTSwapchain& swapchain,
            const HALIBUTVertexLayout& vertexLayout = HALIBUTVertexLayout{},
            vk::Format depthFormat = vk::Format::eUndefined,
            vk::Format colorAttachmentFormat = vk::Format::eUndefined,
            vk::CullModeFlags cullMode = vk::CullModeFlagBits::eNone
        );
        ~HALIBUTGraphicPipeline();
        inline vk::raii::Pipeline& GetPipeline() { return m_GraphicsPipeline; }
        inline vk::raii::PipelineLayout& GetPipelineLayout() { return m_PipelineLayout; }
    private:
        std::vector<char> ReadFile(const std::string& filename);
        [[nodiscard]] vk::raii::ShaderModule CreateShaderModule(vk::raii::Device& device, const std::vector<char>& code) const;
    private:
        HALIBUTDevice& m_Device;
        HALIBUTSwapchain& m_Swapchain;

        std::vector<char> m_VertexCode;
        std::vector<char> m_FragmentCode;
        // TOFO:
        // Geometry Shader
        // Tessellation Control Shader
        // Tessellation Evaluation Shader

        vk::raii::PipelineLayout m_PipelineLayout = nullptr;
        vk::raii::Pipeline m_GraphicsPipeline = nullptr;
    };
}
