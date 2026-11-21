vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO 3dgep/Audio
    REF "v${VERSION}"
    SHA512 56051f948ed4d2fbe806709cfa252d31604b0c2ee43c9c4d07fa8fb4814ac9b1c0aec349c0d25536ef47c555611cd37e17111f543bdcb20d98d30cb58ea46a06
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
