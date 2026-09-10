CXX = g++
CXXFLAGS = -std=c++20 -O3 -Wall -Wextra -Iinclude -fopenmp

CORE_SRCS = src/physics/calibrator.cpp \
            src/physics/speckle_filter.cpp \
            src/physics/polarimetry.cpp \
            src/nn/layers.cpp \
            src/nn/unet.cpp \
            src/nn/sliding_window.cpp \
            src/geo/slick_analyzer.cpp \
            src/geo/geojson_writer.cpp \
            src/simulation/synthetic_scene.cpp \
            src/drift/lagrangian_drift.cpp \
            src/attribution/cfar_detector.cpp \
            src/attribution/ais_engine.cpp \
            src/io/ais_parser.cpp \
            src/pipeline/pipeline_orchestrator.cpp

CORE_OBJS = $(patsubst src/%.cpp, build/%.o, $(CORE_SRCS))

TARGET = bin/sar_oil_detector.exe
TEST_TARGET = bin/test_stress_suite.exe

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(CORE_OBJS) build/main.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $^ -o $(TARGET)

$(TEST_TARGET): $(CORE_OBJS) build/tests/test_stress_suite.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $^ -o $(TEST_TARGET)

build/tests/%.o: tests/red_team/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -rf build bin detected_spills.geojson web/detected_spills.geojson
