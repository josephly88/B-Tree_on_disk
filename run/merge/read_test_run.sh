#!/bin/bash

ARG="-cmb -append 40000 -thres"
THRES=(5)

[ -d "email" ] && rm email

for TH in "${THRES[@]}"
do
    echo "#############################################################" | tee email
    echo "Read Threshold Test: ${TH}" | tee -a email

# READ-TEST
    ./merge_btree.out $ARG $TH -input ../../data/read_test/read_test_final.txt /dev/nvme0n1
    mv read_test_final.dat read_test_final_${TH}.dat

    echo " Done " | tee -a email
    echo "#############################################################" | tee -a email 
    cat email | mail -s "Read Threshold Test: ${TH} Finish" josephly88@yahoo.com.hk
    rm email
done
