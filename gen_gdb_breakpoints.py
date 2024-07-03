#!python

import os


with open("obj/bootsector.sym", "r") as symbol_table:
    for line in symbol_table:
        print(line)
