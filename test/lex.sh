# GOOD INPUT
./ebt -p0 -e '123'
./ebt -p0 -e 'probe insn {}'
./ebt -p0 ./test/lex.good/1.ebt
./ebt -p0 ./test/lex.good/2.ebt

# BAD INPUT
./ebt -p0 ./test/lex.bad/1.ebt
./ebt -p0 ./test/lex.bad/2.ebt
./ebt -p0 ./test/lex.bad/3.ebt
./ebt -p0 ./test/lex.bad/4.ebt
./ebt -p0 ./test/lex.bad/5.ebt

