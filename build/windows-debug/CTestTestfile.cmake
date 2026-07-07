# CMake generated Testfile for 
# Source directory: D:/GITHUB/ShareAudioPC_2
# Build directory: D:/GITHUB/ShareAudioPC_2/build/windows-debug
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[shareaudio_tests]=] "D:/GITHUB/ShareAudioPC_2/build/windows-debug/shareaudio_tests.exe")
set_tests_properties([=[shareaudio_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/GITHUB/ShareAudioPC_2/CMakeLists.txt;123;add_test;D:/GITHUB/ShareAudioPC_2/CMakeLists.txt;0;")
subdirs("_deps/opus-build")
