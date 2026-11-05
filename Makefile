CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror
CORE = src/dcc/decoder.cpp

.PHONY: test clean
test: build/native/decoder_tests
	./build/native/decoder_tests

build/native/decoder_tests: $(CORE) tests/decoder_tests.cpp include/dcc/decoder.hpp
	mkdir -p build/native
	$(CXX) $(CXXFLAGS) -Iinclude $(CORE) tests/decoder_tests.cpp -o $@

clean:
	rm -rf build/native
