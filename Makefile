all: linda

linda: main.cpp
	clang++ -Xpreprocessor -fopenmp -lomp main.cpp -o linda -std=c++11

run: linda
	rm -rf *.txt
	./linda

format: main.cpp
	clang-format -i main.cpp

clean:
	rm -rf linda *.txt