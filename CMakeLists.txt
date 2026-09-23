cmake_minimum_required(VERSION 3.21)

project(DialogueMovementEnabler VERSION 1.0.0 LANGUAGES CXX)

set(PLUGIN_AUTHOR "RoseEden" CACHE STRING "Author name in the plugin declaration")
set(MOD_FOLDER_NAME "Dialogue Movement Enabler" CACHE STRING "Folder name under STARFIELD_MODS_FOLDER")

# Optional deploy target: a game install or a mod manager's mods folder.
if(DEFINED ENV{STARFIELD_FOLDER} AND IS_DIRECTORY "$ENV{STARFIELD_FOLDER}/Data")
    set(OUTPUT_FOLDER "$ENV{STARFIELD_FOLDER}/Data")
endif()

if(DEFINED ENV{STARFIELD_MODS_FOLDER} AND IS_DIRECTORY "$ENV{STARFIELD_MODS_FOLDER}")
    set(OUTPUT_FOLDER "$ENV{STARFIELD_MODS_FOLDER}/${MOD_FOLDER_NAME}")
endif()

# CommonLibSF only ships an xmake build, so its sources are compiled here.
set(
    COMMONLIBSF_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibSF"
    CACHE PATH
    "Path to CommonLibSF"
)

add_subdirectory(
    "${COMMONLIBSF_DIR}/lib/commonlib-shared"
    "${CMAKE_CURRENT_BINARY_DIR}/commonlib-shared"
)

file(GLOB_RECURSE COMMONLIBSF_SOURCES CONFIGURE_DEPENDS "${COMMONLIBSF_DIR}/src/*.cpp")

add_library(commonlibsf STATIC ${COMMONLIBSF_SOURCES})
target_include_directories(commonlibsf PUBLIC "${COMMONLIBSF_DIR}/include")
target_link_libraries(commonlibsf PUBLIC commonlib-shared::commonlib-shared)
target_compile_options(commonlibsf PUBLIC /bigobj)
target_precompile_headers(commonlibsf PRIVATE "${COMMONLIBSF_DIR}/include/SFSE/Impl/PCH.h")

find_path(SIMPLEINI_INCLUDE_DIRS "SimpleIni.h" REQUIRED)

add_library(${PROJECT_NAME} SHARED
    src/plugin.cpp
    src/AutoClose.cpp
    src/Bindings.cpp
    src/Dialogue.cpp
    src/EdgeLook.cpp
    src/Remap.cpp
    src/Settings.cpp
)

configure_file(src/Version.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h" @ONLY)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    ${SIMPLEINI_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE commonlibsf)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)

# Symbols in every configuration for readable crash logs; ICF would mislabel folded functions.
target_compile_options(${PROJECT_NAME} PRIVATE /Z7)
target_link_options(${PROJECT_NAME} PRIVATE /DEBUG /OPT:REF /OPT:NOICF)

set(PACKAGE_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/package/SFSE/Plugins")

add_custom_command(
    TARGET "${PROJECT_NAME}"
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${PACKAGE_FOLDER}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "$<TARGET_PDB_FILE:${PROJECT_NAME}>"
        "${CMAKE_CURRENT_SOURCE_DIR}/package/${PROJECT_NAME}.ini"
        "${PACKAGE_FOLDER}"
    VERBATIM
)

if(DEFINED OUTPUT_FOLDER)
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SFSE/Plugins")

    message(STATUS "SFSE plugin output folder: ${DLL_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "$<TARGET_PDB_FILE:${PROJECT_NAME}>"
            "${DLL_FOLDER}"
        VERBATIM
    )

    # Never overwrite the user's .ini.
    if(NOT EXISTS "${DLL_FOLDER}/${PROJECT_NAME}.ini")
        add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy
                "${CMAKE_CURRENT_SOURCE_DIR}/package/${PROJECT_NAME}.ini"
                "${DLL_FOLDER}/${PROJECT_NAME}.ini"
            VERBATIM
        )
    endif()
endif()
