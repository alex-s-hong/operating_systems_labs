CC=g++
SHELL:=/bin/bash

all:
	. /usr/share/Modules/init/bash; \
	module load gcc-6.3.0; \
	$(CC) -std=c++11 -o linker linker.cpp
clean:
	rm -f linker *~
