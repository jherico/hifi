set(TARGET_NAME browser)

find_package(Qt5 COMPONENTS WebEngineWidgets)

# grab the implementation and header files from src dirs
file(GLOB_RECURSE BROWSER_SRCS "src/*.cpp" "src/*.h")
file (GLOB_RECURSE QT_UI_FILES src/*.ui)
source_group("UI Files" FILES ${QT_UI_FILES})
qt5_wrap_ui(QT_UI_HEADERS "${QT_UI_FILES}")

set(BROWSER_SRCS ${BROWSER_SRCS} "${QT_UI_HEADERS}")


add_executable(${TARGET_NAME} ${BROWSER_SRCS})

target_link_libraries(${TARGET_NAME} Qt5::WebEngineWidgets)

set(EXTRA_DEPLOY_OPTIONS "--qmldir  \"${CMAKE_CURRENT_SOURCE_DIR}/src\"")
package_libraries_for_deployment()
