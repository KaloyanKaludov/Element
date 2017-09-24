INCLUDE_PATH = ../source
SOURCE_PATH = ../source

OBJECTS_PATH = ./obj

EXECUTABLE_NAME = ../bin/element

CC=g++
CFLAGS=-O2 -std=c++14 -Wall -I$(INCLUDE_PATH)
LFLAGS=-L.

_HEADER_FILES = $(shell ls $(INCLUDE_PATH) | grep .h)
HEADER_FILES = $(patsubst %,$(INCLUDE_PATH)/%,$(_HEADER_FILES))

SOURCE_FILES = $(shell ls $(SOURCE_PATH) | grep .cpp)

_OBJECT_FILES = $(patsubst %.cpp,%.obj,$(SOURCE_FILES))
OBJECT_FILES = $(patsubst %,$(OBJECTS_PATH)/%,$(_OBJECT_FILES))

#####################################################################

all: $(OBJECT_FILES)
	$(CC) -o $(EXECUTABLE_NAME) $^ $(LFLAGS)

$(OBJECTS_PATH):
	mkdir $(OBJECTS_PATH)

$(OBJECTS_PATH)/%.obj: $(SOURCE_PATH)/%.cpp $(HEADER_FILES) $(OBJECTS_PATH)
	$(CC) -c $< -o $@ $(CFLAGS)

.PHONY: clean

clean:
	rm -rf $(OBJECTS_PATH)
	rm $(EXECUTABLE_NAME)
