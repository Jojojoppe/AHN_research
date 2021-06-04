#!/bin/bash

OMNETPP_BIN=/opt/omnetpp/bin
INET4_3_PROJ=/opt/omnetpp/samples/inet4.3
VEINS_PROJ=~/Files/Study/MSc/2020-2021/2B/AHN/veins
D=
U=Qtenv
THREADS=6

rm -rf simulations/static/results

[[ $1 == "cmd" ]] && U=Cmdenv

PATH=$OMNETPP_BIN:$PATH

opp_makemake -f --deep \
    -KINET4_3_PROJ=$INET4_3_PROJ \
    -KVEINS_PROJ=$VEINS_PROJ \
    -DINET_IMPORT \
    -DVEINS_IMPORT \
    -I$INET4_3_PROJ/src \
    -I$VEINS_PROJ/src \
    -Isrc \
    -L$INET4_3_PROJ/src \
    -L$VEINS_PROJ/src \
    -lINET$D \
    -lveins$D 

CURDIR=$(pwd)

cd $INET4_3_PROJ && \
    make -j$THREADS all && \
cd $VEINS_PROJ && \
    make -j$THREADS all && \
cd $CURDIR && \
    make -j$THREADS all && \

cd simulations/static && \
# ../../wireless -m \
#     -n ..:../../src:$INET4_3_PROJ/src:$VEINS_PROJ/examples/veins:$VEINS_PROJ/src/veins \
#     --image-path=$INET4_3_PROJ/images:$VEINS_PROJ/images \
#     -l $INET4_3_PROJ/src/INET -l $VEINS_PROJ/src/veins omnetpp.ini \
#     -u $U && \
opp_runall -j $THREADS ../../wireless -m \
    -n ..:../../src:$INET4_3_PROJ/src:$VEINS_PROJ/examples/veins:$VEINS_PROJ/src/veins \
    --image-path=$INET4_3_PROJ/images:$VEINS_PROJ/images \
    -l $INET4_3_PROJ/src/INET -l $VEINS_PROJ/src/veins omnetpp.ini \
    -u $U && \


cd $CURDIR && \
    cd simulations/static/results && \
#     opp_scavetool x *.sca \
#         -f 'name =~ **delays**' \
#         -o ../../../res.csv \
    opp_scavetool x *.sca \
        -f 'name =~ **schedule**' \
        -o ../../../schedule.csv \

# cd $CURDIR && \
#     python process.py && \

cd $CURDIR && \
    python schedule.py && \


echo DONE