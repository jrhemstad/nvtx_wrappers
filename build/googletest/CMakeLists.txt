cmake_minimum_required(VERSION 3.12)

include(ExternalProject)

ExternalProject_Add(GoogleTest
                    GIT_REPOSITORY    https://github.com/google/googletest.git
                    GIT_TAG           release-1.8.0
                    SOURCE_DIR        "/home/jhemstad/RAPIDS/nvtx_wrappers/build/googletest/googletest"
                    BINARY_DIR        "/home/jhemstad/RAPIDS/nvtx_wrappers/build/googletest/build"
                    INSTALL_DIR		  "/home/jhemstad/RAPIDS/nvtx_wrappers/build/googletest/install"
                    CMAKE_ARGS         -DCMAKE_C_FLAGS=-D_GLIBCXX_USE_CXX11_ABI=1; -DCMAKE_CXX_FLAGS=-D_GLIBCXX_USE_CXX11_ABI=1 -DCMAKE_INSTALL_PREFIX=/home/jhemstad/RAPIDS/nvtx_wrappers/build/googletest/install)

