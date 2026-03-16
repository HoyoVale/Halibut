add_executable(Sandbox)

file(GLOB_RECURSE Sandbox_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/src/*.hpp"
  "${CMAKE_CURRENT_LIST_DIR}/src/*.h"
)


target_sources(Sandbox 
PRIVATE
  "${Sandbox_SOURCES}"
)

find_program(SLANGC_EXECUTABLE NAMES slangc REQUIRED)

set(Sandbox_SHADER_SOURCE "${CMAKE_CURRENT_LIST_DIR}/assets/shaders/BasicShader.slang")
set(Sandbox_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders")
set(Sandbox_VERT_SPV "${Sandbox_SHADER_OUTPUT_DIR}/BasicShader.vert.spv")
set(Sandbox_SHADOW_VERT_SPV "${Sandbox_SHADER_OUTPUT_DIR}/BasicShader.shadow.vert.spv")
set(Sandbox_FRAG_SPV "${Sandbox_SHADER_OUTPUT_DIR}/BasicShader.frag.spv")
set(Sandbox_DZN_FALLBACK_FRAG_SPV "${Sandbox_SHADER_OUTPUT_DIR}/BasicShader.dzn_fallback.frag.spv")
set(Sandbox_SHADOW_FRAG_SPV "${Sandbox_SHADER_OUTPUT_DIR}/BasicShader.shadow.frag.spv")
set(Sandbox_RUNTIME_SHADER_DIR "shaders")
set(Sandbox_SLANG_COMMON_ARGS
  -target spirv
  -profile sm_6_5
)

# Some Linux Vulkan validation-layer stacks still reject Slang's direct SPIR-V
# source-language enum. Routing through GLSL keeps the emitted SPIR-V widely compatible.
if(UNIX AND NOT APPLE)
  list(APPEND Sandbox_SLANG_COMMON_ARGS -emit-spirv-via-glsl)
endif()

add_custom_command(
  OUTPUT "${Sandbox_VERT_SPV}" "${Sandbox_SHADOW_VERT_SPV}" "${Sandbox_FRAG_SPV}" "${Sandbox_DZN_FALLBACK_FRAG_SPV}" "${Sandbox_SHADOW_FRAG_SPV}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${Sandbox_SHADER_OUTPUT_DIR}"
  COMMAND ${SLANGC_EXECUTABLE} "${Sandbox_SHADER_SOURCE}" ${Sandbox_SLANG_COMMON_ARGS} -stage vertex -entry vertMain -o "${Sandbox_VERT_SPV}"
  COMMAND ${SLANGC_EXECUTABLE} "${Sandbox_SHADER_SOURCE}" ${Sandbox_SLANG_COMMON_ARGS} -stage vertex -entry vertShadowMain -o "${Sandbox_SHADOW_VERT_SPV}"
  COMMAND ${SLANGC_EXECUTABLE} "${Sandbox_SHADER_SOURCE}" ${Sandbox_SLANG_COMMON_ARGS} -stage fragment -entry fragMain -o "${Sandbox_FRAG_SPV}"
  COMMAND ${SLANGC_EXECUTABLE} "${Sandbox_SHADER_SOURCE}" ${Sandbox_SLANG_COMMON_ARGS} -stage fragment -entry fragMainDznFallback -o "${Sandbox_DZN_FALLBACK_FRAG_SPV}"
  COMMAND ${SLANGC_EXECUTABLE} "${Sandbox_SHADER_SOURCE}" ${Sandbox_SLANG_COMMON_ARGS} -stage fragment -entry fragShadowMap -o "${Sandbox_SHADOW_FRAG_SPV}"
  DEPENDS "${Sandbox_SHADER_SOURCE}"
  VERBATIM
)

add_custom_target(SandboxShaders DEPENDS "${Sandbox_VERT_SPV}" "${Sandbox_SHADOW_VERT_SPV}" "${Sandbox_FRAG_SPV}" "${Sandbox_DZN_FALLBACK_FRAG_SPV}" "${Sandbox_SHADOW_FRAG_SPV}")
add_dependencies(Sandbox SandboxShaders)

file(GLOB_RECURSE Sandbox_ASSET_FILES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_LIST_DIR}/assets/*"
)
set(Sandbox_ASSET_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets")
set(Sandbox_ASSET_STAMP "${CMAKE_CURRENT_BINARY_DIR}/generated/assets/.stamp")
add_custom_command(
  OUTPUT "${Sandbox_ASSET_STAMP}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${Sandbox_ASSET_OUTPUT_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/assets" "${Sandbox_ASSET_OUTPUT_DIR}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated/assets"
  COMMAND ${CMAKE_COMMAND} -E touch "${Sandbox_ASSET_STAMP}"
  DEPENDS ${Sandbox_ASSET_FILES}
  VERBATIM
)
add_custom_target(SandboxAssets DEPENDS "${Sandbox_ASSET_STAMP}")
add_dependencies(Sandbox SandboxAssets)

target_compile_definitions(Sandbox
  PRIVATE
    GLM_ENABLE_EXPERIMENTAL
    $<$<BOOL:${Sandbox_ENABLE_FULL_EDITOR}>:Sandbox_ENABLE_FULL_EDITOR=1>
    HVK_VERT_SHADER_PATH="${Sandbox_RUNTIME_SHADER_DIR}/BasicShader.vert.spv"
    HVK_SHADOW_VERT_SHADER_PATH="${Sandbox_RUNTIME_SHADER_DIR}/BasicShader.shadow.vert.spv"
    HVK_FRAG_SHADER_PATH="${Sandbox_RUNTIME_SHADER_DIR}/BasicShader.frag.spv"
    HVK_DZN_FALLBACK_FRAG_SHADER_PATH="${Sandbox_RUNTIME_SHADER_DIR}/BasicShader.dzn_fallback.frag.spv"
    HVK_SHADOW_FRAG_SHADER_PATH="${Sandbox_RUNTIME_SHADER_DIR}/BasicShader.shadow.frag.spv"
)

target_include_directories(Sandbox
  SYSTEM PRIVATE
    "${CMAKE_SOURCE_DIR}/Halibut/vendor"
)

target_link_libraries(Sandbox
  PRIVATE
    Halibut
)

add_custom_command(
  TARGET Sandbox POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Sandbox>/${Sandbox_RUNTIME_SHADER_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${Sandbox_VERT_SPV}" "$<TARGET_FILE_DIR:Sandbox>/${Sandbox_RUNTIME_SHADER_DIR}/BasicShader.vert.spv"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${Sandbox_SHADOW_VERT_SPV}" "$<TARGET_FILE_DIR:Sandbox>/${Sandbox_RUNTIME_SHADER_DIR}/BasicShader.shadow.vert.spv"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${Sandbox_FRAG_SPV}" "$<TARGET_FILE_DIR:Sandbox>/${Sandbox_RUNTIME_SHADER_DIR}/BasicShader.frag.spv"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${Sandbox_DZN_FALLBACK_FRAG_SPV}" "$<TARGET_FILE_DIR:Sandbox>/${Sandbox_RUNTIME_SHADER_DIR}/BasicShader.dzn_fallback.frag.spv"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${Sandbox_SHADOW_FRAG_SPV}" "$<TARGET_FILE_DIR:Sandbox>/${Sandbox_RUNTIME_SHADER_DIR}/BasicShader.shadow.frag.spv"
  VERBATIM
)

if(MINGW)
  get_filename_component(Sandbox_MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
  set(Sandbox_MINGW_RUNTIME_DLL_CANDIDATES
    "libstdc++-6.dll"
    "libgcc_s_seh-1.dll"
    "libgcc_s_dw2-1.dll"
    "libwinpthread-1.dll"
  )
  foreach(Sandbox_RUNTIME_DLL ${Sandbox_MINGW_RUNTIME_DLL_CANDIDATES})
    if(EXISTS "${Sandbox_MINGW_BIN_DIR}/${Sandbox_RUNTIME_DLL}")
      add_custom_command(
        TARGET Sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${Sandbox_MINGW_BIN_DIR}/${Sandbox_RUNTIME_DLL}"
                "$<TARGET_FILE_DIR:Sandbox>/${Sandbox_RUNTIME_DLL}"
      )
      install(FILES "${Sandbox_MINGW_BIN_DIR}/${Sandbox_RUNTIME_DLL}" DESTINATION .)
    endif()
  endforeach()
endif()

install(TARGETS Sandbox RUNTIME DESTINATION .)
install(FILES
  "${Sandbox_VERT_SPV}"
  "${Sandbox_SHADOW_VERT_SPV}"
  "${Sandbox_FRAG_SPV}"
  "${Sandbox_DZN_FALLBACK_FRAG_SPV}"
  "${Sandbox_SHADOW_FRAG_SPV}"
  DESTINATION "${Sandbox_RUNTIME_SHADER_DIR}"
)
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/assets/" DESTINATION assets)
