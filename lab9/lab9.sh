#!/bin/bash

gcc -o lab9 lab9.c -lpthread 

if [ $? -eq 0 ]; then
    sudo ./lab9
else
    echo "Ошибка компиляции!"
    exit 1
fi
