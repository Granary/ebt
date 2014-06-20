# GOOD INPUT
./sj -p1 -e 'probe insn {}'
./sj -p1 -e 'probe insn ($opcode == "div") {}'
./sj -p1 -e 'probe function ($name == "foo") and insn ($opcode == "div") {}'
./sj -p1 -e 'probe insn and not function ($name == "boring") {}'
./sj -p1 ./test/parse.good/1.sj
./sj -p1 ./test/parse.good/2.sj # -- test the '::' restriction operator

# BAD INPUT
./sj -p1 ./test/parse.bad/1.sj
./sj -p1 ./test/parse.bad/2.sj
./sj -p1 ./test/parse.bad/3.sj
