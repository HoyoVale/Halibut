add_library(Halibut STATIC)

target_precompile_headers(Halibut PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/src/halibutpch.h"
)

target_sources(Halibut PRIVATE 
  # src/
  "${CMAKE_CURRENT_LIST_DIR}/src/halibutpch.cpp"
  # src/Core
  "${CMAKE_CURRENT_LIST_DIR}/src/Core/Application.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/src/Core/ApplicationImGuiContext.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/src/Core/ApplicationRenderContext.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/src/Core/Layer.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/src/Core/LayerStack.cpp"
  # src/Platform/Vulkan
  "${CMAKE_CURRENT_LIST_DIR}/src/Platform/Vulkan/HALIBUTDevice.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/src/Platform/Vulkan/HALIBUTInstance.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/src/Platform/Vulkan/HALIBUTSurface.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/src/Platform/Vulkan/HALIBUTSwapchain.cpp"
  # src/Platform/Windows
  $<$<PLATFORM_ID:Windows>:
  "${CMAKE_CURRENT_LIST_DIR}/src/Platform/Windows/WindowsInput.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/src/Platform/Windows/WindowsWindow.cpp"
  >
  # src/Platform/Linux
  $<$<PLATFORM_ID:Linux>:
  ${CMAKE_CURRENT_LIST_DIR}/src/Platform/Linux/LinuxInput.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/Platform/Linux/LinuxWindow.cpp
  >
)


# 预处理变量
target_compile_definitions(Halibut
  PUBLIC
    $<$<PLATFORM_ID:Windows>:HALIBUT_PLATFORM_WINDOWS>
    $<$<PLATFORM_ID:Linux>:HALIBUT_PLATFORM_LINUX>
    HALIBUT_STATIC_LIB
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    $<$<PLATFORM_ID:Windows>:VK_USE_PLATFORM_WIN32_KHR>
    GLFW_INCLUDE_VULKAN 
  PRIVATE
    GLM_ENABLE_EXPERIMENTAL
    $<$<CONFIG:Debug>:HALIBUT_DEBUG>
    $<$<CONFIG:Release>:HALIBUT_RELEASE>
)

set(HALIBUT_VENDOR_DIR "${CMAKE_CURRENT_LIST_DIR}/vendor")

target_include_directories(Halibut
  PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/src"
  PRIVATE
    "${HALIBUT_VENDOR_DIR}"
)

################### 添加第三方库 ###################
find_package(Vulkan REQUIRED)

set(GLFW_DIR "${CMAKE_CURRENT_LIST_DIR}/vendor/glfw")
set(GLFW_INCLUDE_DIR "${GLFW_DIR}/include")
set(GLFW_LIB_DIR "${GLFW_DIR}/lib-mingw-w64") # 这里根据不同的编译器选择不同的lib库

# 添加 Glfw 库
add_library(HALIBUT_GLFW INTERFACE)

if(MINGW)
  find_library(GLFW_LIBRARY
    NAMES glfw3 libglfw3.a
    PATHS "${GLFW_LIB_DIR}"
    NO_DEFAULT_PATH
    REQUIRED
  )
  target_include_directories(HALIBUT_GLFW INTERFACE "${GLFW_INCLUDE_DIR}")
  target_link_libraries(HALIBUT_GLFW INTERFACE "${GLFW_LIBRARY}")
else()
  find_package(glfw3 QUIET)
  if(TARGET glfw)
    target_link_libraries(HALIBUT_GLFW INTERFACE glfw) 
  elseif(TARGET glfw::glfw)
    target_link_libraries(HALIBUT_GLFW INTERFACE glfw::glfw)
  else()
    find_library(GLFW_LIBRARY NAMES glfw glfw3 REQUIRED)
    target_link_libraries(HALIBUT_GLFW INTERFACE "${GLFW_LIBRARY}")
  endif()
endif()

# 添加 ImGui 库
set(IMGUI_DIR "${HALIBUT_VENDOR_DIR}/imgui")
add_library(imgui STATIC
  "${IMGUI_DIR}/imgui.cpp"
  "${IMGUI_DIR}/imgui_demo.cpp"
  "${IMGUI_DIR}/imgui_draw.cpp"
  "${IMGUI_DIR}/imgui_tables.cpp"
  "${IMGUI_DIR}/imgui_widgets.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
)
target_include_directories(imgui
  PUBLIC
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)


target_link_libraries(imgui
  PUBLIC
    HalibutEngine::cxx_std
    HALIBUT_GLFW
    Vulkan::Vulkan
)

# 将 imgui 库 设置为第三方库
set_target_properties(imgui PROPERTIES FOLDER "ThirdParty")

set(STB_DIR "${HALIBUT_VENDOR_DIR}/stb")
add_library(stb STATIC
  "${STB_DIR}/stb_image.cpp"
)
target_include_directories(stb
  PUBLIC
    "${STB_DIR}"
)
target_link_libraries(stb
  PUBLIC
    HalibutEngine::cxx_std
)
# 将 stb 库 设置为第三方库
set_target_properties(stb PROPERTIES FOLDER "ThirdParty")


# 添加 glm 库

set(GLM_DIR "${HALIBUT_VENDOR_DIR}/glm")
add_library(glm INTERFACE)
target_include_directories(glm
  SYSTEM INTERFACE
    "${GLM_DIR}"
)
target_link_libraries(glm
  INTERFACE
    HalibutEngine::cxx_std
)

# 将 glm 库 设置为第三方库
set_target_properties(glm PROPERTIES FOLDER "ThirdParty") 

# 链接所有库
target_link_libraries(Halibut
  PUBLIC
    HalibutEngine::cxx_std
    HALIBUT_GLFW
    Vulkan::Vulkan
    imgui
    stb
    glm
)
