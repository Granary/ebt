# In a different folder, build these demos with something like:
# $ cmake -DDynamoRIO_DIR=$DYNAMORIO_HOME/cmake /path/to/dr-demo
# $ make
# $ $DYNAMORIO_HOME/bin64/drrun -c libhello.so -- ls

add_library(empty SHARED empty.c)
add_library(hello SHARED hello.c)
add_library(insn_div SHARED insn_div.c)

find_package(DynamoRIO)
if (NOT DynamoRIO_FOUND)
  message(FATAL_ERROR "DynamoRIO package required to build")
endif (NOT DynamoRIO_FOUND)

configure_DynamoRIO_client(empty)
configure_DynamoRIO_client(hello)
configure_DynamoRIO_client(insn_div)
