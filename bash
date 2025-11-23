g++ -o system_service.exe system_service.cpp -lwininet -lws2_32 -static -O2 -s

mkdir build && cd build
cmake .. -G "MinGW Makefiles"
make
