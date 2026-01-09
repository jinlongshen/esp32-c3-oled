idf_component_register(
    SRCS "main.cpp"
         "Hooks.cpp"
    INCLUDE_DIRS "."
    REQUIRES fonts oled lvgl_driver
)

# 1. Force the linker to 'expect' the FreeType symbol immediately
# 2. Re-include the libraries at the end in a proper group
# target_link_options(${COMPONENT_LIB} PUBLIC 
#     "-Wl,-u,FT_Init_FreeType"
#     "-Wl,-u,FT_New_Face"
#     "-Wl,-u,FT_Done_Face"
# )