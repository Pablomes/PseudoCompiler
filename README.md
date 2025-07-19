# PseudoCompiler
 
A small compiler and virtual machine for a subset of the Cambridge International Computer Science Pseudocode language written in C. Contains all basic functionalities.

All source files are included in the repo, including a bin folder containing three Windows executables.

Ideally to use, download the executables and set their location as a PATH system variable for use anywhere on the machine.
If set as system variable, open terminal in workspace folder and use any of the following commands.

pseudor <file path> : Compiles and runs the program, discarding its bytecode.
pseudoc <file path> <target name> : Compiles the program source into .pcbc bytecode.
pseudo <file path> : Executes a .pcbc bytecode file.
