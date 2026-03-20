#include "halibutpch.h"
#include "HALIBUTBuffer.h"

namespace HALIBUT
{
    namespace
    {
        vk::DeviceSize AlignUp(vk::DeviceSize value, vk::DeviceSize alignment)
        {
            if (alignment <= 1)
            {
                return value;
            }
            return (value + alignment - 1) & ~(alignment - 1);
        }

        vk::DeviceSize AlignDown(vk::DeviceSize value, vk::DeviceSize alignment)
        {
            if (alignment <= 1)
            {
                return value;
            }
            return value & ~(alignment - 1);
        }

        vk::DeviceSize ResolveSize(vk::DeviceSize totalSize, vk::DeviceSize offset, vk::DeviceSize size)
        {
            if (offset >= totalSize)
            {
                throw std::runtime_error("buffer offset is out of range");
            }

            const vk::DeviceSize resolvedSize = (size == VK_WHOLE_SIZE) ? (totalSize - offset) : size;
            if (resolvedSize == 0 || offset + resolvedSize > totalSize)
            {
                throw std::runtime_error("buffer range is out of bounds");
            }
            return resolvedSize;
        }
    }

    HALIBUTBuffer::HALIBUTBuffer(HALIBUTDevice& device, const HALIBUTBufferDesc& desc)
        : m_Device(device),
          m_Size(desc.Size),
          m_Usage(desc.Usage),
          m_MemoryProperties(desc.MemoryProperties)
    {
        if (m_Size == 0)
        {
            throw std::runtime_error("buffer size must be greater than zero");
        }
        if (m_Usage == vk::BufferUsageFlags{})
        {
            throw std::runtime_error("buffer usage must not be empty");
        }
        if (m_MemoryProperties == vk::MemoryPropertyFlags{})
        {
            throw std::runtime_error("buffer memory properties must not be empty");
        }
        if (desc.PersistentMap && !IsHostVisible())
        {
            throw std::runtime_error("persistent mapping requires host visible memory");
        }

        vk::BufferCreateInfo bufferCreateInfo{
            .size = m_Size,
            .usage = m_Usage,
            .sharingMode = desc.SharingMode
        };

        std::array<uint32_t, 2> queueFamilyIndices = {
            m_Device.GetQueueFamilyIndices().graphicsFamily.value(),
            m_Device.GetQueueFamilyIndices().presentFamily.value()
        };
        if (desc.SharingMode == vk::SharingMode::eConcurrent &&
            queueFamilyIndices[0] != queueFamilyIndices[1])
        {
            bufferCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
            bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        }
        else
        {
            bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        }

        m_Buffer = vk::raii::Buffer(m_Device.GetDevice(), bufferCreateInfo);

        const vk::MemoryRequirements memoryRequirements = m_Buffer.getMemoryRequirements();
        const uint32_t memoryTypeIndex = m_Device.FindMemoryType(
            memoryRequirements.memoryTypeBits,
            m_MemoryProperties
        );

        vk::MemoryAllocateInfo memoryAllocateInfo{
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = memoryTypeIndex
        };
        m_Memory = vk::raii::DeviceMemory(m_Device.GetDevice(), memoryAllocateInfo);
        m_Buffer.bindMemory(*m_Memory, 0);

        if (desc.PersistentMap)
        {
            Map();
        }
    }

    HALIBUTBuffer::~HALIBUTBuffer()
    {
        Unmap();
    }

    void* HALIBUTBuffer::Map(vk::DeviceSize offset, vk::DeviceSize size)
    {
        if (!IsHostVisible())
        {
            throw std::runtime_error("cannot map a buffer without host visible memory");
        }

        ResolveSize(m_Size, offset, size);
        if (m_MappedPtr == nullptr)
        {
            m_MappedPtr = m_Memory.mapMemory(0, m_Size);
        }

        return static_cast<char*>(m_MappedPtr) + static_cast<std::ptrdiff_t>(offset);
    }

    void HALIBUTBuffer::Unmap()
    {
        if (m_MappedPtr == nullptr)
        {
            return;
        }

        m_Memory.unmapMemory();
        m_MappedPtr = nullptr;
    }

    void HALIBUTBuffer::Write(const void* data, vk::DeviceSize size, vk::DeviceSize offset)
    {
        if (size == 0)
        {
            return;
        }
        if (data == nullptr)
        {
            throw std::runtime_error("buffer write source data is null");
        }

        ResolveSize(m_Size, offset, size);
        void* destinationPtr = Map(offset, size);
        std::memcpy(destinationPtr, data, static_cast<size_t>(size));
    }

    void HALIBUTBuffer::Flush(vk::DeviceSize offset, vk::DeviceSize size)
    {
        if (!IsHostVisible())
        {
            throw std::runtime_error("cannot flush a buffer without host visible memory");
        }
        if ((m_MemoryProperties & vk::MemoryPropertyFlagBits::eHostCoherent) != vk::MemoryPropertyFlags{})
        {
            return;
        }
        if (m_MappedPtr == nullptr)
        {
            throw std::runtime_error("cannot flush an unmapped buffer");
        }

        const vk::DeviceSize resolvedSize = ResolveSize(m_Size, offset, size);
        const vk::DeviceSize nonCoherentAtomSize =
            m_Device.GetPhysicalDevice().getProperties().limits.nonCoherentAtomSize;
        const vk::DeviceSize alignedOffset = AlignDown(offset, nonCoherentAtomSize);
        const vk::DeviceSize alignedEnd = std::min(
            AlignUp(offset + resolvedSize, nonCoherentAtomSize),
            m_Size
        );

        const vk::MappedMemoryRange mappedMemoryRange{
            .memory = *m_Memory,
            .offset = alignedOffset,
            .size = alignedEnd - alignedOffset
        };
        m_Device.GetDevice().flushMappedMemoryRanges(mappedMemoryRange);
    }

    void HALIBUTBuffer::Invalidate(vk::DeviceSize offset, vk::DeviceSize size)
    {
        if (!IsHostVisible())
        {
            throw std::runtime_error("cannot invalidate a buffer without host visible memory");
        }
        if ((m_MemoryProperties & vk::MemoryPropertyFlagBits::eHostCoherent) != vk::MemoryPropertyFlags{})
        {
            return;
        }
        if (m_MappedPtr == nullptr)
        {
            throw std::runtime_error("cannot invalidate an unmapped buffer");
        }

        const vk::DeviceSize resolvedSize = ResolveSize(m_Size, offset, size);
        const vk::DeviceSize nonCoherentAtomSize =
            m_Device.GetPhysicalDevice().getProperties().limits.nonCoherentAtomSize;
        const vk::DeviceSize alignedOffset = AlignDown(offset, nonCoherentAtomSize);
        const vk::DeviceSize alignedEnd = std::min(
            AlignUp(offset + resolvedSize, nonCoherentAtomSize),
            m_Size
        );

        const vk::MappedMemoryRange mappedMemoryRange{
            .memory = *m_Memory,
            .offset = alignedOffset,
            .size = alignedEnd - alignedOffset
        };
        m_Device.GetDevice().invalidateMappedMemoryRanges(mappedMemoryRange);
    }

    bool HALIBUTBuffer::IsHostVisible() const
    {
        return (m_MemoryProperties & vk::MemoryPropertyFlagBits::eHostVisible) != vk::MemoryPropertyFlags{};
    }
}
