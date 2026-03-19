#include "halibutpch.h"
#include "HALIBUTGraphicPipeline.h"

namespace HALIBUT
{
    HALIBUTGraphicPipeline::HALIBUTGraphicPipeline(
        const std::string &vertexShaderFileName, 
        const std::string &fragmentShaderFileName, 
        HALIBUTDevice &device, 
        HALIBUTSwapchain &swapchain, 
        vk::Format depthFormat, 
        vk::Format colorAttachmentFormat, 
        vk::CullModeFlags cullMode
    )
        :m_Device(device),
        m_Swapchain(swapchain)
    {
        // 读取 vertex shader 和 fragment shader的代码
        m_VertexCode = ReadFile(vertexShaderFileName);
		m_FragmentCode = ReadFile(fragmentShaderFileName);
        // 初始化着色器
        vk::raii::ShaderModule vertShaderModule = CreateShaderModule(m_Device.GetDevice(), m_VertexCode);
		vk::raii::ShaderModule fragShaderModule = CreateShaderModule(m_Device.GetDevice(), m_FragmentCode);
        // 初始化着色器配置
        vk::PipelineShaderStageCreateInfo vertShaderStageInfo{.stage = vk::ShaderStageFlagBits::eVertex, .module = *vertShaderModule, .pName = "main"};
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{.stage = vk::ShaderStageFlagBits::eFragment, .module = *fragShaderModule, .pName = "main"};
		vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
        // 告诉 pipeline 绑定上来的 vertex buffer，里面的数据到底长什么样
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
        // 管道输入装配状态，告诉 pipeline 顶点如何组装成图元
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};
        // 告诉视口和裁剪配置
		vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};
        // 告诉 pipeline 怎么栅格化成像素
		vk::PipelineRasterizationStateCreateInfo rasterizer{
			.depthClampEnable = VK_FALSE, 
			.rasterizerDiscardEnable = VK_FALSE, 
			.polygonMode = vk::PolygonMode::eFill, 
			.cullMode = cullMode, 
			.frontFace = vk::FrontFace::eCounterClockwise, 
			.depthBiasEnable = VK_FALSE, 
			.depthBiasSlopeFactor = 1.0f, 
			.lineWidth = 1.0f
		};
        // Multisample State 多重采样配置 （MASS）
        vk::PipelineMultisampleStateCreateInfo multisampling{
			.rasterizationSamples = vk::SampleCountFlagBits::e1, 
			.sampleShadingEnable = VK_FALSE
		};
        // 告诉pipeline Depth 深度测试 / Stencil 模板 State
        vk::PipelineDepthStencilStateCreateInfo depthStencil{
            .depthTestEnable = VK_FALSE,
            .depthWriteEnable = VK_FALSE,
            .depthCompareOp = vk::CompareOp::eLess,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE
        };
        //  blend 配置，决定 fragment shader 输出的颜色，写入 color attachment 时怎么混合
        vk::PipelineColorBlendAttachmentState colorBlendAttachment{
			.blendEnable         = VK_TRUE,
		    .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
            .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .colorBlendOp        = vk::BlendOp::eAdd,
            .srcAlphaBlendFactor = vk::BlendFactor::eOne,
            .dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .alphaBlendOp        = vk::BlendOp::eAdd,
		    .colorWriteMask      = 
			vk::ColorComponentFlagBits::eR | 
			vk::ColorComponentFlagBits::eG | 
			vk::ColorComponentFlagBits::eB | 
			vk::ColorComponentFlagBits::eA
		};
        vk::PipelineColorBlendStateCreateInfo colorBlending{
			.logicOpEnable = VK_FALSE, 
			.logicOp = vk::LogicOp::eCopy, 
			.attachmentCount = 1, 
			.pAttachments = &colorBlendAttachment
		};
        // 动态渲染声明
        std::vector dynamicStates = {
		    vk::DynamicState::eViewport,
		    vk::DynamicState::eScissor
		};
        vk::PipelineDynamicStateCreateInfo dynamicState{
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), 
			.pDynamicStates = dynamicStates.data()
		};
        // 管线布局 
        // descriptor set layout 有哪些
        // push constant range 有哪些
        
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        m_PipelineLayout = vk::raii::PipelineLayout(m_Device.GetDevice(), pipelineLayoutInfo);
        // 告诉 pipeline 它会输出到什么格式的 attachment 上
        const vk::Format resolvedColorAttachmentFormat =
            colorAttachmentFormat == vk::Format::eUndefined ? m_Swapchain.GetSwapchainSurfaceFormat().format : colorAttachmentFormat;
		vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &resolvedColorAttachmentFormat,
            .depthAttachmentFormat = depthFormat
        };
		vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
		    .pNext               = &pipelineRenderingCreateInfo,
		    .stageCount          = 2,
		    .pStages             = shaderStages,
		    .pVertexInputState   = &vertexInputInfo,
		    .pInputAssemblyState = &inputAssembly,
		    .pViewportState      = &viewportState,
		    .pRasterizationState = &rasterizer,
		    .pMultisampleState   = &multisampling,
            .pDepthStencilState  = &depthStencil,
		    .pColorBlendState    = &colorBlending,
		    .pDynamicState       = &dynamicState,
		    .layout              = *m_PipelineLayout,
		    .renderPass          = nullptr
        };

        const vk::Optional<const vk::raii::PipelineCache> pipelineCache(m_Device.GetPipelineCache());
		m_GraphicsPipeline = vk::raii::Pipeline(m_Device.GetDevice(), pipelineCache, pipelineCreateInfo);

    }

    HALIBUTGraphicPipeline::~HALIBUTGraphicPipeline()
    {
    }
    std::vector<char> HALIBUTGraphicPipeline::ReadFile(const std::string &filename)
    {
        namespace fs = std::filesystem;
        static std::unordered_map<std::string, std::vector<char>> s_ShaderCodeCache;

		fs::path inputPath(filename);
		fs::path resolvedPath = inputPath;
		if (!inputPath.is_absolute())
		{
			resolvedPath = fs::absolute(inputPath).lexically_normal();
		}
        const std::string cacheKey = resolvedPath.string();
        if (const auto cacheIt = s_ShaderCodeCache.find(cacheKey); cacheIt != s_ShaderCodeCache.end())
        {
            return cacheIt->second;
        }

		std::ifstream file(resolvedPath, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error(
				"failed to open file! input='" + filename +
				"', resolved='" + resolvedPath.string() +
				"', cwd='" + fs::current_path().string() + "'");
		}
		std::vector<char> buffer(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		file.close();
        s_ShaderCodeCache.emplace(cacheKey, buffer);
		return buffer;
    }
    vk::raii::ShaderModule HALIBUTGraphicPipeline::CreateShaderModule(vk::raii::Device &device, const std::vector<char> &code) const
    {
        if ((code.size() % sizeof(uint32_t)) != 0)
		{
			throw std::runtime_error("invalid shader bytecode size, not aligned to uint32_t");
		}
		vk::ShaderModuleCreateInfo createInfo{.codeSize = code.size(), .pCode = reinterpret_cast<const uint32_t *>(code.data())};
		vk::raii::ShaderModule m_ShaderModule{device, createInfo};
		return m_ShaderModule;
    }
}
