CXX = g++
CXX_FLAGS = -I./include --std=c++11 -O3 -lpthread

BENCHMARKS = $(patsubst benchmarks/%.cpp, %, $(wildcard benchmarks/*.cpp))
BM = $(BENCHMARKS:%=bin/%)
BASELINE = $(patsubst baseline/%.cpp, %, $(wildcard baseline/*.cpp))
BL = $(BASELINE:%=bin/%)
SOURCE_OBJECTS = $(patsubst src/%.cpp,objects/%.o,$(wildcard src/*.cpp))
BASELINE_OBJECTS = $(patsubst %, objects/baseline_%.o, $(BASELINE))
BENCHMARK_OBJECTS = $(patsubst %, objects/benchmark_%.o, $(BENCHMARKS))
OBJECTS = $(BENCHMARK_OBJECTS) $(SOURCE_OBJECTS) $(BASELINE_OBJECTS)

.PHONY: all
all: bin objects $(BM) $(BL)

bin:
	mkdir bin

objects:
	mkdir objects

$(BENCHMARK_OBJECTS): objects/benchmark_%.o: benchmarks/%.cpp
	$(CXX) $(CXX_FLAGS) -o $@ -c $<

$(BASELINE_OBJECTS): objects/baseline_%.o: baseline/%.cpp
	$(CXX) $(CXX_FLAGS) -o $@ -c $<

$(SOURCE_OBJECTS): objects/%.o: src/%.cpp
	$(CXX) $(CXX_FLAGS) -o $@ -c $<

$(BM): bin/%: objects/benchmark_%.o objects/nonblocking.o objects/%.o
	$(CXX) $(CXX_FLAGS) -o $@ objects/benchmark_$*.o objects/nonblocking.o objects/$*.o

$(BL): bin/%: objects/baseline_%.o
	$(CXX) $(CXX_FLAGS) -o $@ objects/baseline_$*.o

clean:
	rm -f $(BM) $(BL)
	rm -f $(SOURCE_OBJECTS) $(BENCHMARK_OBJECTS) $(BASELINE_OBJECTS)

