# Add CPM Cmake Package Manager
include(CPM.cmake)

include("${CMAKE_CURRENT_LIST_DIR}/version.cmake")

# Add Vulkan
find_package(Vulkan REQUIRED)

# GLFW
CPMAddPackage(
    NAME glfw
    GITHUB_REPOSITORY ${glfw_GITHUB}
    GIT_TAG ${glfw_TAG}
    OPTIONS
    "GLFW_INSTALL OFF"
    "GLFW_BUILD_DOCS OFF"
    "GLFW_BUILD_TESTS OFF"
    "GLFW_BUILD_EXAMPLES OFF"
)

# GLM
CPMAddPackage(
    NAME glm
    GITHUB_REPOSITORY ${glm_GITHUB}
    GIT_TAG ${glm_TAG}
)
set(GLM_INCLUDE_DIR ${glm_SOURCE_DIR} PARENT_SCOPE)

# STB
CPMAddPackage(
    NAME stb
    GITHUB_REPOSITORY ${stb_GITHUB}
    GIT_TAG ${stb_TAG}
    DOWNLOAD_ONLY YES
)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# tinyobjloader
CPMAddPackage(
    NAME tinyobjloader
    GITHUB_REPOSITORY ${tinyobjloader_GITHUB}
    GIT_TAG ${tinyobjloader_TAG}
)

# VMA
CPMAddPackage(
    NAME vma
    GITHUB_REPOSITORY ${vma_GITHUB}
    GIT_TAG ${vma_TAG}
)

# Imgui
CPMAddPackage(
    NAME imgui
    GITHUB_REPOSITORY ${imgui_GITHUB}
    GIT_TAG ${imgui_TAG}
    DOWNLOAD_ONLY YES
)

CPMAddPackage(
    NAME imguizmo
    GITHUB_REPOSITORY ${imguizmo_GITHUB}
    GIT_TAG ${imguizmo_TAG}
    DOWNLOAD_ONLY YES
)

set(IMGUI_FILES
    ${imgui_SOURCE_DIR}/imgui.h
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imguizmo_SOURCE_DIR}/ImGuizmo.h
    ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
)

add_library(imgui STATIC ${IMGUI_FILES})

target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imguizmo_SOURCE_DIR})
target_link_libraries(imgui PUBLIC Vulkan::Vulkan glfw)

# Ktx
CPMAddPackage(
    NAME ktx
    GITHUB_REPOSITORY ${ktx_GITHUB}
    GIT_TAG ${ktx_TAG}
    DOWNLOAD_ONLY YES
)

set(KTX_SOURCES  
    ${ktx_SOURCE_DIR}/lib/checkheader.c
    ${ktx_SOURCE_DIR}/lib/dfdutils/createdfd.c
    ${ktx_SOURCE_DIR}/lib/dfdutils/colourspaces.c
    ${ktx_SOURCE_DIR}/lib/dfdutils/dfd.h
    ${ktx_SOURCE_DIR}/lib/dfdutils/dfd2vk.inl
    ${ktx_SOURCE_DIR}/lib/dfdutils/interpretdfd.c
    ${ktx_SOURCE_DIR}/lib/dfdutils/printdfd.c
    ${ktx_SOURCE_DIR}/lib/dfdutils/queries.c
    ${ktx_SOURCE_DIR}/lib/dfdutils/vk2dfd.c
    ${ktx_SOURCE_DIR}/lib/etcdec.cxx
    ${ktx_SOURCE_DIR}/lib/etcunpack.cxx
    ${ktx_SOURCE_DIR}/lib/filestream.c
    ${ktx_SOURCE_DIR}/lib/filestream.h
    ${ktx_SOURCE_DIR}/lib/formatsize.h
    ${ktx_SOURCE_DIR}/lib/hashlist.c
    ${ktx_SOURCE_DIR}/lib/info.c
    ${ktx_SOURCE_DIR}/lib/ktxint.h
    ${ktx_SOURCE_DIR}/lib/memstream.c
    ${ktx_SOURCE_DIR}/lib/memstream.h
    ${ktx_SOURCE_DIR}/lib/strings.c
    ${ktx_SOURCE_DIR}/lib/swap.c
    ${ktx_SOURCE_DIR}/lib/uthash.h
    ${ktx_SOURCE_DIR}/lib/texture.c
    ${ktx_SOURCE_DIR}/lib/texture.h

    # Basis Universal
    ${ktx_SOURCE_DIR}/lib/basis_sgd.h
    ${ktx_SOURCE_DIR}/lib/basis_transcode.cpp
    ${ktx_SOURCE_DIR}/lib/basisu/transcoder/basisu_containers.h
    ${ktx_SOURCE_DIR}/lib/basisu/transcoder/basisu_containers_impl.h
    ${ktx_SOURCE_DIR}/lib/basisu/transcoder/basisu_file_headers.h
    ${ktx_SOURCE_DIR}/lib/basisu/transcoder/basisu_transcoder_internal.h
    ${ktx_SOURCE_DIR}/lib/basisu/transcoder/basisu_transcoder_uastc.h
    ${ktx_SOURCE_DIR}/lib/basisu/transcoder/basisu_transcoder.cpp
    ${ktx_SOURCE_DIR}/lib/basisu/transcoder/basisu_transcoder.h
    ${ktx_SOURCE_DIR}/lib/basisu/transcoder/basisu.h
    ${ktx_SOURCE_DIR}/lib/basisu/zstd/zstd.c
   
    # KT1
    ${ktx_SOURCE_DIR}/lib/texture1.c
    ${ktx_SOURCE_DIR}/lib/texture1.h

    # KTX2
    ${ktx_SOURCE_DIR}/lib/texture2.c
    ${ktx_SOURCE_DIR}/lib/texture2.h

    # Vulkan support
    ${ktx_SOURCE_DIR}/lib/vk_format.h
    ${ktx_SOURCE_DIR}/lib/vkformat_check.c
    ${ktx_SOURCE_DIR}/lib/vkformat_enum.h
    ${ktx_SOURCE_DIR}/lib/vkformat_str.c
    ${ktx_SOURCE_DIR}/lib/vk_funcs.c
    ${ktx_SOURCE_DIR}/lib/vk_funcs.h
    ${ktx_SOURCE_DIR}/lib/vkloader.c    

    # Zlib Support
    ${ktx_SOURCE_DIR}/lib/miniz_wrapper.cpp
)

