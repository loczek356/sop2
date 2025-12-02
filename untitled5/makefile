CXX = gcc
CXXFLAGS =  -Wall -fsanitize=address,leak -g
LDFLAGS = -fsanitize=address,leak

.PHONY: all clean

all: sop2.3

sop2.3: main.o
	${CXX} ${LDFLAGS} $^ -o $@

main.o: main.c
	${CXX} ${CXXFLAGS} -c $< -o $@

clean:
	rm -f *.o L5
