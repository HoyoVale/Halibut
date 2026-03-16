#include "halibutpch.h"
#include "ApplicationImGuiContext.h"
#include <cctype>
#include <GLFW/glfw3.h>

namespace HALIBUT
{
    namespace
    {
        struct ImGuiFontCandidate
        {
            const char* Label;
            const char* Path;
        };

#if defined(_WIN32)
        constexpr auto kImGuiFontCandidates = std::to_array<ImGuiFontCandidate>({
            {"Microsoft YaHei", "C:/Windows/Fonts/msyh.ttc"},
            {"Microsoft YaHei UI", "C:/Windows/Fonts/msyh.ttf"},
            {"SimHei", "C:/Windows/Fonts/simhei.ttf"},
            {"SimSun", "C:/Windows/Fonts/simsun.ttc"},
            {"Noto Sans CJK SC", "C:/Windows/Fonts/NotoSansCJK-Regular.ttc"}
        });
#elif defined(__linux__)
        constexpr auto kImGuiFontCandidates = std::to_array<ImGuiFontCandidate>({
            {"Noto Sans CJK SC", "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"},
            {"Noto Sans CJK SC (TTF)", "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc"},
            {"WenQuanYi Zen Hei", "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc"},
            {"AR PL UMing", "/usr/share/fonts/truetype/arphic/uming.ttc"}
        });
#else
        constexpr std::array<ImGuiFontCandidate, 0> kImGuiFontCandidates{};
#endif