set(KTX_INCLUDE_DIRS
    ${ktx_SOURCE_DIR}/include
    ${ktx_SOURCE_DIR}/lib
    ${ktx_SOURCE_DIR}/utils
    ${ktx_SOURCE_DIR}/lib/basisu/zstd
    ${ktx_SOURCE_DIR}/lib/basisu/transcoder
    ${ktx_SOURCE_DIR}/other_include
)

add_library(ktx ${KTX_SOURCES})
target_include_directories(ktx PUBLIC ${KTX_INCLUDE_DIRS})

target_compile_definitions(ktx PUBLIC KHRONOS_STATIC) 
target_compile_definitions(ktx PUBLIC LIBKTX)
target_compile_definitions(ktx PUBLIC KTX_FEATURE_WRITE=0)
target_compile_definitions(ktx PUBLIC BASISD_SUPPORT_KTX2_ZSTD=0)
target_compile_definitions(ktx PUBLIC BASISU_NO_ITERATOR_DEBUG_LEVEL)
target_link_libraries(ktx PUBLIC Vulkan::Vulkan)

# EnTT
CPMAddPackage(
    NAME entt
    GITHUB_REPOSITORY ${entt_GITHUB}
    GIT_TAG ${entt_TAG}
)

# JoltPhysics
CPMAddPackage(
    NAME jolt
    GITHUB_REPOSITORY ${jolt_GITHUB}
    GIT_TAG ${jolt_TAG}
    DOWNLOAD_ONLY YES
)
add_subdirectory(${jolt_SOURCE_DIR}/Build ${jolt_BINARY_DIR})

# Yojimbo
CPMAddPackage(
    NAME yojimbo
    GITHUB_REPOSITORY ${yojimbo_GITHUB}
    GIT_TAG ${yojimbo_TAG}
    DOWNLOAD_ONLY YES
)
CPMAddPackage(
    NAME libsodium
    GITHUB_REPOSITORY ${libsodium_GITHUB}
    GIT_TAG ${libsodium_TAG}
)
CPMAddPackage(
    NAME mbedtls
    GITHUB_REPOSITORY ${mbedtls_GITHUB}
    GIT_TAG ${mbedtls_TAG}
)

add_library(yojimbo
        ${yojimbo_SOURCE_DIR}/yojimbo.cpp 
        ${yojimbo_SOURCE_DIR}/yojimbo.h
        ${yojimbo_SOURCE_DIR}/netcode.io/netcode.c 
        ${yojimbo_SOURCE_DIR}/netcode.io/netcode.h
        ${yojimbo_SOURCE_DIR}/reliable.io/reliable.c 
        ${yojimbo_SOURCE_DIR}/reliable.io/reliable.h
        ${yojimbo_SOURCE_DIR}/certs.c 
        ${yojimbo_SOURCE_DIR}/certs.h
        ${yojimbo_SOURCE_DIR}/tlsf/tlsf.c 
        ${yojimbo_SOURCE_DIR}/tlsf/tlsf.c)
target_link_libraries(yojimbo
    PRIVATE MbedTLS::mbedtls
           MbedTLS::mbedcrypto
           MbedTLS::mbedx509 
           sodium)
target_compile_definitions(yojimbo PUBLIC 
        NETCODE_ENABLE_TESTS=0 
        RELIABLE_ENABLE_TESTS=0)    
target_include_directories(yojimbo PUBLIC 
    ${yojimbo_SOURCE_DIR}/netcode.io 
    ${yojimbo_SOURCE_DIR}/reliable.io 
    ${yojimbo_SOURCE_DIR})

set(YOJIMBO_LIBRARY yojimbo CACHE STRING "" FORCE)
mark_as_advanced(YOJIMBO_LIBRARY)