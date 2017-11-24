#!/bin/bash

g++ -O2 -c ./new_betta.cpp -L/usr/lib -L/usr/lib/mysql -I'/usr/include/mysql'
#gcc -O2 -c ./JSON_parser.c 
#g++ -x c -std=c99 -static -c ./JSON_parser.c
g++ -llibyajl -lmysqlclient -Wl -o ./tst2 ./new_betta.o
