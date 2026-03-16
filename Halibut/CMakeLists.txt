add_library(Halibut STATIC)

find_package(Vulkan REQUIRED)

set(HVK_VENDOR_DIR "${CMAKE_CURRENT_LIST_DIR}/vendor")
set(GLFW_DIR "${CMAKE_CURRENT_LIST_DIR}/vendor/glfw")
set(GLFW_INCLUDE_DIR "${GLFW_DIR}/include")
set(GLFW_LIB_DIR "${GLFW_DIR}/lib-mingw-w64")

add_library(HVKGLFW INTERFACE)

if(MINGW)
  find_library(GLFW_LIBRARY
    NAMES glfw3 libglfw3.a
    PATHS "${GLFW_LIB_DIR}"
    NO_DEFAULT_PATH
    REQUIRED
  )
  target_include_directories(HVKGLFW INTERFACE "${GLFW_INCLUDE_DIR}")
  target_link_libraries(HVKGLFW INTERFACE "${GLFW_LIBRARY}")
else()
  find_package(glfw3 QUIET)
  if(TARGET glfw)
    target_link_libraries(HVKGLFW INTERFACE glfw)
  elseif(TARGET glfw::glfw)
    target_link_libraries(HVKGLFW INTERFACE glfw::glfw)
  else()
    find_library(GLFW_LIBRARY NAMES glfw glfw3 REQUIRED)
    target_link_libraries(HVKGLFW INTERFACE "${GLFW_LIBRARY}")
  endif()
endif()

set(IMGUI_DIR "${HVK_VENDOR_DIR}/imgui")
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
    HVKGLFW
    Vulkan::Vulkan
)
set_target_properties(imgui PROPERTIES FOLDER "ThirdParty")

set(STB_DIR "${HVK_VENDOR_DIR}/stb")
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
set_target_properties(stb PROPERTIES FOLDER "ThirdParty")

set(GLM_DIR "${HVK_VENDOR_DIR}/glm")
add_library(glm INTERFACE)
target_include_directories(glm
  SYSTEM INTERFACE
    "${GLM_DIR}"
)
target_link_libraries(glm
  INTERFACE
    HalibutEngine::cxx_std
)
set_target_properties(glm PROPERTIES FOLDER "ThirdParty")

file(GLOB_RECURSE Halibut_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/src/*.hpp"
  "${CMAKE_CURRENT_LIST_DIR}/src/*.h"
)

target_sources(Halibut PRIVATE ${Halibut_SOURCES})
target_compile_definitions(Halibut
  PUBLIC
    $<$<PLATFORM_ID:Windows>:HVK_PLATFORM_WINDOWS>
    $<$<PLATFORM_ID:Linux>:HVK_PLATFORM_LINUX>
    HVK_STATIC_LIB
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    $<$<PLATFORM_ID:Windows>:VK_USE_PLATFORM_WIN32_KHR>
    GLFW_INCLUDE_VULKAN 
  PRIVATE
    GLM_ENABLE_EXPERIMENTAL
    $<$<CONFIG:Debug>:HVK_DEBUG>
    $<$<CONFIG:Release>:HVK_RELEASE>
)
target_include_directories(Halibut
  PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/src"
  PRIVATE
    "${HVK_VENDOR_DIR}"
)

target_link_libraries(Halibut
  PUBLIC
    HalibutEngine::cxx_std
    HVKGLFW
    Vulkan::Vulkan
    imgui
    stb
    glm
)
