CXX = g++-6
CXX_FLAGS = -I./include --std=c++11 -O3
SRC = $(wildcard src/*.cpp)

BENCHMARKS = obstruction_free_set lock_free_set

BM = $(BENCHMARKS:%=benchmarks/%)

.PHONY: all
all: $(BM)

$(BM): %: %.cpp $(SRC)
	$(CXX) $(CXX_FLAGS) $^ -o $@

clean:
	rm -f $(BM)

