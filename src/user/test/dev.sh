stat /dev/zero
echo foo bar baz > /dev/zero 
stat /dev/zero
echo foo bar baz > /dev/null
stat /dev/null
