all: quash
quash: main.cpp
	gcc -g main.cpp -lstdc++ -o quash
test: quash
	./quash

clean:
	rm quash
