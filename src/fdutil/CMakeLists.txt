file(GLOB fdutil_sources CONFIGURE_DEPENDS "*.c")

add_library(fdutil STATIC ${fdutil_sources})
