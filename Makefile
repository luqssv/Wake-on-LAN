CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2

.PHONY: all clean test

all: wol

wol: main.cpp wol.cpp
	$(CXX) $(CXXFLAGS) -o wol main.cpp wol.cpp

test: wol
	$(CXX) $(CXXFLAGS) -o test_wol tests/test_wol.cpp wol.cpp
	./test_wol

clean:
	rm -f wol test_wol