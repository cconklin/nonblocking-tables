CXX = g++-6
CXX_FLAGS = -I./include --std=c++11 -O3
SRC = $(wildcard src/*.cpp)

BENCHMARKS = obstruction_free_set

BM = $(BENCHMARKS:%=benchmarks/%)

$(BM): %: %.cpp $(SRC)
	$(CXX) $(CXX_FLAGS) $^ -o $@

clean:
	rm -f $(BM)

