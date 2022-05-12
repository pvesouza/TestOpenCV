GCC = g++
PROJECT = RecCalc
SRC = main.cpp fileHelper.cpp

LIBS = `pkg-config --cflags --libs opencv4`

$(PROJECT) : $(SRC)
	$(GCC) $(SRC) -o $(PROJECT) $(LIBS)