@echo off
cd ..
del RSE.sln
del ZERO_CHECK.vcxproj
del ZERO_CHECK.vcxproj.filters
del ALL_BUILD.vcxproj
del ALL_BUILD.vcxproj.filters
del cmake_install.cmake
del CMakeCache.txt
del INSTALL.vcxproj
del INSTALL.vcxproj.filters
del RSE/INSTALL.vcxproj
del RSE/INSTALL.vcxproj.filters
rmdir /S /Q "fmt"
rmdir /S /Q "SDL"
rmdir /S /Q "SDL_image"
rmdir /S /Q "yaml-cpp"
rmdir /S /Q "x64"
rmdir /S /Q "CMakeFiles"
rmdir /S /Q "build"