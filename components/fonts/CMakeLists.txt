set(FREETYPE_LIB "/opt/freetype/lib/libfreetype.a")
set(FREETYPE_INCLUDE "/opt/freetype/include/freetype2")

idf_component_register(
    SRCS "src/FontRenderer.cpp"
         "src/font_rotate_task.cpp"
         "src/font_test_task.cpp"
    INCLUDE_DIRS "inc"
    EMBED_FILES "oled_subset_ascii_umlaut.ttf"
    PRIV_REQUIRES oled
)

target_link_libraries(${COMPONENT_LIB} PUBLIC "${FREETYPE_LIB}")
target_include_directories(${COMPONENT_LIB} PUBLIC "${FREETYPE_INCLUDE}")