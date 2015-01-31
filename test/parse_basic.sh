# GOOD INPUT
./ebt -p1 -e 'probe insn {}'
./ebt -p1 -e 'probe insn ($opcode == "div", $fname == "foo") {}'
./ebt -p1 -e 'probe insn ($opcode == "div", $fname != "boring") {}'
./ebt -p1 ./test/parse.good/basic1.ebt

# BAD INPUT
./ebt -p1 ./test/parse.bad/1.ebt
./ebt -p1 ./test/parse.bad/2.ebt
./ebt -p1 ./test/parse.bad/3.ebt
./ebt -p1 ./test/parse.good/1.ebt
./ebt -p1 ./test/parse.good/2.ebt
