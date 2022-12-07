PHONY := dev

outfile = draincl.exe

dev: src/main.c
	gcc src/main.c -o $(outfile) -municode -g3

release: src/main.c
	gcc src/main.c -o $(outfile) -municode -mwindows -O2