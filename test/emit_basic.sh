./sj -p3 -e 'probe insn {}'
./sj -p3 -e 'probe insn ($opcode == "div", $fname == "foo") {}'
./sj -p3 -e 'probe insn ($opcode == "div", $fname != "boring") {}'
./sj -p3 ./test/emit.good/basic1.sj

./sj -p3 -e 'probe insn (($fname == "extra" && $opcode == "mul") || $opcode == "div") {}'
./sj -p3 -e 'probe insn ($fname == "this" ? $opcode == "div" : $opcode == "mul") {}'
