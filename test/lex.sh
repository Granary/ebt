# GOOD INPUT
./sj -p0 -e '123'
./sj -p0 -e 'probe insn {}'
./sj -p0 ./test/lex.good/1.sj
./sj -p0 ./test/lex.good/2.sj

# BAD INPUT
./sj -p0 ./test/lex.bad/1.sj
./sj -p0 ./test/lex.bad/2.sj
./sj -p0 ./test/lex.bad/3.sj
./sj -p0 ./test/lex.bad/4.sj
./sj -p0 ./test/lex.bad/5.sj

