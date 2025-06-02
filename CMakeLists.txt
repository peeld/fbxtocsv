cmake_minimum_required(VERSION 3.5)

# D:\MOCAP\Astro_Project\LYRA\2024-03-06-LyraDay1\FBX\Agree_001.fbx  d:\foo.csv

project(FbxToCsv)

 IF(WIN32)
        SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W4 ${FBX_CC_RTLd}")
        SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /W4 ${FBX_CC_RTLd}")
        SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${FBX_CC_RTL}")
        SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${FBX_CC_RTL}")
        SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${FBX_CC_RTL}")
        SET(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${FBX_CC_RTL}")
endif()

ADD_DEFINITIONS(-DFBXSDK_SHARED)

IF (WIN32)
   set(FBXSDK "C:/Program Files/Autodesk/FBX/FBX SDK/2020.3.4")
   set(FBXLIBS 
     debug ${FBXSDK}/lib/vs2022/x64/debug/libxml2-md.lib
     optimized ${FBXSDK}/lib/vs2022/x64/release/libxml2-md.lib
     debug ${FBXSDK}/lib/vs2022/x64/debug/zlib-md.lib
     optimized ${FBXSDK}/lib/vs2022/x64/release/zlib-md.lib
     debug ${FBXSDK}/lib/vs2022/x64/debug/libfbxsdk.lib
     optimized ${FBXSDK}/lib/vs2022/x64/release/libfbxsdk.lib)
ELSE()
   set(FBXSDK {CMAKE_CURRENT_LIST_DIR}/sdk)
   set(FBXLIBS  xml2 z dl pthread
    debug ${FBXSDK}/lib/gcc/x64/debug/libfbxsdk.a
    optimized ${FBXSDK}/lib/gcc/x64/release/libfbxsdk.a)
ENDIF()

add_executable(FbxToCsv main.cpp)
target_link_libraries(FbxToCsv ${FBXLIBS})

include_directories(${FBXSDK}/include)


# Add post-build commands for copying DLLs
set(FBXSDK_DLL_DEBUG "${FBXSDK}/lib/vs2022/x64/debug/libfbxsdk.dll")
set(FBXSDK_DLL_RELEASE "${FBXSDK}/lib/vs2022/x64/release/libfbxsdk.dll")

add_custom_command(TARGET FbxToCsv POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<IF:$<CONFIG:Debug>,${FBXSDK_DLL_DEBUG},${FBXSDK_DLL_RELEASE}>"
        "$<TARGET_FILE_DIR:FbxToCsv>/libfbxsdk.dll"
    COMMENT "Copying libfbxsdk.dll based on configuration"
)