CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?= -Iinclude
BUILD_DIR := build
LIB_SOURCES := src/logic.cpp src/pipeline.cpp
APP := $(BUILD_DIR)/tableau-es
TEST_APP := $(BUILD_DIR)/test-pipeline

.PHONY: all run test clean

all: $(APP)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(APP): $(LIB_SOURCES) src/main.cpp include/semantic_tableau/logic.hpp include/semantic_tableau/pipeline.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LIB_SOURCES) src/main.cpp -o $(APP)

$(TEST_APP): $(LIB_SOURCES) tests/test_pipeline.cpp include/semantic_tableau/logic.hpp include/semantic_tableau/pipeline.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LIB_SOURCES) tests/test_pipeline.cpp -o $(TEST_APP)

run: $(APP)
	$(APP) $(ARGS)

test: $(TEST_APP)
	$(TEST_APP)

clean:
	rm -rf $(BUILD_DIR)