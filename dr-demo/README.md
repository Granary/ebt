# DynamoRIO Sample Code Collection

The DynamoRIO clients in this folder are used to test features which EBT needs to support, before implementing them. The `.c` files are handwritten clients, while `.gen.c` files are automatically produced by EBT from the `.ebt` scripts of the same name.

We write and debug a handwritten client along with the EBT script which implements the same analysis, then make sure that EBT produces a client with the same structure and behaviour as the handwritten one.

To compile and run the hand-written clients, use a procedure along the following lines:

    mkdir build
    cd build
    cmake ..
    make
    
    # Compile a suitable target program:
    gcc ../test/divtest3.c -o divtest3
    
    # Now, to run e.g. the insn_div client on a 64-bit system:
    $DYNAMORIO_HOME/bin64/drrun -c libinsn_div.so -- divtest3

To test the corresponding EBT script (in the top level of the EBT repo):

    # Be sure to set EBT_HOME (manually or in .bash_profile):
    export EBT_HOME=/path/to/ebt/repo
    
    ./ebt dr-demo/insn_div.ebt -- dr-demo/build/divtest3
    
    # If you need to regenerate the .gen.c file
    ./ebt -o dr-demo/insn_div.ebt >dr-demo/insn_div.gen.c
