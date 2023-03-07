

# Build GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL  "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL  "GLFW lib only")
add_subdirectory(glfw)

# GLM
add_subdirectory(glm)

# STB
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")

# tinyobjloader
add_subdirectory(tinyobjloader)

# VMA
add_subdirectory(VulkanMemoryAllocator)

# Imgui
include_directories(imgui)

include_directories(ImGuizmo)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
set(IMGUIZMO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo")

set(IMGUI_FILES
    ${IMGUI_DIR}/imgui.h
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUIZMO_DIR}/ImGuizmo.h
    ${IMGUIZMO_DIR}/ImGuizmo.cpp
)

add_library(imgui STATIC ${IMGUI_FILES})

target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUIZMO_DIR})


# Ktx
set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ktx)

set(KTX_SOURCES  
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/dfdutils/createdfd.c
    ${KTX_DIR}/lib/dfdutils/colourspaces.c
    ${KTX_DIR}/lib/dfdutils/dfd.h
    ${KTX_DIR}/lib/dfdutils/dfd2vk.inl
    ${KTX_DIR}/lib/dfdutils/interpretdfd.c
    ${KTX_DIR}/lib/dfdutils/printdfd.c
    ${KTX_DIR}/lib/dfdutils/queries.c
    ${KTX_DIR}/lib/dfdutils/vk2dfd.c
    ${KTX_DIR}/lib/etcdec.cxx
    ${KTX_DIR}/lib/etcunpack.cxx
    ${KTX_DIR}/lib/filestream.c
    ${KTX_DIR}/lib/filestream.h
    ${KTX_DIR}/lib/formatsize.h
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/info.c
    ${KTX_DIR}/lib/ktxint.h
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/memstream.h
    ${KTX_DIR}/lib/strings.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/uthash.h
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/texture.h

    # Basis Universal
    ${KTX_DIR}/lib/basis_sgd.h
    ${KTX_DIR}/lib/basis_transcode.cpp
    ${KTX_DIR}/lib/basisu/transcoder/basisu_containers.h
    ${KTX_DIR}/lib/basisu/transcoder/basisu_containers_impl.h
    ${KTX_DIR}/lib/basisu/transcoder/basisu_file_headers.h
    ${KTX_DIR}/lib/basisu/transcoder/basisu_transcoder_internal.h
    ${KTX_DIR}/lib/basisu/transcoder/basisu_transcoder_uastc.h
    ${KTX_DIR}/lib/basisu/transcoder/basisu_transcoder.cpp
    ${KTX_DIR}/lib/basisu/transcoder/basisu_transcoder.h
    ${KTX_DIR}/lib/basisu/transcoder/basisu.h
    ${KTX_DIR}/lib/basisu/zstd/zstd.c
   
    # KT1
    ${KTX_DIR}/lib/texture1.c
    ${KTX_DIR}/lib/texture1.h

    # KTX2
    ${KTX_DIR}/lib/texture2.c
    ${KTX_DIR}/lib/texture2.h

    # Vulkan support
    ${KTX_DIR}/lib/vk_format.h
    ${KTX_DIR}/lib/vkformat_check.c
    ${KTX_DIR}/lib/vkformat_enum.h
    ${KTX_DIR}/lib/vkformat_str.c
    ${KTX_DIR}/lib/vk_funcs.c
    ${KTX_DIR}/lib/vk_funcs.h
    ${KTX_DIR}/lib/vkloader.c    
)

set(KTX_INCLUDE_DIRS
    ${KTX_DIR}/include
    ${KTX_DIR}/lib
    ${KTX_DIR}/utils
    ${KTX_DIR}/lib/basisu/zstd
    ${KTX_DIR}/lib/basisu/transcoder
    ${KTX_DIR}/other_include
)

add_library(ktx ${KTX_SOURCES})
target_include_directories(ktx PUBLIC ${KTX_INCLUDE_DIRS})

target_compile_definitions(ktx PUBLIC LIBKTX)
if (WIN32)
#target_compile_definitions(ktx PUBLIC "KTX_API=__declspec(dllexport)")
add_compile_definitions(KHRONOS_STATIC) 
endif()
target_compile_definitions(ktx PUBLIC KTX_FEATURE_WRITE=0)
target_compile_definitions(ktx PUBLIC BASISD_SUPPORT_KTX2_ZSTD=0)
target_compile_definitions(ktx PUBLIC BASISU_NO_ITERATOR_DEBUG_LEVEL)
target_link_libraries(ktx PUBLIC Vulkan::Vulkan)