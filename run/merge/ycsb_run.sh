#!/bin/bash

#ARG=""
#ARG="-cmb"
ARG="-cmb -append 40000 -thres 40"
WORKLOAD=("workloada" "workloadb" "workloadd" "workloadf")
DIST=("uni" "zip")

[ -d "email" ] && rm email

for WL in "${WORKLOAD[@]}"
do
    if [ "${WL}" != "worklaodd" ];then
        for DT in "${DIST[@]}"
        do

            echo "#############################################################" | tee email
            echo "YCSB-Benchmark: ${WL}_${DT}" | tee -a email

            # YCSB-A,B,F
            ./merge_btree.out $ARG -input ../../data/ycsb/${WL}_${DT}.txt /dev/nvme0n1 

            echo " Done " | tee -a email
            echo "#############################################################" | tee -a email
            cat email | mail -s "YCSB-Benchmark: ${WL}_${DT} Finish" josephly88@yahoo.com.hk
            rm email

        done
    else
            echo "#############################################################" | tee email
            echo "YCSB-Benchmark: ${WL}" | tee -a email

            # YCSB-D
            ./merge_btree.out $ARG -input ../../data/ycsb/${WL}.txt /dev/nvme0n1 

            echo " Done " | tee -a email
            echo "#############################################################" | tee -a email
            cat email | mail -s "YCSB-Benchmark: ${WL} Finish" josephly88@yahoo.com.hk
            rm email
    fi
done

