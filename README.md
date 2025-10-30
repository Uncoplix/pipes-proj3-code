# Inter-process Communication Using Pipes
Alex Chen and Blane Herndon <br>
University of Minnesota <br>
CSCI4061: Operating Systems

## Description
The ability to launch several processes and establish communication between them has many applications in systems programming and beyond. This project will explore two such applications: (1) parallel file processing, and (2) sequential program I/O pipelining.

## Systems Programming Topics
The purpose of this project is to familiarize ourselves with systems calls for inter-process communication. Topics include:
- Text file input and parsing
- Creating pipes with the `pipe()` system call
- Using pipe file descriptors in combination with `read()` and `write()` to coordinate among several processes
- Executing program pipelines using `pipe()` and `dup2()` along with `exec()` and `fork()`
