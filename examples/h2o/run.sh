#!/bin/bash


../../build/bin/polyquant -i h2o.json
~/Documents/qmcpack/build/bin/convert4qmc -orbitals electron.h5
~/Documents/qmcpack/build/bin/qmcpack electron.qmc.in-wfnoj.xml



#python h2o.py
#
#
#rm H_core.txt
#rm kinetic.txt
#rm nuclear.txt
#rm overlap.txt
#rm twoelec.txt
#rm F.txt
