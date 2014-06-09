#!/bin/sh
cc $1 -g -o leaky.a.out
atom leaky.a.out malloc.inst.c malloc.anal.c -o leaky.atom
./leaky.atom
