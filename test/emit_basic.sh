./sj -e 'probe insn {}'
./sj -e 'probe insn ($opcode == "div", $fname == "foo") {}'
./sj -e 'probe insn ($opcode == "div", $fname != "boring") {}'
./sj ./test/emit.good/basic1.sj
