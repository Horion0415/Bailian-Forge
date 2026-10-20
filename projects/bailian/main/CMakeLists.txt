idf_component_register(
    SRCS "forge_device.c" "forge_screen_mirror.c" "game.c" "forge_money.c"
         "forge_data.c" "forge_i18n.c" "forge_model.c" "forge_input.c" "forge_view.c" "forge_effects.c" "forge_heating.c" "forge_frenzy.c" "forge_tribute.c" "forge_sword.c" "forge_swing.c"
    INCLUDE_DIRS "."
    REQUIRES mosaico_game_app mosaico_raylib_fast mosaico_game_2d mosaico_game_assets
             mosaico_game_audio esp-gsp nvs_flash esp_timer esp_hw_support)

set(FORGE_ASSETS "${CMAKE_CURRENT_LIST_DIR}/../assets/generated")
file(GLOB FORGE_ASSET_INPUTS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../assets_src/*" "${CMAKE_CURRENT_LIST_DIR}/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/*.h")
execute_process(COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/../assets_src/prepare_assets.py"
    RESULT_VARIABLE forge_assets_result)
if(NOT forge_assets_result EQUAL 0)
    message(FATAL_ERROR "Could not compile Bailian's image/font/audio assets")
endif()
add_custom_target(forge_assets
    COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/../assets_src/prepare_assets.py"
    DEPENDS ${FORGE_ASSET_INPUTS})
add_dependencies(${COMPONENT_LIB} forge_assets)
target_include_directories(${COMPONENT_LIB} PRIVATE "${FORGE_ASSETS}")
foreach(name room hammer swords relics)
    target_add_binary_data(${COMPONENT_LIB} "${FORGE_ASSETS}/${name}.atlas" BINARY
        RENAME_TO "bailian_asset_${name}" DEPENDS forge_assets)
endforeach()
target_add_binary_data(${COMPONENT_LIB} "${FORGE_ASSETS}/font.mask" BINARY
    RENAME_TO "bailian_asset_font" DEPENDS forge_assets)
foreach(name hit great damage break bell select heat frenzy)
    target_add_binary_data(${COMPONENT_LIB} "${FORGE_ASSETS}/${name}.sound" BINARY
        RENAME_TO "bailian_asset_${name}" DEPENDS forge_assets)
endforeach()

# GSP needs an image to bind the Raylib framebuffer. This generated placeholder
# is embedded with the authored image/font/audio resources in this application.
set(canvas_png "${CMAKE_CURRENT_BINARY_DIR}/canvas.png")
execute_process(COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/make_canvas_png.py"
    "${canvas_png}" RESULT_VARIABLE canvas_result)
if(NOT canvas_result EQUAL 0)
    message(FATAL_ERROR "Could not generate the game canvas placeholder")
endif()
set(CANVAS_IMAGE "${canvas_png}")
configure_file("scene.json.in" "${CMAKE_CURRENT_BINARY_DIR}/scene.json" @ONLY)
gsp_add_bundle(${COMPONENT_LIB}
    SCENES "${CMAKE_CURRENT_BINARY_DIR}/scene.json"
    PIXEL_FORMAT rgb565)
