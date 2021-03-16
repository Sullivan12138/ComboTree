#!/bin/bash
output = `../build/ycsb -db fastfair -threads 1 -P ../include/ycsb/insert_ratio/ > fastfair_nvm.log`
output = `../build/ycsb -db pgm -threads 1 -P ../include/ycsb/insert_ratio/ > pgm_nvm.log`
output = `../build/ycsb -db alex -threads 1 -P ../include/ycsb/insert_ratio/ > alex_nvm.log`
