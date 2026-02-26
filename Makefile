# Makefile – PV + BESS Simulator
# PROG2: Buildmanagement mit Makefiles
# Kompilieren: make
# Ausfuehren:  make run
# Aufraeumen:  make clean

CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -O2
TARGET = pv_bess_sim

all: $(TARGET)

$(TARGET): pv_bess_simulator.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) pv_bess_simulator.cpp

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) simulation_output.csv

.PHONY: all run clean
