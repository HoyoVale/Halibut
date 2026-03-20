#include "halibutpch.h"
#include "HALIBUTDevice.h"

namespace HALIBUT
{
    namespace
    {
        bool ContainsIgnoreCase(const std::string& text, const std::string& pattern)
        {
            if (pattern.empty())
            {
                return true;
            }
            const auto toLower = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };
            std::string lowerText(text.size(), '\0');
            std::string lowerPattern(pattern.size(), '\0');
            std::transform(text.begin(), text.end(), lowerText.begin(), toLower);
            std::transform(pattern.begin(), pattern.end(), lowerPattern.begin(), toLower);
            return lowerText.find(lowerPattern) != std::string::npos;
        }

        const char* DeviceTypeToString(vk::PhysicalDeviceType type)
        {
            switch (type)
            {
            case vk::PhysicalDeviceType::eOther: return "Other";
            case vk::PhysicalDeviceType::eIntegratedGpu: return "Integrated";
            case vk::PhysicalDeviceType::eDiscreteGpu: return "Discrete";
            case vk::PhysicalDeviceType::eVirtualGpu: return "Virtual";
            case vk::PhysicalDeviceType::eCpu: return "CPU";
            default: return "Unknown";
            }
        }

        int BaseDeviceScore(const vk::PhysicalDeviceProperties& deviceProperties)
        {
            int score = 0;
            switch (deviceProperties.deviceType)
            {
            case vk::PhysicalDeviceType::eDiscreteGpu:
                score += 20000;
                break;
            case vk::PhysicalDeviceType::eIntegratedGpu:
                score += 10000;
                break;
            case vk::PhysicalDeviceType::eVirtualGpu:
                score += 2000;
                break;
            case vk::PhysicalDeviceType::eCpu:
                score -= 10000;
                break;
            default:
                break;
            }
            score += static_cast<int>(deviceProperties.limits.maxImageDimension2D);

            const std::string deviceName = deviceProperties.deviceName;
            if (ContainsIgnoreCase(deviceName, "lavapipe") ||
                ContainsIgnoreCase(deviceName, "llvmpipe") ||
                ContainsIgnoreCase(deviceName, "swiftshader"))
            {
                score -= 5000;
            }
            return score;
        }

        bool HasExtension(const std::unordered_set<std::string>& extensions, const char* extensionName)
        {
            return extensions.find(extensionName) != extensions.end();
        }

        std::string ApiVersionToString(uint32_t apiVersion)
        {
            return std::to_string(VK_API_VERSION_MAJOR(apiVersion)) + "." +
                   std::to_string(VK_API_VERSION_MINOR(apiVersion)) + "." +
                   std::to_string(VK_API_VERSION_PATCH(apiVersion));
        }

