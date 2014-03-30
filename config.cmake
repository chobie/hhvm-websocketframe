SET(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0")

HHVM_EXTENSION(websocketframe websocketframe.cpp)
HHVM_SYSTEMLIB(websocketframe websocketframe.php)
