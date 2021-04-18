mkdir -p third_party
cd third_party

if [ ! -f "eigen.tar.gz" ]; then
wget https://gitlab.com/libeigen/eigen/-/archive/3.3.9/eigen-3.3.9.tar.gz -O eigen.tar.gz
tar zxvf eigen.tar.gz 
mv eigen-3.3.9 eigen3
fi

if [ ! -f "abseil.tar.gz" ]; then
wget https://github.com/abseil/abseil-cpp/archive/20200923.3.tar.gz -O abseil.tar.gz
tar zxvf abseil.tar.gz
mv abseil-cpp-20200923.3 abseil-cpp
fi

# if [ ! -f "grpc.tar.gz" ]; then
# wget https://github.com/grpc/grpc/archive/v1.36.4.tar.gz -O grpc.tar.gz 
# tar zxvf grpc.tar.gz
# mv grpc-1.36.4 grpc
# fi

#if [ ! -f "opencv.zip" ]; then 
#wget https://github.com/opencv/opencv/archive/4.5.1.zip -O opencv.zip 
#unzip opencv.zip 
#mv opencv-4.5.1 opencv 
#cd opencv 
#mkdir -p build && cd build 
#cmake -GNinja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../../third_party_libs -DCMAKE_CXX_COMPILER=/usr/bin/g++-5 -DCMAKE_C_COMPILER=/usr/bin/gcc-5
#cmake --build . -j20 
#cd ..
#cd ..
#fi 

# mkdir -p build && cd build 
# cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/bin/g++-5 -DCMAKE_C_COMPILER=/usr/bin/gcc-5 ..
# cmake --build . -j20
