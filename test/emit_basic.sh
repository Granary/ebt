./sj -e 'probe insn {}'
./sj -e 'probe insn ($opcode == "div", $fname == "foo") {}'
./sj -e 'probe insn ($opcode == "div", $fname != "boring") {}'
./sj ./test/emit.good/basic1.sj

./sj -e 'probe insn (($fname == "extra" && $opcode == "mul") || $opcode == "div") {}'
./sj -e 'probe insn ($fname == "this" ? $opcode == "div" : $opcode == "mul") {}'
