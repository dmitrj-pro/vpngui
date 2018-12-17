#!/bin/bash

QTSTART=$(cat GUI/build/start.txt)
QTEND=$(cat GUI/build/end.txt)
SRVSTART=$(cat Core/build/Makefile.0)
SRVEND=$(cat Core/build/Makefile.1)
CCSTART=$(cat Core/build/MakefileClient.0)

printf "Input path to libdp: "
read PATH

printf "Input encrypt password: "
read PASSWORD

printf "Debug (y)/Release(n): "
read DEBUG

printf "Path to configs (Ex: /home/vpn): "
read VPNPATH

printf "System config name (Ex: /home/vpn/vpn.conf): "
read VPNCONFIG

printf "$SRVSTART" > Core/Makefile
printf " -I$PATH\n" >> Core/Makefile
printf "LDFLAGS=-pthread $PATH/libdp.a\n" >> Core/Makefile
printf "$SRVEND" >> Core/Makefile

printf "$CCSTART" > Core/MakefileClient


printf "#ifndef __DP_DEFINES__\n#define __DP_DEFINES__\n" > Core/DEFINES.h
printf "#define VPN_CONFIGS_PATH \"$VPNPATH\"\n" >> Core/DEFINES.h
printf "#define ConfigDB \"$VPNCONFIG\"\n" >> Core/DEFINES.h
printf "#define ENC_PASSWORD \"$PASSWORD\"\n" >> Core/DEFINES.h
if [[ $DEBUG == y* ]]
then
    printf "#define DP_DEBUG\n" >> Core/DEFINES.h
fi
printf "#endif" >> Core/DEFINES.h

printf "$QTSTART\n" > GUI/GUI.pro
printf "LIBS += $PATH/libdp.a\n" >> GUI/GUI.pro
printf "INCLUDEPATH += $PATH\n" >> GUI/GUI.pro
if [[ $DEBUG == y* ]]
then
    printf "DEFINES += DP_DEBUG\n" >> GUI/GUI.pro
fi

printf "$QTEND" >> GUI/GUI.pro



