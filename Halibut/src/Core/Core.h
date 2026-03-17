#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#if defined(_WIN32)
  #if defined(HALIBUT_SHARED_LIB)
    #if defined(HALIBUT_BUILD_DLL)
      #define HALIBUT_API __declspec(dllexport)
    #else
      #define HALIBUT_API __declspec(dllimport)
    #endif
  #else
    #define HALIBUT_API
  #endif
#elif defined(__GNUC__) || defined(__clang__)
  #if defined(HALIBUT_SHARED_LIB) && defined(HALIBUT_BUILD_DLL)
    #define HALIBUT_API __attribute__((visibility("default")))
  #else
    #define HALIBUT_API
  #endif
#else
  #define HALIBUT_API
#endif

#define BIT(x) (1 << x)
#define HALIBUT_BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

namespace HALIBUT
{
    template<typename T>
    using UPtr = std::unique_ptr<T>;

    template<typename T, typename... Args>
    constexpr UPtr<T> MakeUPtr(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    struct vec3
    {
        uint32_t x,y,z;
    };

    struct vec4
    {
        uint32_t x,y,z,w;
    };

    struct vec3f
    {
        float x,y,z,w;
    };
    struct vec4f
    {
        float x,y,z,w;
    };


}
