CXX ?= clang++
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Darwin)
CXXFLAGS += -Xpreprocessor -fopenmp -lomp
else
CXXFLAGS = -fopenmp
endif
CXXFLAGS += -std=c++11
SRC = main.cpp
TARGET = linda

.PHONY: all run format clean
all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

run: $(TARGET)
	./$(TARGET) < testcase/test.txt

format: $(SRC)
	clang-format -i $(SRC)

clean:
	rm -rf $(TARGET) *.txt