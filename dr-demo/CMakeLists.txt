# In a different folder, build these demos with something like:
# $ cmake -DDynamoRIO_DIR=$DYNAMORIO_HOME/cmake /path/to/dr-demo
# $ make
# $ $DYNAMORIO_HOME/bin64/drrun -c libhello.so -- ls

cmake_minimum_required(VERSION 3.0)

add_library(empty SHARED empty.c)
add_library(hello SHARED hello.c)
add_library(insn_div SHARED insn_div.c)
add_library(insn_div_array SHARED insn_div_array.c)
add_library(insn_div_fn SHARED insn_div_fn.c)
add_library(fcalls SHARED fcalls.c)

find_package(DynamoRIO)
if (NOT DynamoRIO_FOUND)
  message(FATAL_ERROR "DynamoRIO package required to build")
endif (NOT DynamoRIO_FOUND)

configure_DynamoRIO_client(empty)
configure_DynamoRIO_client(hello)
configure_DynamoRIO_client(insn_div)
configure_DynamoRIO_client(insn_div_array)
use_DynamoRIO_extension(insn_div_array drcontainers)
use_DynamoRIO_extension(insn_div_array drsyms)
configure_DynamoRIO_client(insn_div_fn)
use_DynamoRIO_extension(insn_div_fn drsyms)
configure_DynamoRIO_client(fcalls)
use_DynamoRIO_extension(fcalls drsyms)
