# GOOD INPUT
./ebt -p1 -e 'probe insn {}'
./ebt -p1 -e 'probe insn ($opcode == "div") {}'
./ebt -p1 -e 'probe function ($name == "foo") and insn ($opcode == "div") {}'
./ebt -p1 -e 'probe insn and not function ($name == "boring") {}'
./ebt -p1 ./test/parse.good/1.ebt
./ebt -p1 ./test/parse.good/2.ebt # -- test the '::' restriction operator

# BAD INPUT
./ebt -p1 ./test/parse.bad/1.ebt
./ebt -p1 ./test/parse.bad/2.ebt
./ebt -p1 ./test/parse.bad/3.ebt
