# Compiler settings - Can change to clang++ if preferred
CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread

# Build targets
TARGET = threadpool
SRC = thread-pool.cc main.cc Semaphore.cc
OBJ = $(SRC:.cc=.o)

# Test targets
TEST_TARGET = test
TEST_SRC = tptest.cc
TEST_OBJ = $(TEST_SRC:.cc=.o)

# Second test target
TEST_TARGET2 = customtest
TEST_SRC2 = tpcustomtest.cc
TEST_OBJ2 = $(TEST_SRC2:.cc=.o)

# Link the target with object files
$(TARGET): $(filter-out $(TEST_OBJ),$(OBJ))
	$(CXX) $(CXXFLAGS) -o $@ $^

# Link the test target with test files and the main target
$(TEST_TARGET): $(filter-out main.o,$(OBJ)) $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Link the second test target with test files and the main target
$(TEST_TARGET2): $(filter-out main.o,$(OBJ)) $(TEST_OBJ2)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Run the program with Valgrind
valgrind: $(TARGET)
	valgrind --leak-check=full ./$(TARGET)

# Clean up build artifacts
clean:
	rm -f $(TARGET) $(TEST_TARGET) $(TEST_TARGET2) $(OBJ) $(TEST_OBJ) $(TEST_OBJ2)

.PHONY: all clean valgrind