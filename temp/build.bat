@echo off

llc -filetype=obj -march=x86-64 -o llvmtest.obj llvmtest.ll 

link /nologo llvmtest.obj /entry:main /subsystem:console kernel32.lib