        std::string Trim(std::string value)
        {
            auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), [isSpace](char ch) {
                return !isSpace(static_cast<unsigned char>(ch));
            }));
            value.erase(std::find_if(value.rbegin(), value.rend(), [isSpace](char ch) {
                return !isSpace(static_cast<unsigned char>(ch));
            }).base(), value.end());
            return value;
        }
    }

    void ApplicationImGuiContext::LoadUiSettings()
    {
        std::ifstream settingsFile(m_UiSettingsPath);
        if (!settingsFile.is_open())
        {
            return;
        }

        std::string line;
        while (std::getline(settingsFile, line))
        {
            const std::string trimmedLine = Trim(line);
            if (trimmedLine.empty() || trimmedLine[0] == '#')
            {
                continue;
            }

            const size_t equalsPosition = trimmedLine.find('=');
            if (equalsPosition == std::string::npos)
            {
                continue;
            }

            const std::string key = Trim(trimmedLine.substr(0, equalsPosition));
            const std::string value = Trim(trimmedLine.substr(equalsPosition + 1));

            try
            {
                if (key == "font_scale")
                {
                    m_FontScale = std::clamp(std::stof(value), 0.5f, 2.5f);
                }
                else if (key == "font_preset")
                {
                    m_FontPresetIndex = std::max(std::stoi(value), 0);
                    m_HasPersistedFontPreset = true;
                }
            }
            catch (const std::exception&)
            {
            }
        }
    }

    void ApplicationImGuiContext::SaveUiSettings() const
    {
        std::ofstream settingsFile(m_UiSettingsPath, std::ios::trunc);
        if (!settingsFile.is_open())
        {
            return;
        }

        settingsFile << "# HoyoEditor UI settings\n";
        settingsFile << "font_scale=" << m_FontScale << "\n";
        settingsFile << "font_preset=" << m_FontPresetIndex << "\n";
    }

    void ApplicationImGuiContext::Initialize(Window& window, HALIBUTInstance& instance, HALIBUTDevice& device, HALIBUTSwapchain& swapchain)
    {
        if (m_IsInitialized)
        {
            return;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        SetupFonts();
        io.FontGlobalScale = m_FontScale;
        ImGui::StyleColorsDark();

        constexpr uint32_t kImGuiTextureDescriptorCapacity = 1024;
        std::array descriptorPoolSizes = {
            vk::DescriptorPoolSize{
                vk::DescriptorType::eCombinedImageSampler,
                kImGuiTextureDescriptorCapacity
            }
        };
        vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = kImGuiTextureDescriptorCapacity,
            .poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size()),
            .pPoolSizes = descriptorPoolSizes.data()
        };
        m_DescriptorPool = vk::raii::DescriptorPool(device.GetDevice(), descriptorPoolCreateInfo);

        InitPlatformBackend(window);
        InitVulkanBackend(instance, device, swapchain);
        m_IsInitialized = true;
    }

    void ApplicationImGuiContext::RecreateBackends(Window& window, HALIBUTInstance& instance, HALIBUTDevice& device, HALIBUTSwapchain& swapchain)
    {
        if (!m_IsInitialized)
        {
            return;
        }

        ShutdownVulkanBackend();
        ShutdownPlatformBackend();
        InitPlatformBackend(window);
        InitVulkanBackend(instance, device, swapchain);
    }

    void ApplicationImGuiContext::Shutdown()
    {
        if (!m_IsInitialized || ImGui::GetCurrentContext() == nullptr)
        {
            return;
        }

        ShutdownVulkanBackend();
        m_DescriptorPool = nullptr;
        ShutdownPlatformBackend();
        ImGui::DestroyContext();
        m_IsInitialized = false;
    }

    void ApplicationImGuiContext::BeginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ApplicationImGuiContext::Render(vk::raii::CommandBuffer& commandBuffer)
    {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandBuffer);
    }

    float ApplicationImGuiContext::GetFontScale() const
    {
        return m_FontScale;
    }

    void ApplicationImGuiContext::SetFontScale(float scale)
    {
        m_FontScale = std::clamp(scale, 0.5f, 2.5f);
        if (ImGui::GetCurrentContext() != nullptr)
        {
            ImGuiIO& io = ImGui::GetIO();
            io.FontGlobalScale = m_FontScale;
        }
    }

    int ApplicationImGuiContext::GetFontPresetIndex() const
    {
        return m_FontPresetIndex;
    }

    void ApplicationImGuiContext::SetFontPresetIndex(int presetIndex)
    {
        if (m_Fonts.empty() || ImGui::GetCurrentContext() == nullptr)
        {
            return;
        }
        m_FontPresetIndex = std::clamp(presetIndex, 0, static_cast<int>(m_Fonts.size()) - 1);
        m_HasPersistedFontPreset = true;
        ImGuiIO& io = ImGui::GetIO();
        io.FontDefault = m_Fonts[static_cast<size_t>(m_FontPresetIndex)];
    }

    const std::vector<std::string>& ApplicationImGuiContext::GetFontPresetNames() const
    {
        return m_FontPresetNames;
    }

    void ApplicationImGuiContext::InitPlatformBackend(Window& window)
    {
        ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window.GetNativeWindow(), true);
    }

    void ApplicationImGuiContext::InitVulkanBackend(HALIBUTInstance& instance, HALIBUTDevice& device, HALIBUTSwapchain& swapchain)
    {
        const HALIBUTDeviceCapabilityProfile& capabilityProfile = device.GetCapabilityProfile();
        if (!capabilityProfile.IsRuntimeCompatible())
        {
            throw std::runtime_error("ImGui backend requires a runtime-compatible Vulkan device capability profile.");
        }

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.ApiVersion = capabilityProfile.ApiVersion;
        initInfo.Instance = *instance.GetInstance();
        initInfo.PhysicalDevice = *device.GetPhysicalDevice();
        initInfo.Device = *device.GetDevice();
        initInfo.QueueFamily = device.GetQueueFamilyIndices().graphicsFamily.value();
        initInfo.Queue = *device.GetGraphicsQueue();
        initInfo.DescriptorPool = *m_DescriptorPool;
        initInfo.MinImageCount = 2;
        initInfo.ImageCount = static_cast<uint32_t>(swapchain.GetSwapChainImages().size());
        initInfo.UseDynamicRendering = true;

        static VkFormat colorFormat;
        colorFormat = static_cast<VkFormat>(swapchain.GetSwapchainSurfaceFormat().format);
        VkPipelineRenderingCreateInfoKHR renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = &colorFormat;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = renderingInfo;

        ImGui_ImplVulkan_Init(&initInfo);
    }

    void ApplicationImGuiContext::ShutdownPlatformBackend()
    {
        ImGui_ImplGlfw_Shutdown();
    }

    void ApplicationImGuiContext::ShutdownVulkanBackend()
    {
        ImGui_ImplVulkan_Shutdown();
    }

    void ApplicationImGuiContext::SetupFonts()
    {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        m_Fonts.clear();
        m_FontPresetNames.clear();

        ImFont* defaultFont = io.Fonts->AddFontDefault();
        m_Fonts.push_back(defaultFont);
        m_FontPresetNames.emplace_back("Default");

        ImFontConfig fontConfig{};
        fontConfig.OversampleH = 2;
        fontConfig.OversampleV = 2;
        fontConfig.PixelSnapH = false;
        const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesChineseSimplifiedCommon();
        constexpr float fontPixelSize = 18.0f;

        for (const ImGuiFontCandidate& candidate : kImGuiFontCandidates)
        {
            if (!std::filesystem::exists(candidate.Path))
            {
                continue;
            }

            ImFont* loadedFont = io.Fonts->AddFontFromFileTTF(candidate.Path, fontPixelSize, &fontConfig, glyphRanges);
            if (loadedFont != nullptr)
            {
                m_Fonts.push_back(loadedFont);
                m_FontPresetNames.emplace_back(candidate.Label);
            }
        }

        if (!m_Fonts.empty())
        {
            if (m_HasPersistedFontPreset)
            {
                m_FontPresetIndex = std::clamp(m_FontPresetIndex, 0, static_cast<int>(m_Fonts.size()) - 1);
            }
            else if (m_FontPresetNames.size() > 1)
            {
                m_FontPresetIndex = 1;
            }
            else
            {
                m_FontPresetIndex = 0;
            }
            io.FontDefault = m_Fonts[static_cast<size_t>(m_FontPresetIndex)];
        }
    }
}
