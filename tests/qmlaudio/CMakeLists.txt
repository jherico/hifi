
set(TARGET_NAME qmlaudio-test)
 
if (WIN32)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4049 /ignore:4217")
endif()

# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui OpenGL Multimedia Qml WebEngine WebEngineWidgets)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

# link in the shared libraries
link_hifi_libraries(shared )

set(EXTRA_DEPLOY_OPTIONS "--qmldir  ${PROJECT_SOURCE_DIR}/src")

package_libraries_for_deployment()

