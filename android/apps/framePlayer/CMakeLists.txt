set(TARGET_NAME framePlayer)
setup_hifi_library(AndroidExtras)
link_hifi_libraries(shared ktx shaders gpu gl ${PLATFORM_GL_BACKEND})
target_link_libraries(${TARGET_NAME} android log m)
target_opengl()

option(OCULUS_MOBILE OFF)

if(OCULUS_MOBILE)
    target_oculus_mobile()
    link_hifi_libraries(oculusMobile)
    target_compile_definitions(${TARGET_NAME} PRIVATE OCULUS_MOBILE=1)
else()
    target_compile_definitions(${TARGET_NAME} PRIVATE OCULUS_MOBILE=0)
endif()




