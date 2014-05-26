CC = g++ -g

SJ_SOURCES = main.cc util.h util.cc ir.h ir.cc parse.h parse.cc

sj: $(SJ_SOURCES)
	$(CC) -o $@ $(SJ_SOURCES)

all: sj

clean:
	rm -f ./sj
