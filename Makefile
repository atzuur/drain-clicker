drain.exe: src/main.c
	gcc src/main.c -o drain.exe -municode -lgdi32 -mwindows
