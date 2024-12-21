file(GLOB net_sources CONFIGURE_DEPENDS "*.c")
add_library(net STATIC ${net_sources})
