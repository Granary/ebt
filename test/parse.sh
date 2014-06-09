# GOOD INPUT
./sj -p1 -e 'probe insn {}'
./sj -p1 -e 'probe insn ($opcode == "div") {}'
./sj -p1 -e 'probe function ($name == "foo") and insn ($opcode == "div") {}'
./sj -p1 -e 'probe insn and not function ($name == "boring") {}'
./sj -p1 ./test/parse.good/1.sj

# BAD INPUT
./sj -p1 ./test/parse.bad/1.sj
