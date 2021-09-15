# CMake generated Testfile for 
# Source directory: /home/joseph/TTG/ttg/examples
# Build directory: /home/joseph/TTG/ttg/build/examples
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(ttg/test/uts-parsec/build "/home/joseph/.local/lib/python3.8/site-packages/cmake/data/bin/cmake" "--build" "/home/joseph/TTG/ttg/build" "--target" "uts-parsec")
set_tests_properties(ttg/test/uts-parsec/build PROPERTIES  FIXTURES_SETUP "TTG_TEST_uts-parsec_FIXTURE" _BACKTRACE_TRIPLES "/home/joseph/TTG/ttg/cmake/modules/AddTTGTestExecutable.cmake;14;add_test;/home/joseph/TTG/ttg/cmake/modules/AddTTGExecutable.cmake;85;add_ttg_test_executable;/home/joseph/TTG/ttg/examples/CMakeLists.txt;57;add_ttg_executable;/home/joseph/TTG/ttg/examples/CMakeLists.txt;0;")
add_test(ttg/test/uts-parsec/run-np-1 "/usr/local/bin/mpiexec" "-n" "1" "/home/joseph/TTG/ttg/build/examples/uts-parsec")
set_tests_properties(ttg/test/uts-parsec/run-np-1 PROPERTIES  ENVIRONMENT "MAD_NUM_THREADS=2" FIXTURES_REQUIRED "TTG_TEST_uts-parsec_FIXTURE" WORKING_DIRECTORY "/home/joseph/TTG/ttg/build" _BACKTRACE_TRIPLES "/home/joseph/TTG/ttg/cmake/modules/AddTTGTestExecutable.cmake;23;add_test;/home/joseph/TTG/ttg/cmake/modules/AddTTGExecutable.cmake;85;add_ttg_test_executable;/home/joseph/TTG/ttg/examples/CMakeLists.txt;57;add_ttg_executable;/home/joseph/TTG/ttg/examples/CMakeLists.txt;0;")
add_test(ttg/test/uts-parsec/run-np-2 "/usr/local/bin/mpiexec" "-n" "2" "/home/joseph/TTG/ttg/build/examples/uts-parsec")
set_tests_properties(ttg/test/uts-parsec/run-np-2 PROPERTIES  ENVIRONMENT "MAD_NUM_THREADS=2" FIXTURES_REQUIRED "TTG_TEST_uts-parsec_FIXTURE" WORKING_DIRECTORY "/home/joseph/TTG/ttg/build" _BACKTRACE_TRIPLES "/home/joseph/TTG/ttg/cmake/modules/AddTTGTestExecutable.cmake;23;add_test;/home/joseph/TTG/ttg/cmake/modules/AddTTGExecutable.cmake;85;add_ttg_test_executable;/home/joseph/TTG/ttg/examples/CMakeLists.txt;57;add_ttg_executable;/home/joseph/TTG/ttg/examples/CMakeLists.txt;0;")
