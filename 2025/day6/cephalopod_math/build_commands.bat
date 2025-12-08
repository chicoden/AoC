cmake -S src -B build -G "NMake Makefiles"
cd build
nmake
.\CephalopodMath.exe "..\..\input.txt"
cd ..
