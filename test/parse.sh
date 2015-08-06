# Be sure to run using bash -x.

# GOOD INPUT
./ebt -p1 -e 'probe insn {}'
./ebt -p1 -e 'probe insn ($opcode == "div") {}'
./ebt -p1 -e 'probe function ($name == "foo") and insn ($opcode == "div") {}'
./ebt -p1 -e 'probe insn and not function ($name == "boring") {}'
./ebt -p1 ./test/parse.good/1.ebt
./ebt -p1 ./test/parse.good/2.ebt # -- test the '::' restriction operator
# ALSO THE REAL TEST PROGRAMS
./ebt -p1 ./dr-demo/empty.ebt
./ebt -p1 ./dr-demo/fcalls.ebt
./ebt -p1 ./dr-demo/hello.ebt
./ebt -p1 ./dr-demo/insn_div_array.ebt
./ebt -p1 ./dr-demo/insn_div.ebt
./ebt -p1 ./dr-demo/insn_div_fn.ebt
# TODOXXX ./ebt -p1 ./dr-demo/memdummy.ebt
# TODOXXX ./ebt -p1 ./dr-demo/memwatch.ebt

# BAD INPUT
./ebt -p1 ./test/parse.bad/1.ebt
./ebt -p1 ./test/parse.bad/2.ebt
./ebt -p1 ./test/parse.bad/3.ebt
./ebt -p1 ./test/parse.bad/4.ebt
