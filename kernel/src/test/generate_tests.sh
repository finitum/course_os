#!/bin/bash
# this bash script will read all directories to find tests.
# it will make a list of tests and write this to test.c
# the makefile will run this file automatically while building,
# so all tests are automatically executed.

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# clear the file
echo -n "" > "$DIR/test.c"

echo "
// DO NOT EDIT
// this file is generated by generate_tests.sh

#ifdef ENABLE_TESTS

#include <stdio.h>
#include <hw_handlers.h>

" >> "$DIR/test.c"

#TESTFNS=$(grep -hr --include "*.c" -oP "(?<=TEST_CREATE\()(.*)(?=,)")
TESTFNS=$(grep -hr --include "*.c" -vP "^\s*\/\/.+" | grep -oP "(?<=TEST_CREATE\()(.*)(?=,)")

for FNNAME in $TESTFNS
do
  echo "int test_$FNNAME();" >> "$DIR/test.c"
done

echo "void test_main(){" >> "$DIR/test.c"

len=0
for FNNAME in $TESTFNS
do
  echo "    if (!test_$FNNAME()) {return;}" >> "$DIR/test.c"
  ((len++))
done


# shellcheck disable=SC2028
echo "
  kprintf(\"TESTS COMPLETE. Passed %i tests\n\", "$len");
  SemihostingCall(ApplicationExit);
}
#endif
" >> "$DIR/test.c"