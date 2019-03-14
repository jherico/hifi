include(vcpkg_common_functions)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO ValveSoftware/openvr
    REF v1.2.10
    SHA512 f25a45d732f7cd4d49647e9a82bfcfe3722c412075237a996133540f82b69f30aaa745c58e602f74b896fcc624b536cd6d616b65ab30c331bf01506816ae9889
    HEAD_REF master
)

set(VCPKG_LIBRARY_LINKAGE dynamic)

if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(ARCH_PATH "win64")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    set(ARCH_PATH "win32")
else()
    message(FATAL_ERROR "Package only supports x64 and x86 windows.")
endif()

if(VCPKG_CMAKE_SYSTEM_NAME)
    message(FATAL_ERROR "Package only supports windows desktop.")
endif()

file(MAKE_DIRECTORY
    ${CURRENT_PACKAGES_DIR}/lib
    ${CURRENT_PACKAGES_DIR}/bin
    ${CURRENT_PACKAGES_DIR}/debug/lib
    ${CURRENT_PACKAGES_DIR}/debug/bin
)
file(COPY ${SOURCE_PATH}/lib/${ARCH_PATH}/openvr_api.lib DESTINATION ${CURRENT_PACKAGES_DIR}/lib)
file(COPY ${SOURCE_PATH}/lib/${ARCH_PATH}/openvr_api.lib DESTINATION ${CURRENT_PACKAGES_DIR}/debug/lib)
file(COPY
    ${SOURCE_PATH}/bin/${ARCH_PATH}/openvr_api.dll
    ${SOURCE_PATH}/bin/${ARCH_PATH}/openvr_api.pdb
    DESTINATION ${CURRENT_PACKAGES_DIR}/bin
)
file(COPY
    ${SOURCE_PATH}/bin/${ARCH_PATH}/openvr_api.dll
    ${SOURCE_PATH}/bin/${ARCH_PATH}/openvr_api.pdb
    DESTINATION ${CURRENT_PACKAGES_DIR}/debug/bin
)
file(COPY ${SOURCE_PATH}/headers DESTINATION ${CURRENT_PACKAGES_DIR})
file(RENAME ${CURRENT_PACKAGES_DIR}/headers ${CURRENT_PACKAGES_DIR}/include)

file(COPY ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/openvr)
file(RENAME ${CURRENT_PACKAGES_DIR}/share/openvr/LICENSE ${CURRENT_PACKAGES_DIR}/share/openvr/copyright)
