# JarvisScript
## About the compiler
* Compiles 'Jarvis' programming language into 86x_64 NASM fully AOT (Ahead of time)
* Can output just the executable or the compiled assembly along with the execuatable
* It only properly supports Windows NASM output. I don't know if it works on any other operating system (most likely not), however I do plan on making it compile for Linux aswell

## Project
* All actual compilation files are found in Compiler/jc
* Code I last tested is in 'source.jarv'
* The compiled assembly from the code should be in 'output.nasm' but i might forget to update this
## Features
* Variable Declaration
* Variable Reassignment
* If, For and While keywords
* Methods
* (WIP) Manual Memory Management

## To Do (higher means more priority)
* 'delete' keyword support
* Array indexing with []
* Casting (auto and explicitly)
* Classes (just with variables)
* AND/OR (&& and ||) support for boolean expressions
* Standard Library
