set(TARGET_NAME browser)
setup_hifi_project(Quick Gui WebEngine)

set(EXTRA_DEPLOY_OPTIONS "--qmldir  \"${CMAKE_CURRENT_SOURCE_DIR}/src\"")
package_libraries_for_deployment()
