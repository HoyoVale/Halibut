add_executable(Sandbox
  "${CMAKE_CURRENT_LIST_DIR}/src/EditorLayer.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/src/HoyoEditor.cpp"
)

find_program(SLANGC_EXECUTABLE NAMES slangc REQUIRED)

target_compile_definitions(Sandbox
  PRIVATE
)

target_include_directories(Sandbox
  SYSTEM PRIVATE
    "${CMAKE_SOURCE_DIR}/Halibut/vendor"
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/src/"
)

target_link_libraries(Sandbox
  PRIVATE
    Halibut
)




