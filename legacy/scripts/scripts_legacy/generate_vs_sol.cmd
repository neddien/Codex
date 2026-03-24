@echo off
cd ..
if exist build (
 call scripts/EraseSln.cmd
)
mkdir build
#cd build
cmake -G "Visual Studio 17"
cmd /k