# I don't want to mix all test cases into kernel.
# this will make kernel.elf too large and is problematic.

# test utilities(add new ones here)
# these are NOT specific to any test cases
set(test_util_src
    rand.c
    range.c
)
add_library(test_util STATIC ${test_util_src})
# make test a fake lib which does not run any test in reality.
add_library(test STATIC "${test_util_src};void.c")

# test cases
# Hint: (use semicolon to separate; do NOT add whitespaces)
set(tcases "debug;bitmap;lst" CACHE STRING "")

# add all of your test cases here.
# and build a library for it.
set(test_src "")

foreach(tcase IN LISTS tcases)
    message(STATUS "Collecting test case: ${tcase}")
    set(test_src "${test_src} ${tcase}_test.c")
    add_library("${tcase}_test" STATIC "${tcase}_test.c")
endforeach()

# for debugging
message(STATUS "Collected test sources: ${test_src}.")

#add_library(debug_test STATIC debug_test.c)
#add_library(bitmap_test STATIC bitmap_test.c)

file(GLOB test_sources CONFIGURE_DEPENDS "*.c")

#add_library(test STATIC ${test_sources})
