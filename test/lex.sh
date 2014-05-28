# TODOXXX modify to use -pN option as we test sj beyond just a lexer
./sj -g /dev/null -e "123"
./sj -g /dev/null -e "probe insn {}"

./sj -g /dev/null ./test/lex.good/1.sj
./sj -g /dev/null ./test/lex.good/2.sj
./sj -g /dev/null ./test/lex.bad/1.sj
./sj -g /dev/null ./test/lex.bad/2.sj
./sj -g /dev/null ./test/lex.bad/3.sj
./sj -g /dev/null ./test/lex.bad/4.sj
./sj -g /dev/null ./test/lex.bad/5.sj

