# Unit test module
**@Fudanyrd**: please follow the following steps to add a unit test named `foo`:
<ol>
    <li>add the declaration in test_util.h;</li>
    <li>add a source file named foo_test.c;</li>
    <li>add this to src/test/CMakeLists(as shown below) </li>
    <li> build foo-test.elf and run. </li>
</ol>

src/test/CMakeLists.txt before:
```cmake
# test cases
# Hint: (use semicolon to separate; do NOT add whitespaces)
set(tcases "debug;bitmap")
```

src/test/CMakeLists.txt after:
```cmake
# test cases
# Hint: (use semicolon to separate; do NOT add whitespaces)
set(tcases "debug;bitmap;foo")
#                       ^^^^
```