        std::optional<bool> ParseOptionalBoolEnv(const char* envValue)
        {
            if (envValue == nullptr || envValue[0] == '\0')
            {
                return std::nullopt;
            }

            std::string value(envValue);
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });

            if (value == "1" || value == "true" || value == "yes" || value == "on")
            {
                return true;
            }
            if (value == "0" || value == "false" || value == "no" || value == "off")
            {
                return false;
            }
            return std::nullopt;
        }

        vk::DeviceSize AlignUp(vk::DeviceSize value, vk::DeviceSize alignment)
        {
            if (alignment <= 1)
            {
                return value;
            }
            return (value + alignment - 1) & ~(alignment - 1);
        }

        std::optional<uint32_t> TryFindMemoryTypeIndex(
            const vk::PhysicalDeviceMemoryProperties& memoryProperties,
            uint32_t typeFilter,
            vk::MemoryPropertyFlags properties
        )
        {
            for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
            {
                if ((typeFilter & (1u << i)) &&
                    (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    return i;
                }
            }
            return std::nullopt;
        }
    }

    HALIBUTDevice::HALIBUTDevice(HALIBUTInstance& instance, HALIBUTSurface& surface)
        :m_Instance(instance),
        m_Surface(surface)
    {
        PickPhysicalDevice();
        CreateLogicalDevice();
    }
    HALIBUTDevice::~HALIBUTDevice()
    {
        SavePipelineCache();
        if (m_UploadRingMappedPtr != nullptr)
        {
            m_UploadRingMemory.unmapMemory();
            m_UploadRingMappedPtr = nullptr;
        }
    }


    QueueFamilyIndices HALIBUTDevice::findQueueFamilies(vk::raii::PhysicalDevice const& physicalDevice) const
    {
        QueueFamilyIndices queueFamilyIndices{};
        auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

        for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyProperties.size(); queueFamilyIndex++)
        {
            if ((queueFamilyProperties[queueFamilyIndex].queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0))
            {
                queueFamilyIndices.graphicsFamily = queueFamilyIndex;
            }
            if (physicalDevice.getSurfaceSupportKHR(queueFamilyIndex, *m_Surface.GetSurface()))
            {
                queueFamilyIndices.presentFamily = queueFamilyIndex;
            }
            if (queueFamilyIndices.IsComplete())
            {
                break;
            }
        }
        return queueFamilyIndices;
    }

    HALIBUTDeviceCapabilityProfile HALIBUTDevice::BuildCapabilityProfile(vk::raii::PhysicalDevice const& physicalDevice) const
    {
        HALIBUTDeviceCapabilityProfile capabilityProfile{};
        const vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
        capabilityProfile.ApiVersion = deviceProperties.apiVersion;

        const auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
        capabilityProfile.AvailableExtensionNames.reserve(availableExtensions.size());
        for (const auto& extension : availableExtensions)
        {
            capabilityProfile.AvailableExtensionNames.insert(extension.extensionName);
        }

        capabilityProfile.HasSwapchain = HasExtension(capabilityProfile.AvailableExtensionNames, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        const vk::PhysicalDeviceFeatures supportedFeatures = physicalDevice.getFeatures();
        capabilityProfile.HasSamplerAnisotropy = supportedFeatures.samplerAnisotropy == VK_TRUE;

        bool supportsCoreDynamicRendering = false;
        bool supportsCoreSynchronization2 = false;
        if (deviceProperties.apiVersion >= VK_API_VERSION_1_3)
        {
            const auto featureChain = physicalDevice.getFeatures2<
                vk::PhysicalDeviceFeatures2,
                vk::PhysicalDeviceVulkan13Features
            >();
            const vk::PhysicalDeviceVulkan13Features& vulkan13Features =
                featureChain.get<vk::PhysicalDeviceVulkan13Features>();
            supportsCoreDynamicRendering = vulkan13Features.dynamicRendering == VK_TRUE;
            supportsCoreSynchronization2 = vulkan13Features.synchronization2 == VK_TRUE;
        }
        capabilityProfile.SupportsCoreDynamicRendering = supportsCoreDynamicRendering;
        capabilityProfile.SupportsCoreSynchronization2 = supportsCoreSynchronization2;

        bool supportsKhrDynamicRendering = HasExtension(capabilityProfile.AvailableExtensionNames, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        bool supportsKhrSynchronization2 = HasExtension(capabilityProfile.AvailableExtensionNames, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
        if (supportsKhrDynamicRendering && supportsKhrSynchronization2)
        {
            const auto featureChain = physicalDevice.getFeatures2<
                vk::PhysicalDeviceFeatures2,
                vk::PhysicalDeviceDynamicRenderingFeaturesKHR,
                vk::PhysicalDeviceSynchronization2FeaturesKHR
            >();
            const vk::PhysicalDeviceDynamicRenderingFeaturesKHR& dynamicRenderingFeatures =
                featureChain.get<vk::PhysicalDeviceDynamicRenderingFeaturesKHR>();
            const vk::PhysicalDeviceSynchronization2FeaturesKHR& synchronization2Features =
                featureChain.get<vk::PhysicalDeviceSynchronization2FeaturesKHR>();
            supportsKhrDynamicRendering = dynamicRenderingFeatures.dynamicRendering == VK_TRUE;
            supportsKhrSynchronization2 = synchronization2Features.synchronization2 == VK_TRUE;
        }
        capabilityProfile.SupportsKhrDynamicRendering = supportsKhrDynamicRendering;
        capabilityProfile.SupportsKhrSynchronization2 = supportsKhrSynchronization2;

        if (capabilityProfile.SupportsCoreDynamicRendering && capabilityProfile.SupportsCoreSynchronization2)
        {
            capabilityProfile.EnablesCoreDynamicRendering = true;
        }
        else if (capabilityProfile.SupportsKhrDynamicRendering && capabilityProfile.SupportsKhrSynchronization2)
        {
            capabilityProfile.EnablesKhrDynamicRendering = true;
        }

        capabilityProfile.EnabledDeviceExtensions.clear();
        if (capabilityProfile.HasSwapchain)
        {
            capabilityProfile.EnabledDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }
        if (capabilityProfile.EnablesKhrDynamicRendering)
        {
            capabilityProfile.EnabledDeviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
            capabilityProfile.EnabledDeviceExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
        }

        return capabilityProfile;
    }


    void HALIBUTDevice::PickPhysicalDevice()
    {
        auto devices = vk::raii::PhysicalDevices(m_Instance.GetInstance());
        if (devices.empty())
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        const char* preferredDeviceNameEnv = std::getenv("HALIBUT_VK_PREFERRED_DEVICE");
        const std::string preferredDeviceName = preferredDeviceNameEnv != nullptr ? preferredDeviceNameEnv : "";
        int forcedDeviceIndex = -1;
        if (const char* forcedDeviceIndexEnv = std::getenv("HALIBUT_VK_DEVICE_INDEX"); forcedDeviceIndexEnv != nullptr && forcedDeviceIndexEnv[0] != '\0')
        {
            try
            {
                forcedDeviceIndex = std::stoi(forcedDeviceIndexEnv);
            }
            catch (const std::exception&)
            {
                std::cout << "[HALIBUT] Invalid HALIBUT_VK_DEVICE_INDEX='" << forcedDeviceIndexEnv << "', ignoring." << std::endl;
            }
        }

        int bestScore = -1;
        VkPhysicalDevice bestPhysicalDevice = VK_NULL_HANDLE;
        QueueFamilyIndices bestQueueFamilyIndices{};
        HALIBUTDeviceCapabilityProfile bestCapabilityProfile{};
        std::string bestDeviceName;
        int llvmPipeScore = std::numeric_limits<int>::min();
        VkPhysicalDevice llvmPipePhysicalDevice = VK_NULL_HANDLE;
        QueueFamilyIndices llvmPipeQueueFamilyIndices{};
        HALIBUTDeviceCapabilityProfile llvmPipeCapabilityProfile{};
        std::string llvmPipeDeviceName;
        bool hasMicrosoftD3D12Candidate = false;

        std::cout << "[HALIBUT] Vulkan device discovery begin." << std::endl;
        if (!preferredDeviceName.empty())
        {
            std::cout << "[HALIBUT] Preferred device name contains: '" << preferredDeviceName << "'" << std::endl;
        }
        if (forcedDeviceIndex >= 0)
        {
            std::cout << "[HALIBUT] Forced device index: " << forcedDeviceIndex << std::endl;
        }

        for (size_t deviceIndex = 0; deviceIndex < devices.size(); ++deviceIndex)
        {
            const auto& device = devices[deviceIndex];
            const auto deviceProperties = device.getProperties();
            const std::string deviceName = deviceProperties.deviceName;
            const bool isLlvmPipeDevice = ContainsIgnoreCase(deviceName, "llvmpipe");
            const bool isMicrosoftD3D12Device = deviceName.find("Microsoft Direct3D12") != std::string::npos;
            if (isMicrosoftD3D12Device)
            {
                hasMicrosoftD3D12Candidate = true;
            }

            if (forcedDeviceIndex >= 0 && static_cast<int>(deviceIndex) != forcedDeviceIndex)
            {
                std::cout << "[HALIBUT] Device[" << deviceIndex << "] " << deviceName
                          << " skipped (forced index mismatch)." << std::endl;
                continue;
            }

            QueueFamilyIndices queueFamilyIndices = findQueueFamilies(device);
            if (!queueFamilyIndices.IsComplete())
            {
                std::cout << "[HALIBUT] Device[" << deviceIndex << "] " << deviceName
                          << " skipped (missing graphics/present queue)." << std::endl;
                continue;
            }
            const HALIBUTDeviceCapabilityProfile capabilityProfile = BuildCapabilityProfile(device);
            if (!capabilityProfile.IsRuntimeCompatible())
            {
                std::cout << "[HALIBUT] Device[" << deviceIndex << "] " << deviceName
                          << " skipped (capability mismatch: swapchain=" << (capabilityProfile.HasSwapchain ? "yes" : "no")
                          << ", dynamicRenderingPath=" << capabilityProfile.GetDynamicRenderingPathLabel() << ")." << std::endl;
                continue;
            }
            auto surfaceFormats      = device.getSurfaceFormatsKHR(*m_Surface.GetSurface());
            auto surfacePresentModes = device.getSurfacePresentModesKHR(*m_Surface.GetSurface());
            if (surfaceFormats.empty() || surfacePresentModes.empty())
            {
                std::cout << "[HALIBUT] Device[" << deviceIndex << "] " << deviceName
                          << " skipped (surface formats/present modes unavailable)." << std::endl;
                continue;
            }

            int score = BaseDeviceScore(deviceProperties);
            if (!preferredDeviceName.empty() && ContainsIgnoreCase(deviceName, preferredDeviceName))
            {
                score += 100000;
            }

            std::cout << "[HALIBUT] Device[" << deviceIndex << "] " << deviceName
                      << " type=" << DeviceTypeToString(deviceProperties.deviceType)
                      << " api=" << ApiVersionToString(capabilityProfile.ApiVersion)
                      << " dynPath=" << capabilityProfile.GetDynamicRenderingPathLabel()
                      << " score=" << score << std::endl;

            if (score > bestScore)
            {
                bestScore = score;
                bestPhysicalDevice = *device;
                bestQueueFamilyIndices = queueFamilyIndices;
                bestCapabilityProfile = capabilityProfile;
                bestDeviceName = deviceName;
            }

            if (isLlvmPipeDevice && score > llvmPipeScore)
            {
                llvmPipeScore = score;
                llvmPipePhysicalDevice = *device;
                llvmPipeQueueFamilyIndices = queueFamilyIndices;
                llvmPipeCapabilityProfile = capabilityProfile;
                llvmPipeDeviceName = deviceName;
            }
        }

        const bool hasExplicitSelection = forcedDeviceIndex >= 0 || !preferredDeviceName.empty();
        bool defaultToLlvmPipe = false;
        if (!hasExplicitSelection)
        {
            const char* defaultLlvmPipeEnv = std::getenv("HALIBUT_VK_DEFAULT_LLVMPIPE");
            const std::optional<bool> defaultLlvmPipe = ParseOptionalBoolEnv(defaultLlvmPipeEnv);
            if (defaultLlvmPipeEnv != nullptr && !defaultLlvmPipe.has_value())
            {
                std::cout << "[HALIBUT] Invalid HALIBUT_VK_DEFAULT_LLVMPIPE='" << defaultLlvmPipeEnv
                          << "', expected 0/1/true/false, using auto policy." << std::endl;
            }
            defaultToLlvmPipe = defaultLlvmPipe.value_or(hasMicrosoftD3D12Candidate);
        }
        if (defaultToLlvmPipe)
        {
            if (llvmPipePhysicalDevice != VK_NULL_HANDLE)
            {
                bestScore = llvmPipeScore;
                bestPhysicalDevice = llvmPipePhysicalDevice;
                bestQueueFamilyIndices = llvmPipeQueueFamilyIndices;
                bestCapabilityProfile = llvmPipeCapabilityProfile;
                bestDeviceName = llvmPipeDeviceName;
                std::cout << "[HALIBUT] Defaulting Vulkan device to llvmpipe for stable functional regression. "
                          << "Set HALIBUT_VK_DEFAULT_LLVMPIPE=0 to disable." << std::endl;
            }
            else
            {
                std::cout << "[HALIBUT] llvmpipe default requested but no llvmpipe candidate was found; "
                          << "falling back to best available GPU candidate." << std::endl;
            }
        }

        if (bestPhysicalDevice != VK_NULL_HANDLE && bestScore > 0)
        {
            m_QueueFamilyIndices = bestQueueFamilyIndices;
            m_CapabilityProfile = bestCapabilityProfile;
            m_PhysicalDevice = vk::raii::PhysicalDevice(m_Instance.GetInstance(), bestPhysicalDevice);
            std::cout << "[HALIBUT] Selected Vulkan device: " << bestDeviceName
                      << " (score=" << bestScore
                      << ", api=" << ApiVersionToString(m_CapabilityProfile.ApiVersion)
                      << ", dynPath=" << m_CapabilityProfile.GetDynamicRenderingPathLabel()
                      << ")" << std::endl;
        }
        else
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void HALIBUTDevice::CreateLogicalDevice()
    {
        if (!m_QueueFamilyIndices.IsComplete())
        {
            throw std::runtime_error("Could not find queue families for graphics and present");
        }

        float queuePriority = 1.0f;
        std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            m_QueueFamilyIndices.graphicsFamily.value(),
            m_QueueFamilyIndices.presentFamily.value()
        };

        for (uint32_t queueFamilyIndex : uniqueQueueFamilies)
        {
            deviceQueueCreateInfos.push_back(
                vk::DeviceQueueCreateInfo{
                    .queueFamilyIndex = queueFamilyIndex,
                    .queueCount       = m_DeviceQueueCount,
                    .pQueuePriorities = &queuePriority
                }
            );
        }

        vk::PhysicalDeviceVulkan11Features vulkan11Features{};
        vulkan11Features.shaderDrawParameters = VK_TRUE;

        vk::PhysicalDeviceVulkan13Features vulkan13Features{};
        vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{};
        vk::PhysicalDeviceSynchronization2FeaturesKHR synchronization2Features{};
        void* featureChainHead = &vulkan11Features;

        m_DeviceFeatures = vk::PhysicalDeviceFeatures{};
        m_DeviceFeatures.samplerAnisotropy = m_CapabilityProfile.HasSamplerAnisotropy ? VK_TRUE : VK_FALSE;

        if (!m_CapabilityProfile.IsRuntimeCompatible())
        {
            throw std::runtime_error("Selected Vulkan device capability profile is not runtime-compatible.");
        }

        if (m_CapabilityProfile.EnablesCoreDynamicRendering)
        {
            vulkan13Features.pNext = &vulkan11Features;
            vulkan13Features.dynamicRendering = VK_TRUE;
            vulkan13Features.synchronization2 = VK_TRUE;
            featureChainHead = &vulkan13Features;
        }
        else if (m_CapabilityProfile.EnablesKhrDynamicRendering)
        {
            dynamicRenderingFeatures.pNext = &synchronization2Features;
            dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
            synchronization2Features.pNext = &vulkan11Features;
            synchronization2Features.synchronization2 = VK_TRUE;
            featureChainHead = &dynamicRenderingFeatures;
        }
        else
        {
            throw std::runtime_error("Selected Vulkan device does not support required dynamic rendering/synchronization2 features.");
        }

        vk::DeviceCreateInfo deviceCreateInfo{
            .pNext                   = featureChainHead,
            .queueCreateInfoCount    = static_cast<uint32_t>(deviceQueueCreateInfos.size()),
            .pQueueCreateInfos       = deviceQueueCreateInfos.data(),
            .pEnabledFeatures        = &m_DeviceFeatures
        };

        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_CapabilityProfile.EnabledDeviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = m_CapabilityProfile.EnabledDeviceExtensions.data();

        m_Device = vk::raii::Device(m_PhysicalDevice, deviceCreateInfo);
        m_GraphicsQueue = vk::raii::Queue(m_Device, m_QueueFamilyIndices.graphicsFamily.value(), m_QueueCount);
        m_PresentQueue = vk::raii::Queue(m_Device, m_QueueFamilyIndices.presentFamily.value(), m_QueueCount);
        CreatePipelineCache();
        CreateImmediateSubmitContext();
        std::cout << "[HALIBUT] Logical device capabilities: api=" << ApiVersionToString(m_CapabilityProfile.ApiVersion)
                  << ", dynamicRenderingPath=" << m_CapabilityProfile.GetDynamicRenderingPathLabel()
                  << ", samplerAnisotropy=" << (m_CapabilityProfile.HasSamplerAnisotropy ? "yes" : "no")
                  << ", enabledExtensions=" << m_CapabilityProfile.EnabledDeviceExtensions.size() << std::endl;
    }

    void HALIBUTDevice::CreatePipelineCache()
    {
        const char* pipelineCachePathEnv = std::getenv("HALIBUT_PIPELINE_CACHE_PATH");
        m_PipelineCachePath = (pipelineCachePathEnv != nullptr && pipelineCachePathEnv[0] != '\0')
            ? pipelineCachePathEnv
            : ".HALIBUT_cache/pipeline_cache.bin";

        std::vector<uint8_t> initialPipelineCacheData;
        if (std::filesystem::exists(m_PipelineCachePath))
        {
            std::ifstream cacheFile(m_PipelineCachePath, std::ios::binary | std::ios::ate);
            if (cacheFile.is_open())
            {
                const std::streamsize cacheSize = cacheFile.tellg();
                if (cacheSize > 0)
                {
                    initialPipelineCacheData.resize(static_cast<size_t>(cacheSize));
                    cacheFile.seekg(0, std::ios::beg);
                    cacheFile.read(reinterpret_cast<char*>(initialPipelineCacheData.data()), cacheSize);
                }
            }
        }

        vk::PipelineCacheCreateInfo pipelineCacheCreateInfo{
            .initialDataSize = initialPipelineCacheData.size(),
            .pInitialData = initialPipelineCacheData.empty() ? nullptr : initialPipelineCacheData.data()
        };

        try
        {
            m_PipelineCache = vk::raii::PipelineCache(m_Device, pipelineCacheCreateInfo);
            if (!initialPipelineCacheData.empty())
            {
                std::cout << "[HALIBUT] Pipeline cache loaded: " << initialPipelineCacheData.size()
                          << " bytes from " << m_PipelineCachePath << std::endl;
            }
        }
        catch (const std::exception&)
        {
            // Fall back to an empty pipeline cache when cached data is incompatible.
            m_PipelineCache = vk::raii::PipelineCache(m_Device, vk::PipelineCacheCreateInfo{});
            std::cout << "[HALIBUT] Pipeline cache warm load failed, fallback to empty cache." << std::endl;
        }
    }

    void HALIBUTDevice::SavePipelineCache()
    {
        if (m_PipelineCachePath.empty())
        {
            return;
        }

        try
        {
            const std::vector<uint8_t> pipelineCacheData = m_PipelineCache.getData();
            if (pipelineCacheData.empty())
            {
                return;
            }

            const std::filesystem::path cachePath(m_PipelineCachePath);
            if (const std::filesystem::path parentPath = cachePath.parent_path(); !parentPath.empty())
            {
                std::error_code ec;
                std::filesystem::create_directories(parentPath, ec);
                if (ec)
                {
                    std::cout << "[HALIBUT] Pipeline cache directory creation failed: " << ec.message() << std::endl;
                    return;
                }
            }

            std::ofstream cacheFile(cachePath, std::ios::binary | std::ios::trunc);
            if (!cacheFile.is_open())
            {
                std::cout << "[HALIBUT] Failed to open pipeline cache file for write: " << m_PipelineCachePath << std::endl;
                return;
            }
            cacheFile.write(reinterpret_cast<const char*>(pipelineCacheData.data()),
                            static_cast<std::streamsize>(pipelineCacheData.size()));
            std::cout << "[HALIBUT] Pipeline cache saved: " << pipelineCacheData.size()
                      << " bytes to " << m_PipelineCachePath << std::endl;
        }
        catch (const std::exception& exception)
        {
            std::cout << "[HALIBUT] Pipeline cache save failed: " << exception.what() << std::endl;
        }
    }

    void HALIBUTDevice::CreateImmediateSubmitContext()
    {
        vk::CommandPoolCreateInfo commandPoolCreateInfo{
            .flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = m_QueueFamilyIndices.graphicsFamily.value()
        };
        m_ImmediateCommandPool = vk::raii::CommandPool(m_Device, commandPoolCreateInfo);

        vk::CommandBufferAllocateInfo commandBufferAllocateInfo{
            .commandPool = *m_ImmediateCommandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        vk::raii::CommandBuffers commandBuffers(m_Device, commandBufferAllocateInfo);
        m_ImmediateCommandBuffer = std::move(commandBuffers.front());

        vk::FenceCreateInfo fenceCreateInfo{
            .flags = vk::FenceCreateFlagBits::eSignaled
        };
        m_ImmediateSubmitFence = vk::raii::Fence(m_Device, fenceCreateInfo);

        CreateUploadRingBuffer();
    }

    void HALIBUTDevice::CreateUploadRingBuffer()
    {
        uint64_t uploadRingMb = 64;
        if (const char* uploadRingMbEnv = std::getenv("HALIBUT_UPLOAD_RING_MB");
            uploadRingMbEnv != nullptr && uploadRingMbEnv[0] != '\0')
        {
            try
            {
                const int parsedValue = std::stoi(uploadRingMbEnv);
                if (parsedValue > 0)
                {
                    uploadRingMb = static_cast<uint64_t>(parsedValue);
                }
            }
            catch (const std::exception&)
            {
            }
        }
        if (uploadRingMb < 8)
        {
            uploadRingMb = 8;
        }
        m_UploadRingSize = static_cast<vk::DeviceSize>(uploadRingMb) * 1024u * 1024u;

        vk::BufferCreateInfo bufferCreateInfo{
            .size = m_UploadRingSize,
            .usage = vk::BufferUsageFlagBits::eTransferSrc,
            .sharingMode = vk::SharingMode::eExclusive
        };
        m_UploadRingBuffer = vk::raii::Buffer(m_Device, bufferCreateInfo);

        const vk::MemoryRequirements memoryRequirements = m_UploadRingBuffer.getMemoryRequirements();
        const vk::PhysicalDeviceMemoryProperties memoryProperties = m_PhysicalDevice.getMemoryProperties();

        const auto coherentMemoryTypeIndex = TryFindMemoryTypeIndex(
            memoryProperties,
            memoryRequirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        const auto hostVisibleMemoryTypeIndex = TryFindMemoryTypeIndex(
            memoryProperties,
            memoryRequirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible
        );
        if (!coherentMemoryTypeIndex.has_value() && !hostVisibleMemoryTypeIndex.has_value())
        {
            throw std::runtime_error("failed to find host visible memory type for upload ring buffer");
        }

        m_UploadRingHostCoherent = coherentMemoryTypeIndex.has_value();
        const uint32_t memoryTypeIndex = coherentMemoryTypeIndex.value_or(hostVisibleMemoryTypeIndex.value());
        vk::MemoryAllocateInfo memoryAllocateInfo{
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = memoryTypeIndex
        };
        m_UploadRingMemory = vk::raii::DeviceMemory(m_Device, memoryAllocateInfo);
        m_UploadRingBuffer.bindMemory(*m_UploadRingMemory, 0);
        m_UploadRingMappedPtr = m_UploadRingMemory.mapMemory(0, m_UploadRingSize);
        m_UploadRingHead = 0;
        std::cout << "[HALIBUT] Upload ring buffer created: sizeMB="
                  << uploadRingMb
                  << ", coherent=" << (m_UploadRingHostCoherent ? "yes" : "no")
                  << std::endl;
    }

    void HALIBUTDevice::BeginUploadBatch()
    {
        if (m_UploadBatchDepth > 0)
        {
            ++m_UploadBatchDepth;
            return;
        }

        const vk::Result waitResult = m_Device.waitForFences(*m_ImmediateSubmitFence, VK_TRUE, UINT64_MAX);
        if (waitResult != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to wait upload batch fence");
        }

        m_ImmediateCommandBuffer.reset(vk::CommandBufferResetFlags{});
        m_ImmediateCommandBuffer.begin(vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        });
        m_UploadBatchDepth = 1;
        m_UploadBatchOpen = true;
        m_UploadBatchHasCommands = false;
        m_UploadRingHead = 0;
    }

    void HALIBUTDevice::SubmitImmediateCommandBuffer()
    {
        m_ImmediateCommandBuffer.end();
        m_Device.resetFences(*m_ImmediateSubmitFence);

        const vk::CommandBuffer rawCommandBuffer = *m_ImmediateCommandBuffer;
        const vk::SubmitInfo submitInfo{
            .commandBufferCount = 1,
            .pCommandBuffers = &rawCommandBuffer
        };
        m_GraphicsQueue.submit(submitInfo, *m_ImmediateSubmitFence);

        const vk::Result submitWaitResult = m_Device.waitForFences(*m_ImmediateSubmitFence, VK_TRUE, UINT64_MAX);
        if (submitWaitResult != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to wait upload batch submit completion");
        }
    }

    void HALIBUTDevice::FlushUploadBatchSegment()
    {
        if (m_UploadBatchDepth == 0 || !m_UploadBatchOpen)
        {
            return;
        }
        if (m_UploadBatchHasCommands)
        {
            SubmitImmediateCommandBuffer();

            m_ImmediateCommandBuffer.reset(vk::CommandBufferResetFlags{});
            m_ImmediateCommandBuffer.begin(vk::CommandBufferBeginInfo{
                .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
            });
            m_UploadBatchHasCommands = false;
        }
        m_UploadRingHead = 0;
    }

    HALIBUTDevice::UploadBufferSlice HALIBUTDevice::AllocateUploadRingSlice(vk::DeviceSize size, vk::DeviceSize alignment)
    {
        if (size == 0)
        {
            throw std::runtime_error("upload ring slice size must be greater than zero");
        }
        if (size > m_UploadRingSize)
        {
            throw std::runtime_error("upload ring slice exceeds ring buffer capacity");
        }
        if (alignment == 0)
        {
            alignment = 1;
        }

        vk::DeviceSize alignedHead = AlignUp(m_UploadRingHead, alignment);
        if (alignedHead + size > m_UploadRingSize)
        {
            if (m_UploadBatchDepth > 0)
            {
                FlushUploadBatchSegment();
            }
            alignedHead = 0;
            m_UploadRingHead = 0;
        }
        if (alignedHead + size > m_UploadRingSize)
        {
            throw std::runtime_error("upload ring slice allocation failed");
        }

        UploadBufferSlice slice{};
        slice.Buffer = *m_UploadRingBuffer;
        slice.Offset = alignedHead;
        slice.Size = size;
        m_UploadRingHead = alignedHead + size;
        return slice;
    }

    void HALIBUTDevice::CopyBufferToImageImmediate(vk::Buffer sourceBuffer, vk::DeviceSize sourceOffset, vk::Image destinationImage, vk::ImageLayout destinationImageLayout, const vk::ImageSubresourceLayers &imageSubresourceLayers, vk::Extent3D imageExtent, vk::Offset3D imageOffset)
    {
        if (imageExtent.width == 0 || imageExtent.height == 0 || imageExtent.depth == 0)
        {
            throw std::runtime_error("CopyBufferToImageImmediate image extent must be non-zero");
        }

        ImmediateSubmit([&](vk::CommandBuffer commandBuffer) {
            const vk::BufferImageCopy copyRegion{
                .bufferOffset = sourceOffset,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = imageSubresourceLayers,
                .imageOffset = imageOffset,
                .imageExtent = imageExtent
            };

            commandBuffer.copyBufferToImage(
                sourceBuffer,
                destinationImage,
                destinationImageLayout,
                copyRegion
            );
        });
    }

    HALIBUTDevice::UploadBufferSlice HALIBUTDevice::StageUploadData(
        const void *sourceData,
        vk::DeviceSize size,
        vk::DeviceSize alignment)
    {
        if (sourceData == nullptr || size == 0)
        {
            throw std::runtime_error("upload staging source data is empty");
        }
        if (m_UploadRingMappedPtr == nullptr)
        {
            throw std::runtime_error("upload ring buffer is not mapped");
        }

        const UploadBufferSlice slice = AllocateUploadRingSlice(size, alignment);
        std::memcpy(
            static_cast<char*>(m_UploadRingMappedPtr) + static_cast<std::ptrdiff_t>(slice.Offset),
            sourceData,
            static_cast<size_t>(size)
        );

        if (!m_UploadRingHostCoherent)
        {
            const vk::MappedMemoryRange mappedMemoryRange{
                .memory = *m_UploadRingMemory,
                .offset = slice.Offset,
                .size = slice.Size
            };
            m_Device.flushMappedMemoryRanges(mappedMemoryRange);
        }

        return slice;
    }

    void HALIBUTDevice::EndUploadBatch()
    {
        if (m_UploadBatchDepth == 0)
        {
            throw std::runtime_error("EndUploadBatch called without active batch");
        }
        --m_UploadBatchDepth;
        if (m_UploadBatchDepth > 0)
        {
            return;
        }

        if (!m_UploadBatchOpen)
        {
            return;
        }

        if (m_UploadBatchHasCommands)
        {
            SubmitImmediateCommandBuffer();
        }
        else
        {
            m_ImmediateCommandBuffer.end();
        }

        m_UploadBatchOpen = false;
        m_UploadBatchHasCommands = false;
        m_UploadRingHead = 0;
    }

    void HALIBUTDevice::ImmediateSubmit(const std::function<void(vk::CommandBuffer)>& recordCommands)
    {
        if (!recordCommands)
        {
            throw std::runtime_error("ImmediateSubmit callback is null");
        }

        const bool autoBatch = (m_UploadBatchDepth == 0);
        if (autoBatch)
        {
            BeginUploadBatch();
        }

        if (!m_UploadBatchOpen)
        {
            throw std::runtime_error("upload batch command buffer is not open");
        }

        recordCommands(*m_ImmediateCommandBuffer);
        m_UploadBatchHasCommands = true;

        if (autoBatch)
        {
            EndUploadBatch();
        }
    }

    uint32_t HALIBUTDevice::FindMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags properties) const
    {
        const vk::PhysicalDeviceMemoryProperties memoryProperties = m_PhysicalDevice.getMemoryProperties();
        const std::optional<uint32_t> memoryTypeIndex = TryFindMemoryTypeIndex(
            memoryProperties,
            typeBits,
            properties
        );

        if (!memoryTypeIndex.has_value())
        {
            throw std::runtime_error("failed to find suitable Vulkan memory type");
        }

        return memoryTypeIndex.value();
    }

    void HALIBUTDevice::CopyBufferImmediate(vk::Buffer sourceBuffer, vk::Buffer destinationBuffer, vk::DeviceSize size)
    {
        CopyBufferImmediate(sourceBuffer, 0, destinationBuffer, 0, size);
    }
    void HALIBUTDevice::CopyBufferImmediate(vk::Buffer src, vk::DeviceSize srcOffset, vk::Buffer dst, vk::DeviceSize dstOffset, vk::DeviceSize size)
    {
        if (size == 0)
        {
            throw std::runtime_error("CopyBufferImmediate size must be greater than zero");
        }

        ImmediateSubmit([&](vk::CommandBuffer commandBuffer) {
            const vk::BufferCopy copyRegion{
                .srcOffset = srcOffset,
                .dstOffset = dstOffset,
                .size = size
            };
            commandBuffer.copyBuffer(src, dst, copyRegion);
        });
    }
}
