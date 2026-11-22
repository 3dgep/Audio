vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO 3dgep/Audio
    REF "v${VERSION}"
    SHA512 e19dec536a09256ad933943f49e00aad70dc98f74d5af6087ab35ba0442e2055b0b6fda41a253de2e4bf8cdfaeecf3e9b76e27817aae24509725af52fad5763a
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DAUDIO_BUILD_EXAMPLES=OFF
        -DAUDIO_INSTALL_CONFIG_SUBDIRS=OFF
        # Use vcpkg's miniaudio and stb ports instead of the copies in the repository.
        -DAUDIO_USE_SYSTEM_DEPS=ON
        # Use the C runtime selected by the triplet.
        -DAUDIO_MSVC_ANY_CRT=OFF
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME Audio
    CONFIG_PATH lib/cmake/Audio
)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
