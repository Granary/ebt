CC = g++ -g

SJ_SOURCES = main.cc util.h util.cc ir.h ir.cc parse.h parse.cc emit.h emit.cc

sj: $(SJ_SOURCES)
	$(CC) -o $@ $(SJ_SOURCES)

all: sj

check: sj
	test/test_all.sh

clean:
	rm -f ./sj

# Purely informational targets:

count:
	@echo "PROJECT SOURCE CODE SUMMARY"
	@echo "==========================="
	@wc *.cc *.h Makefile README.md test/*.sh
