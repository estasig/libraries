#!/bin/sh
g++ -c -g -Wall -Werror -I../libmacro/ -I../liblog/ select.cpp -o select.o
g++ -c -g -Wall -Werror -I../libmacro/ -I../liblog/ libgevent.cpp -o libgevent.o
ar rcs libgevent.a libgevent.o select.o
