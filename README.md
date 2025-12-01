# Assignment 3 Part 2
Concurrent Processes in Unix – TA Marking System  
Student IDs: 101308579, 101299663  
Repository: `SYSC4001_A3P2`

---

## Description

This project implements **Assignment 3 – Part 2** for SYSC 4001.  
It simulates multiple TAs marking exams concurrently under Unix, using:

- **Part 2.a** – processes + shared memory (no semaphores, intentional race conditions)
- **Part 2.b** – processes + shared memory + System V semaphores (proper critical-section handling)

Exams are stored in text files, a shared rubric is kept in shared memory, and the student number `9999` is used as a sentinel to signal the end of marking.

---

## Features

- Multiple TA processes (`num_TAs ≥ 2`)
- Shared rubric in shared memory  
- Shared “current exam” state in shared memory
- Automatic progression through a sequence of exam files
- Sentinel student **9999** to terminate all TAs
- Detailed console logging:
  - rubric corrections
  - marking start/finish per question
  - loading of the next exam
  - TA termination on sentinel

---

## File List

**Source code**

- `ta_marking_a_101308579_101299663.cpp`  
  Part 2.a – shared memory only, **no semaphores**.

- `ta_marking_b_101308579_101299663.cpp`  
  Part 2.b – shared memory + **System V semaphores**.

**Support files**

- `build.sh` – shell script to compile both programs.  
- `rubric.txt` – initial rubric (5 lines, one per question).  
- `exams/` – directory containing all exam input files.  
- `reportPartC.pdf` – short report for Part 2.c (deadlock / livelock / execution order).

---

## Assumptions

- The programs are run on **Linux** or **WSL** with a C++17 compiler and System V IPC support.
- All paths are resolved relative to the **repository root**:
  - `rubric.txt` in the root directory.
  - `exams/` subfolder containing all exam files.
- Exam files are named:

  ```text
  exam_0001.txt
  exam_0002.txt
  ...
  exam_0019.txt
  exam_9999.txt