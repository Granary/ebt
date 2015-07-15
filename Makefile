# TODOXXX need some Autoconf nonsense in the longer term
CC = g++ -g -lssl /usr/lib64/libcrypto.so.10
#CC = g++ -g -lssl /usr/lib/libcrypto.so.10 # on 32-bit system

ebt_SOURCES = main.cc util.h util.cc ir.h ir.cc parse.h parse.cc emit.h emit.cc

ebt: $(ebt_SOURCES)
	$(CC) -o $@ $(ebt_SOURCES)

all: ebt

check: ebt
	test/test_all.sh

clean:
	rm -f ./ebt

# Purely informational targets:

count:
	@echo "PROJECT SOURCE CODE SUMMARY"
	@echo "==========================="
	@wc *.cc *.h Makefile README.md test/*.sh runtime/*.h
