# Exact byte references from reviewed, source-controlled assets. Runtime VFS
# overrides must match these; stb_truetype is not safe for arbitrary fonts.
set(_font_header "${CMAKE_CURRENT_BINARY_DIR}/trusted_mui_fonts.h")
file(WRITE "${_font_header}" "// Generated; do not commit.\n")
foreach(_font IN ITEMS FalloutClassicDialog PTM55FT)
    set(_path "${CMAKE_CURRENT_SOURCE_DIR}/files/ce.dat/fonts/${_font}.ttf")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_path}")
    file(READ "${_path}" _bytes HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _bytes "${_bytes}")
    file(APPEND "${_font_header}" "static const unsigned char k${_font}[] = {${_bytes}};\n")
endforeach()
