CC = g++

sj: main.cc
	$(CC) -o $@ $<

all: sj

clean:
	rm -f ./sj
