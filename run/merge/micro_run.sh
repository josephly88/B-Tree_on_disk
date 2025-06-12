#!/bin/bash

#ARG=""
#ARG="-cmb"
#ARG="-cmb -append 40000 -MRU -thres 1"
ARG="-cmb -hb 256"

[ -d "email" ] && rm email

echo "#############################################################" | tee email
echo "Micro-benchmark" | tee -a email

./merge_btree.out $ARG -input ../../data/micro/micro_5M_uni_final.txt /dev/nvme0n1 

echo " Done " | tee -a email
echo "#############################################################" | tee -a email
cat email | mail -s "Micro-benchmark Finish" josephly88@yahoo.com.hk
rm email
