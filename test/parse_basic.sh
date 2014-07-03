# GOOD INPUT
./sj -p1 -e 'probe insn {}'
./sj -p1 -e 'probe insn ($opcode == "div", $fname == "foo") {}'
./sj -p1 -e 'probe insn ($opcode == "div", $fname != "boring") {}'
./sj -p1 ./test/parse.good/basic1.sj

# BAD INPUT
./sj -p1 ./test/parse.bad/1.sj
./sj -p1 ./test/parse.bad/2.sj
./sj -p1 ./test/parse.bad/3.sj
./sj -p1 ./test/parse.good/1.sj
./sj -p1 ./test/parse.good/2.sj
