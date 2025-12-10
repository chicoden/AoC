cmake -D ERRORS_AS_WARNINGS=%1 -B build -G "NMake Makefiles"
cd build
nmake | findstr /v "vk_mem_alloc.h"
.\CephalopodMath.exe "..\..\input.txt"
cd ..
