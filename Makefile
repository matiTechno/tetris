COMM1 = g++ -std=c++11 -Wall -Wextra -pedantic -Wno-nested-anon-types -fno-exceptions \
       -fno-rtti -g

COMM2 = -o tetris main.cpp -lglfw -ldl

linux:
	${COMM1} ${COMM2} ./fmod/libfmod.so.10.4 -Wl,-rpath=./fmod

mac:
	${COMM1} -I/usr/local/include -L/usr/local/Cellar -L/usr/local/lib \
        ${COMM2} ./fmod/libfmod.dylib
