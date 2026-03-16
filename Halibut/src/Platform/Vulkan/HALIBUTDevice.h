#pragma once
#include "Core/Core.h"
#include "HALIBUTInstance.h"
#include "HALIBUTSurface.h"
#include <functional>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace HALIBUT
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool IsComplete() const
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct HALIBUTDeviceCapabilityProfile
    {
        uint32_t ApiVersion = VK_API_VERSION_1_0;
        bool HasSwapchain = false;
        bool HasSamplerAnisotropy = false;
        bool SupportsCoreDynamicRendering = false;
        bool SupportsCoreSynchronization2 = false;
        bool SupportsKhrDynamicRendering = false;
        bool SupportsKhrSynchronization2 = false;
        bool EnablesCoreDynamicRendering = false;
        bool EnablesKhrDynamicRendering = false;
        std::unordered_set<std::string> AvailableExtensionNames;
        std::vector<const char*> EnabledDeviceExtensions;

        bool IsRuntimeCompatible() const
        {
            return HasSwapchain && (EnablesCoreDynamicRendering || EnablesKhrDynamicRendering);
        }

        const char* GetDynamicRenderingPathLabel() const
        {
            if (EnablesCoreDynamicRendering)
            {
                return "core-1.3";
            }
            if (EnablesKhrDynamicRendering)
            {
                return "KHR-extensions";
            }
            return "unsupported";
        }
    };

    class HALIBUT_API HALIBUTDevice
    {
    public:
        struct UploadBufferSlice
        {
            vk::Buffer Buffer = vk::Buffer{};
            vk::DeviceSize Offset = 0;
            vk::DeviceSize Size = 0;
        };

        class ScopedUploadBatch
        {
        public:
            explicit ScopedUploadBatch(HALIBUTDevice& device)
                : m_Device(device)
            {
                m_Device.BeginUploadBatch();
            }

            ~ScopedUploadBatch() noexcept
            {
                try
                {
                    m_Device.EndUploadBatch();
                }
                catch (...)
                {
                }
            }

            ScopedUploadBatch(const ScopedUploadBatch&) = delete;
            ScopedUploadBatch& operator=(const ScopedUploadBatch&) = delete;

        private:
            HALIBUTDevice& m_Device;
        };

        HALIBUTDevice(HALIBUTInstance& instance, HALIBUTSurface& surface);
        ~HALIBUTDevice();

        inline QueueFamilyIndices& GetQueueFamilyIndices() { return m_QueueFamilyIndices; }
        inline vk::raii::PhysicalDevice& GetPhysicalDevice() { return m_PhysicalDevice; }
        inline vk::raii::Device& GetDevice() { return m_Device; }
        inline vk::raii::Queue& GetGraphicsQueue() { return m_GraphicsQueue; }
        inline vk::raii::Queue& GetPresentQueue() { return m_PresentQueue; }
        inline vk::raii::PipelineCache& GetPipelineCache() { return m_PipelineCache; }
        inline const HALIBUTDeviceCapabilityProfile& GetCapabilityProfile() const { return m_CapabilityProfile; }
        void BeginUploadBatch();
        void EndUploadBatch();
        bool IsUploadBatchActive() const { return m_UploadBatchDepth > 0; }
        void ImmediateSubmit(const std::function<void(vk::CommandBuffer)>& recordCommands);
        void CopyBufferImmediate(vk::Buffer sourceBuffer, vk::Buffer destinationBuffer, vk::DeviceSize size);
        UploadBufferSlice StageUploadData(
            const void* sourceData,
            vk::DeviceSize size,
            vk::DeviceSize alignment = 16
        );
    private:
        QueueFamilyIndices findQueueFamilies(vk::raii::PhysicalDevice const& physicalDevice) const;
        HALIBUTDeviceCapabilityProfile BuildCapabilityProfile(vk::raii::PhysicalDevice const& physicalDevice) const;
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreatePipelineCache();
        void SavePipelineCache();
        void CreateImmediateSubmitContext();
        void CreateUploadRingBuffer();
        void SubmitImmediateCommandBuffer();
        void FlushUploadBatchSegment();
        UploadBufferSlice AllocateUploadRingSlice(vk::DeviceSize size, vk::DeviceSize alignment);
    private:
        HALIBUTInstance&                     m_Instance;
        HALIBUTSurface&                      m_Surface;

        QueueFamilyIndices               m_QueueFamilyIndices;
        vk::raii::PhysicalDevice         m_PhysicalDevice = nullptr;
        vk::raii::Device                 m_Device         = nullptr;
        uint32_t                         m_DeviceQueueCount = 1;
        uint32_t                         m_QueueCount = 0;
        vk::PhysicalDeviceFeatures       m_DeviceFeatures = {};
        HALIBUTDeviceCapabilityProfile       m_CapabilityProfile;
        vk::raii::PipelineCache          m_PipelineCache = nullptr;
        std::string                      m_PipelineCachePath;
        vk::raii::Queue                  m_GraphicsQueue  = nullptr;
        vk::raii::Queue                  m_PresentQueue   = nullptr;
        vk::raii::CommandPool            m_ImmediateCommandPool = nullptr;
        vk::raii::CommandBuffer          m_ImmediateCommandBuffer = nullptr;
        vk::raii::Fence                  m_ImmediateSubmitFence = nullptr;
        vk::raii::Buffer                 m_UploadRingBuffer = nullptr;
        vk::raii::DeviceMemory           m_UploadRingMemory = nullptr;
        void*                            m_UploadRingMappedPtr = nullptr;
        vk::DeviceSize                   m_UploadRingSize = 0;
        vk::DeviceSize                   m_UploadRingHead = 0;
        bool                             m_UploadRingHostCoherent = true;
        uint32_t                         m_UploadBatchDepth = 0;
        bool                             m_UploadBatchOpen = false;
        bool                             m_UploadBatchHasCommands = false;
    };
}
