cd build

cmake .. -DGL_SILENCE_DEPRECATION=ON
make -j$(sysctl -n hw.ncpu)






cmake .. -DGL_SILENCE_DEPRECATION=ON


make
