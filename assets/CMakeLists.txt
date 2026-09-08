file(GLOB ASSET_FILES CONFIGURE_DEPENDS
        "${Engine_Root}/assets/shaders/*.hlsl"
        "${Engine_Root}/assets/shaders/*.hlsli"
        "${Engine_Root}/assets/textures/*.png"
        "${Engine_Root}/assets/scripts/*.nex"
)

# must be set before adding files to any target
set_source_files_properties(${ASSET_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

set(COPY_CMDS)
foreach (ASSET_FILE IN LISTS ASSET_FILES)
    file(RELATIVE_PATH REL "${Engine_Root}/assets" "${ASSET_FILE}")
    get_filename_component(REL_DIR "${REL}" DIRECTORY)
    list(APPEND COPY_CMDS
            COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_DIR}/$<CONFIG>/assets/${REL_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ASSET_FILE}" "${BIN_DIR}/$<CONFIG>/assets/${REL}"
    )
endforeach ()

add_custom_target(AssetFiles ALL
        ${COPY_CMDS}
        SOURCES ${ASSET_FILES}
        COMMENT "Syncing assets to bin/$<CONFIG>/assets"
)
