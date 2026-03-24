#!/bin/bash

C_RED='\0332[1;31m'
C_BLU='\033[1;34m'
C_GRN='\033[1;32m'
C_NC='\033[0m'

logln ()
{
	printf "${C_BLU} [!] :: $1 ${C_NC}\n"
}

logerr ()
{
	printf "${C_RED} [!ERR] :: $1${C_NC}\n"
}

logln "Initializing..."

if [ ! -d "../linux-build" ]; then
	mkdir ../linux-build
	#rm -rf ../linux-build
	#echo [!]: Deleted previous build files
fi

logln "Generating build files..."
cd ../linux-build
cmake ../
if [ $? -ne 0 ]; then
	loger "CMake generation failed! Aborting..."
	exit 1
fi

logln "Generation of build files finished."
logln "Building..."
cd ../
cmake --build linux-build/
if [ $? -ne 0 ]; then
	logerr "CMake build failed!"
	exit 1
fi
logln "Build finished."
logln "Executing binary"
cd linux-build/TestGame
./TestGame
