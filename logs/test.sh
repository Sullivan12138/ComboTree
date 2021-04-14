#!/bin/bash
#output = `../build/ycsb -db fastfair -threads 1 -P ../include/ycsb/insert_ratio/ | awk '$1=="op" {print $2, $6}' > intel-nvm/fastfair_latency.log`
#output = `../build/ycsb -db pgm -threads 1 -P ../include/ycsb/insert_ratio/ | awk '$1=="op" {print $2, $6}' > intel-nvm/pgm_latency.log`
#output = `../build/ycsb -db xindex -threads 1 -P ../include/ycsb/insert_ratio/ | awk '$1=="op" {print $2, $6}' > intel-nvm/xindex_latency.log`
#output = `../build/ycsb -db alex -threads 1 -P ../include/ycsb/insert_ratio/ | awk '$1=="op" {print $2, $6}' > intel-nvm/alex_latency.log`
output = `../build/ycsb -db learngroup -threads 1 -P ../include/ycsb/insert_ratio/ | awk '$1=="op" {print $2, $6}' > intel-nvm/learngroup_latency.log`
