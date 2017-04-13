CXX = g++-6
CXX_FLAGS = -I./include --std=c++11 -O3

BENCHMARKS = $(patsubst benchmarks/%.cpp, %, $(wildcard benchmarks/*.cpp))
BM = $(BENCHMARKS:%=bin/%)
SOURCE_OBJECTS = $(patsubst src/%.cpp,objects/%.o,$(wildcard src/*.cpp))
BENCHMARK_OBJECTS = $(patsubst %, objects/benchmark_%.o, $(BENCHMARKS))
OBJECTS = $(BENCHMARK_OBJECTS) $(SOURCE_OBJECTS)

.PHONY: all
all: $(BM)

bin:
	mkdir bin

objects:
	mkdir objects

$(BENCHMARK_OBJECTS): objects/benchmark_%.o: benchmarks/%.cpp objects
	$(CXX) $(CXX_FLAGS) -o $@ -c $<

$(SOURCE_OBJECTS): objects/%.o: src/%.cpp objects
	$(CXX) $(CXX_FLAGS) -o $@ -c $<

$(BM): bin/%: objects/benchmark_%.o objects/nonblocking.o objects/%.o bin
	$(CXX) $(CXX_FLAGS) -o $@ objects/benchmark_$*.o objects/nonblocking.o objects/$*.o

clean:
	rm -f $(BM)
	rm -f $(SOURCE_OBJECTS) $(BENCHMARK_OBJECTS)

