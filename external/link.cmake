target_link_libraries(engine PUBLIC SDL3::SDL3)
target_link_libraries(engine PUBLIC EnTT::EnTT)
target_link_libraries(engine PUBLIC glm::glm)
target_link_libraries(engine PUBLIC Vulkan::Vulkan)

target_link_libraries(engine PUBLIC vma)

target_link_libraries(engine PUBLIC imgui)
target_include_directories(engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
