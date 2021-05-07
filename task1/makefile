all: mypipeline

mypipeline: mypipeline.o LineParser.o
	gcc -g -m32 -Wall -o mypipeline mypipeline.o LineParser.o

mypipeline.o: mypipeline.c
	gcc -g -m32 -Wall -c -o mypipeline.o mypipeline.c

LineParser.o: LineParser.c LineParser.h
	gcc -g -m32 -Wall -c -o LineParser.o LineParser.c

.PHONY: clean

clean:
	rm -rf ./*.o mypipeline