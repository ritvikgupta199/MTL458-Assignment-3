# Sets COMPILER to the default compiler for OSX and other systems
# taken from https://stackoverflow.com/questions/24563150/makefile-with-os-dependent-compiler
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
	COMPILER = clang 
else
	COMPILER = gcc
endif

run:
	./frames example_trace.in 100 OPT
clean:
	rm -f frames
build:
	rm -f frames
	$(COMPILER) frames.c -o frames
all:
	rm -f frames
	$(COMPILER) frames.c -o frames
	clear
	./frames trace.in 4 LRU -verbose