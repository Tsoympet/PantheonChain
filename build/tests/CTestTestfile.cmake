# CMake generated Testfile for 
# Source directory: /home/runner/work/PantheonChain/PantheonChain/tests
# Build directory: /home/runner/work/PantheonChain/PantheonChain/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(integration "/home/runner/work/PantheonChain/PantheonChain/build/tests/test_integration")
set_tests_properties(integration PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/PantheonChain/PantheonChain/tests/CMakeLists.txt;24;add_test;/home/runner/work/PantheonChain/PantheonChain/tests/CMakeLists.txt;0;")
add_test(integration_automated "/home/runner/work/PantheonChain/PantheonChain/build/tests/test_integration_automated")
set_tests_properties(integration_automated PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/PantheonChain/PantheonChain/tests/CMakeLists.txt;39;add_test;/home/runner/work/PantheonChain/PantheonChain/tests/CMakeLists.txt;0;")
add_test(consensus "/home/runner/work/PantheonChain/PantheonChain/build/tests/test_consensus")
set_tests_properties(consensus PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/PantheonChain/PantheonChain/tests/CMakeLists.txt;47;add_test;/home/runner/work/PantheonChain/PantheonChain/tests/CMakeLists.txt;0;")
subdirs("unit")
