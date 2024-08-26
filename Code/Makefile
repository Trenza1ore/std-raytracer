# Variables
CC = g++ -pthread 
CFLAGS = -w -O3 -march=native -std=c++17
INCLUDE_DIRS = -I.
SRC_DIR = .
BIN_DIR = ../bin
OBJ_DIR = $(BIN_DIR)/obj

# Platform-specific configuration
ifeq ($(OS),Windows_NT)
    RM = rmdir /Q /S
    MKDIR = mkdir
    CP = copy
    BIN_DIR := ..\bin
    OBJ_DIR := $(BIN_DIR)\obj
    # Link with the filesystem library if needed (for g++ on Windows this is often not required)
    LDFLAGS = 
else
    RM = rm -rf
    MKDIR = mkdir -p
    CP = cp
    OBJ_DIR = $(BIN_DIR)/obj
    # Link with the filesystem library (for Linux and possibly Mac)
    LDFLAGS = -lstdc++fs
endif


# Targets
all: clean prepare $(BIN_DIR)/RunRaytracer

clean:
	-$(RM) $(BIN_DIR)

prepare:
	$(MKDIR) $(BIN_DIR)
	$(MKDIR) $(OBJ_DIR)

$(OBJ_DIR)/MyMatrix.o: $(SRC_DIR)/MyMatrix.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(OBJ_DIR)/MyVector.o: $(SRC_DIR)/MyVector.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@
    
$(OBJ_DIR)/PlyParser.o: $(SRC_DIR)/PlyParser.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(OBJ_DIR)/SceneObject.o: $(SRC_DIR)/SceneObject.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(OBJ_DIR)/Lights.o: $(SRC_DIR)/Lights.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(OBJ_DIR)/ImageProcessing.o: $(SRC_DIR)/ImageProcessing.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(OBJ_DIR)/MyCamera.o: $(SRC_DIR)/MyCamera.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@
    
$(OBJ_DIR)/ImagePPM.o: $(SRC_DIR)/ImagePPM.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(OBJ_DIR)/RunRaytracer.o: $(SRC_DIR)/RunRaytracer.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(BIN_DIR)/RunRaytracer: $(OBJ_DIR)/MyMatrix.o $(OBJ_DIR)/MyVector.o $(OBJ_DIR)/PlyParser.o $(OBJ_DIR)/SceneObject.o $(OBJ_DIR)/Lights.o $(OBJ_DIR)/ImageProcessing.o $(OBJ_DIR)/MyCamera.o $(OBJ_DIR)/ImagePPM.o $(OBJ_DIR)/RunRaytracer.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)


run: all
	$(BIN_DIR)/RunRaytracer
