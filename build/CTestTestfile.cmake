# CMake generated Testfile for 
# Source directory: /home/viktoria/oop/Pacman
# Build directory: /home/viktoria/oop/Pacman/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[CoreTests]=] "/home/viktoria/oop/Pacman/build/pacman_tests")
set_tests_properties([=[CoreTests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/viktoria/oop/Pacman/CMakeLists.txt;84;add_test;/home/viktoria/oop/Pacman/CMakeLists.txt;0;")
subdirs("_deps/doctest-build")
