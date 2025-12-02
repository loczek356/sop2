CXX = gcc
CXXFLAGS = -std=c++20 -Wall -fsanitize=address,leak -g
LDFLAGS = -fsanitize=address,leak

.PHONY: all clean

all: l5

l5: main.o
	${CXX} ${LDFLAGS} $^ -o $@

main.o: main.c
	${CXX} ${CXXFLAGS} -c $< -o $@

clean:
	rm -f *.o L5
