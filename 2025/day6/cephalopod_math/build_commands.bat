cmake -B build -G "NMake Makefiles"
cd build
nmake
cd ..
:: .\build\CephalopodMath.exe "..\input.txt"
