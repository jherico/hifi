set(TARGET_NAME gl)
setup_hifi_library(Gui Widgets)
link_hifi_libraries(shared)
target_opengl()

if (ANDROID)
    #target_link_libraries(${TARGET_NAME} android log)
endif()

