# The Skailar SDK ships only a static library through vcpkg.
vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO getskailar/sdk-cpp
    REF "v${VERSION}"
    SHA512 34f904b425c8d7e050bec610b5b4bee7234b0cc240dade5a222565bb64e5231e2f2ebab3ec8ce44cad0508d15434be63bef28027c3230a6c380ddcd9e27dd5cc
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DSKAILAR_BUILD_TESTS=OFF
        -DSKAILAR_BUILD_EXAMPLES=OFF
        -DSKAILAR_WERROR=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME skailar CONFIG_PATH lib/cmake/skailar)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
