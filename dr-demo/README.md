# DynamoRIO Sample Code Collection

The DynamoRIO clients in this folder are used to test features which EBT needs to support, before implementing them. We write and debug a handwritten client along with the EBT script which implements the same analysis, then make sure that EBT produces a client with the same structure and behaviour as the handwritten one.

To compile and run the hand-written clients, use a procedure along the following lines:

    mkdir build
    cd build
    cmake ..
    make
    
    # Compile a suitable target program:
    gcc ../test/divtest3.c -o divtest3
    
    # Now, to run e.g. the insn_div client on a 64-bit system:
    $DYNAMORIO_HOME/bin64/drrun -c libinsn_div.so -- divtest3
