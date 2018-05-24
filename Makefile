CXX = g++
CFLAGS = 
LFLAGS = -lcrypto
OBJECTS = maketmd.o
TARGET_NAME = maketmd
VERSION_VER = 0.1
VERSION_DATE = built on `date`

ifeq ($(OS),Windows_NT)
	TARGET = $(TARGET_NAME).exe
	LFLAGS += -static -static-libgcc
else
	TARGET = $(TARGET_NAME)
endif

all: $(TARGET)

version.h: Makefile
	@echo -e "#define TMD_CREATOR_VER  \""$(VERSION_VER)"\"" > version.h
	@echo -e "#define TMD_CREATOR_DATE \""$(VERSION_DATE)"\"" >> version.h

%.o: %.cpp version.h
	$(CXX) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(CFLAGS) $^ -o $@ $(LFLAGS)
	
run: $(TARGET)
	@./$(TARGET) test/00000000.app test/title.tmd

clean:
	rm -f $(OBJECTS) $(TARGET) version.h
