# @Fudanyrd
# Please Follow the instruction in README.md to add a new test.

# I don't want to mix all test cases into kernel.
# this will make kernel.elf too large and is problematic.

# test utilities(add new ones here)
# these are NOT specific to any test cases
set(test_util_src
    rand.c
    range.c
    sync.c
    proc_test.c
)
add_library(test_util STATIC ${test_util_src})
# make test a fake lib which does not run any test in reality.
add_library(test STATIC "${test_util_src}")

# test cases
# Hint: (use semicolon to separate; do NOT add whitespaces)
# Lab 0: Boot
set(lab0cases "debug;bitmap;lst")
# Lab 1: malloc
set(lab1cases "palloc;malloc;alloc2023;lab1;little;steal")
# Lab 2: kernel proc
set(lab2cases "alloc2023;pcreat;pwait;pwtmany;prpr;prpr2;prpr3;prpr4;pstree;pstree2;trap")

# enabled test cases
# (warning: too many test cases result in too many images)
# TO ENABLE ALL TESTS: 
# set(tcases "${lab0cases};${lab1cases};...;${labncases}" CACHE STRING "")
set(tcases "${lab2cases}" CACHE STRING "")

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
