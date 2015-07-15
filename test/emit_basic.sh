# Be sure to run using bash -x.

./ebt -p3 -e 'probe insn {}'
./ebt -p3 -e 'probe insn ($opcode == "div", $fname == "foo") {}'
./ebt -p3 -e 'probe insn ($opcode == "div", $fname != "boring") {}'
./ebt -p3 ./test/emit.good/basic1.ebt

./ebt -p3 -e 'probe insn (($fname == "extra" && $opcode == "mul") || $opcode == "div") {}'
./ebt -p3 -e 'probe insn ($fname == "this" ? $opcode == "div" : $opcode == "mul") {}'
