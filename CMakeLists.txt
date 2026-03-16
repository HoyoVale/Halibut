cmake_minimum_required(VERSION 3.20)

project(HalibutEngine LANGUAGES CXX)

add_library(HalibutEngine_cxx_std INTERFACE)
add_library(HalibutEngine::cxx_std ALIAS HalibutEngine_cxx_std)
target_compile_features(HalibutEngine_cxx_std INTERFACE cxx_std_20)

add_subdirectory(Halibut)
add_subdirectory(Sandbox)
