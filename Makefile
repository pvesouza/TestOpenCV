GCC = g++
PROJECT = TestOpenCV
SRC = main.cpp

LIBS = `pkg-config --cflags --libs opencv4`

$(PROJECT) : $(SRC)
	$(GCC) $(SRC) -o $(PROJECT) $(LIBS)