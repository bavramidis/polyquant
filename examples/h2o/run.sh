#!/bin/bash


../../build/bin/polyquant -i h2o.json
~/Documents/qmcpack/build/bin/convert4qmc -orbitals electron.h5


#python h2o.py
#
#
#rm H_core.txt
#rm kinetic.txt
#rm nuclear.txt
#rm overlap.txt
#rm twoelec.txt
#rm F.txt
