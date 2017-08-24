
set(TARGET_NAME spacemouse)

if (WIN32)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4049 /ignore:4217")
endif()

setup_memory_debugger()

# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Gui)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")
set(SPACEMOUSE_SDK "C:/dev/3dxware")
target_include_directories(${TARGET_NAME} PRIVATE "${SPACEMOUSE_SDK}/Inc")
target_link_libraries(${TARGET_NAME} "${SPACEMOUSE_SDK}/Lib/x64/siapp.lib")

package_libraries_for_deployment()
