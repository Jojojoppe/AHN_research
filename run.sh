#!/bin/bash

INET4_3_PROJ=/opt/omnetpp/samples/inet4.3
VEINS_PROJ=~/Files/Study/MSc/2020-2021/2B/AHN/veins
D=

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
    make all && \
cd $VEINS_PROJ && \
    make all && \
cd $CURDIR && \
make all && \

cd simulations/static && \
../../wireless -m \
    -n ..:../../src:$INET4_3_PROJ/src:$VEINS_PROJ/examples/veins:$VEINS_PROJ/src/veins \
    --image-path=$INET4_3_PROJ/images:$VEINS_PROJ/images \
    -l $INET4_3_PROJ/src/INET -l $VEINS_PROJ/src/veins omnetpp.ini