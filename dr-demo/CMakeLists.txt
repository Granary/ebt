# In a different folder, build these demos with something like:
# $ cmake -DDynamoRIO_DIR=$DYNAMORIO_HOME/cmake /path/to/dr-demo
# $ make
# $ drrun -root $DYNAMORIO_HOME -c libhello.so -- ls

# TODOXXX This setup seems to ONLY work with DR 5.0.0-9 and not 5.1.0-RC

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

# TODOXXX Umbra is included by default with DynamoRIO and DrMemory
# TODOXXX However the DrMemory bundled in my install is 32-bit only
# TODOXXX Some way to resolve this is necessary down the line?

# Watchpoints example also requires Umbra:
#find_package(DrMemoryFramework)
#if (DrMemoryFramework_FOUND)
  add_library(memdummy SHARED memdummy.c)
  #add_library(memwatch SHARED memwatch.c)

  configure_DynamoRIO_client(memdummy)
  use_DynamoRIO_extension(memdummy umbra)

  #configure_DynamoRIO_client(memwatch)
  #use_DynamoRIO_extension(memwatch drcontainers)
  #use_DynamoRIO_extension(memwatch drsyms)
  #use_DynamoRIO_extension(memwatch umbra)
#else (DrMemoryFramework_FOUND)
#  message(FATAL_ERROR "DrMemory (Umbra) package required to build memwatch")
#endif (DrMemoryFramework_FOUND)
