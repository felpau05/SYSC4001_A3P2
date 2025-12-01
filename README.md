# Assignment 3 – Part 2

Concurrent Processes in Unix – TA Marking System  
Student IDs: 101308579, 101299663  
Repository: `SYSC4001_A3P2`

This repository contains the solutions for Assignment 3 Part 2:

  * `ta_marking_a_101308579_101299663.cpp` — Part 2.a (shared memory only, intentional races)
  * `ta_marking_b_101308579_101299663.cpp` — Part 2.b (shared memory + System V semaphores)
  * `rubric.txt` — The initial 5-line rubric file
  * `exams/` — Folder containing all exam input files (e.g., `exam_0001.txt` to `exam_9999.txt`)
  * `build.sh` — Script to compile both programs

The programs are intended to run on **Linux/WSL** with System V IPC support.

-----

## 1\. Compiling

### Using `build.sh` (recommended)

From the repository root:

```bash
chmod +x build.sh     # only needed once
./build.sh
```

This produces the executables: `ta_marking_a` and `ta_marking_b`.

### Manual compilation

```bash
# Part 2.a
g++ -std=c++17 -Wall -Wextra -pedantic \
    ta_marking_a_101308579_101299663.cpp \
    -o ta_marking_a

# Part 2.b
g++ -std=c++17 -Wall -Wextra -pedantic \
    ta_marking_b_101308579_101299663.cpp \
    -o ta_marking_b
```

-----

## 2\. Running Test Cases

Both programs use the same command-line format:

```bash
./ta_marking_a <num_TAs> <exam_folder>
./ta_marking_b <num_TAs> <exam_folder>
```

Where:

  * `<num_TAs>` — Number of TA processes to fork (integer $\ge 2$).
  * `<exam_folder>` — Path to the exam directory (e.g., `exams`).

### Example Test Cases

From the repository root:

```bash
# Small test
./ta_marking_a 2 exams
./ta_marking_b 2 exams

# Medium test (used for most checks)
./ta_marking_a 3 exams
./ta_marking_b 3 exams

# Larger test
./ta_marking_a 5 exams
./ta_marking_b 5 exams
```

### Saving Outputs (for Part 2.c analysis)

To save the terminal output for later inspection and use in `reportPartC.pdf`:

```bash
./ta_marking_a 3 exams > run_a_3TAs.txt
./ta_marking_b 3 exams > run_b_3TAs.txt
```

-----

## 3\. Design Discussion

The assignment requires a discussion of how the design satisfies the three requirements of the critical section problem: **mutual exclusion**, **progress**, and **bounded waiting**.

### 3.1 Part 2.a — Shared Memory Only (No Semaphores)

`ta_marking_a_101308579_101299663.cpp` uses shared memory for all shared data (**student ID**, **exam index**, **question state**, and **rubric**) without any synchronization.

  * **Mutual exclusion:** **Not guaranteed.** Multiple TAs can read/write shared data at the same time, leading to race conditions (e.g., claiming the same question).
  * **Progress:** **Not strictly guaranteed.** Race conditions can lead to inconsistent state, though the system usually finishes in practice.
  * **Bounded waiting:** **Not guaranteed.** There is no mechanism to prevent a TA from being indefinitely starved of work.

This version intentionally demonstrates unsafe concurrent behavior.

### 3.2 Part 2.b — Shared Memory + Semaphores

`ta_marking_b_101308579_101299663.cpp` uses a System V semaphore set with two semaphores:

  * **Semaphore 0 (Exam State Lock):** Protects the current exam state (`studentId`, `activeExamIdx`, `questionState[]`).
  * **Semaphore 1 (Rubric Lock):** Protects rubric modifications and file writes (`rubricLines[]`, `rubric.txt`).

This design ensures:

  * **Mutual exclusion:** **Satisfied.** Only one TA can update exam state or the rubric at a time by using `P()` and `V()` operations around critical sections.
  * **Progress:** **Satisfied.** No circular wait is created (a TA holds at most one lock at a time, and critical sections are short). The system continues to advance.
  * **Bounded waiting:** **Satisfied.** The underlying System V semaphore implementation queues waiting processes, guaranteeing that a TA waiting for a lock will eventually acquire it.

Part 2.b satisfies the critical section requirements using semaphore-protected critical sections.

```
```