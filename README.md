\# SYSC 4001 – Assignment 3, Part 2  

\*\*Concurrent Processes in Unix: TA Marking System\*\*  

Student IDs: 101308579, 101299663  

Repository: `SYSC4001\_A3P2`



---



\## 1. Overview



This repository implements \*\*Part 2\*\* of the assignment:



\- \*\*Part 2.a\*\* – Concurrent TA marking using \*\*processes and shared memory only\*\* (no semaphores).  

\- \*\*Part 2.b\*\* – Same marking system, but using \*\*processes, shared memory, and System V semaphores\*\* to enforce proper critical-section behaviour.



The system simulates several TAs grading a sequence of exams using a common rubric stored in shared memory. Exam data and rubric data are shared between all TA processes; the student number `9999` is used as a sentinel to signal that no more real exams remain, and all TAs should terminate.



---



\## 2. Repository contents



Root:



\- `ta\_marking\_a\_101308579\_101299663.cpp`  

&nbsp; Part 2.a implementation (shared memory only, no semaphores).



\- `ta\_marking\_b\_101308579\_101299663.cpp`  

&nbsp; Part 2.b implementation (shared memory + semaphores).



\- `build.sh`  

&nbsp; Convenience script to compile both programs.



\- `rubric.txt`  

&nbsp; Initial rubric file (5 lines, one per question).



\- `exams/`  

&nbsp; Directory containing all exam files used as input.



\- `reportPartC.pdf`  

&nbsp; Short report for Part 2.c (deadlock / livelock / execution order discussion).



(Additional temporary or log files are not required for submission.)



---



\## 3. Build instructions



These programs are intended to run on \*\*Linux\*\* (or WSL) with a C++17 compiler that supports System V IPC (shared memory and semaphores).



\### 3.1. Build using `build.sh` (recommended)



From the repository root:



```bash

./build.sh



