add_executable(html_colour_to_ansi main.c ../../libclink/src/colour.c)
target_include_directories(html_colour_to_ansi PRIVATE ../../libclink/src)
