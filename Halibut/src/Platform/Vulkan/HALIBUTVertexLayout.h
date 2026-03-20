#pragma once
#include "Core/Core.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <type_traits>
#include <vulkan/vulkan_raii.hpp>
#include <vector>

namespace HALIBUT
{
    struct HALIBUTVertexBindingDesc
    {
        uint32_t Binding = 0;
        uint32_t Stride = 0;
        vk::VertexInputRate InputRate = vk::VertexInputRate::eVertex;
    };

    struct HALIBUTVertexAttributeDesc
    {
        uint32_t Location = 0;
        uint32_t Binding = 0;
        vk::Format Format = vk::Format::eUndefined;
        uint32_t Offset = 0;
    };

    template<typename T>
    struct HALIBUTVertexFormatTraits
    {
        static constexpr bool kIsSupported = false;

        static constexpr vk::Format GetFormat()
        {
            return vk::Format::eUndefined;
        }
    };

    template<>
    struct HALIBUTVertexFormatTraits<glm::vec2>
    {
        static constexpr bool kIsSupported = true;

        static constexpr vk::Format GetFormat()
        {
            return vk::Format::eR32G32Sfloat;
        }
    };

    template<>
    struct HALIBUTVertexFormatTraits<glm::vec3>
    {
        static constexpr bool kIsSupported = true;

        static constexpr vk::Format GetFormat()
        {
            return vk::Format::eR32G32B32Sfloat;
        }
    };

    template<>
    struct HALIBUTVertexFormatTraits<glm::vec4>
    {
        static constexpr bool kIsSupported = true;

        static constexpr vk::Format GetFormat()
        {
            return vk::Format::eR32G32B32A32Sfloat;
        }
    };

    class HALIBUT_API HALIBUTVertexLayout
    {
    public:
        // 指定容量
        HALIBUTVertexLayout& ReserveBindings(size_t count)
        {
            m_Bindings.reserve(count);
            return *this;
        }

        HALIBUTVertexLayout& ReserveAttributes(size_t count)
        {
            m_Attributes.reserve(count);
            return *this;
        }
        // 清除
        HALIBUTVertexLayout& Clear()
        {
            m_Bindings.clear();
            m_Attributes.clear();
            return *this;
        }

        // 绑定数据
        template<typename TVertex>
        HALIBUTVertexLayout& AddBinding(
            uint32_t binding = 0,
            vk::VertexInputRate inputRate = vk::VertexInputRate::eVertex
        )
        {
            static_assert(std::is_standard_layout_v<TVertex>, "Vertex type must be standard-layout");
            return AddBinding(binding, static_cast<uint32_t>(sizeof(TVertex)), inputRate);
        }

        HALIBUTVertexLayout& AddBinding(
            uint32_t binding,
            uint32_t stride,
            vk::VertexInputRate inputRate = vk::VertexInputRate::eVertex
        )
        {
            m_Bindings.push_back(HALIBUTVertexBindingDesc{
                .Binding = binding,
                .Stride = stride,
                .InputRate = inputRate
            });
            return *this;
        }
        // 向 shader 描述数据结构
        template<typename TVertex, typename TMember>
        HALIBUTVertexLayout& AddAttribute(
            uint32_t location,
            uint32_t binding,
            vk::Format format,
            TMember TVertex::* member
        )
        {
            static_assert(std::is_standard_layout_v<TVertex>, "Vertex type must be standard-layout");
            static_assert(std::is_default_constructible_v<TVertex>, "Vertex type must be default-constructible");

            const TVertex vertex{};
            const auto* basePtr = reinterpret_cast<const char*>(&vertex);
            const auto* memberPtr = reinterpret_cast<const char*>(&(vertex.*member));
            const uint32_t offset = static_cast<uint32_t>(memberPtr - basePtr);
            return AddAttribute(location, binding, format, offset);
        }

        template<typename TVertex, typename TMember>
        HALIBUTVertexLayout& AddAttribute(
            uint32_t location,
            uint32_t binding,
            TMember TVertex::* member
        )
        {
            using MemberType = std::remove_cv_t<TMember>;
            static_assert(
                HALIBUTVertexFormatTraits<MemberType>::kIsSupported,
                "Unsupported vertex attribute type. Add a HALIBUTVertexFormatTraits specialization."
            );
            return AddAttribute(
                location,
                binding,
                HALIBUTVertexFormatTraits<MemberType>::GetFormat(),
                member
            );
        }

        HALIBUTVertexLayout& AddAttribute(
            uint32_t location,
            uint32_t binding,
            vk::Format format,
            uint32_t offset
        )
        {
            m_Attributes.push_back(HALIBUTVertexAttributeDesc{
                .Location = location,
                .Binding = binding,
                .Format = format,
                .Offset = offset
            });
            return *this;
        }

        const std::vector<HALIBUTVertexBindingDesc>& GetBindings() const { return m_Bindings; }
        const std::vector<HALIBUTVertexAttributeDesc>& GetAttributes() const { return m_Attributes; }
        bool IsEmpty() const { return m_Bindings.empty() && m_Attributes.empty(); }

    private:
        std::vector<HALIBUTVertexBindingDesc> m_Bindings;
        std::vector<HALIBUTVertexAttributeDesc> m_Attributes;
    };
}
