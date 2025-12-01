/**
 * @file ta_marking_b_101308579_101299663.cpp
 * 
 * @author Ajay Uppal 101326354
 * @author Paul Felfli 101299663
 * 
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <iomanip>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

struct SharedRegion {
    char rubricLines[5][16];
    char studentId[8];
    int activeExamIdx;
    int questionState[5];
};

// Small helper: sleep between [minUsec, maxUsec]
static void sleepRandomUsec(useconds_t minUsec, useconds_t maxUsec) {
    if (maxUsec < minUsec) maxUsec = minUsec;
    useconds_t delta = maxUsec - minUsec;
    useconds_t offset = (delta == 0) ? 0 : (rand() % (delta + 1));
    usleep(minUsec + offset);
}

// Bump the letter after the comma in a rubric line: "1, A" -> "1, B" etc.
static void bumpRubricLetter(char line[16]) {
    char *comma = strchr(line, ',');
    if (!comma) return;

    char *letter = comma + 2;
    if (*letter >= 'A' && *letter < 'Z') {
        (*letter)++;
    }
}

// Reload rubric.txt from shared memory content (overwrite file)
static void writeRubricToFile(const SharedRegion *shared) {
    ofstream out("rubric.txt");
    if (!out) {
        cerr << "[Parent/TA] WARNING: could not reopen rubric.txt for writing\n";
        return;
    }
    for (int i = 0; i < 5; ++i) {
        out << shared->rubricLines[i] << '\n';
    }
}

// Read a single student id from an exam file into dest
static bool loadStudentFromFile(const string &path, char dest[8]) {
    ifstream in(path);
    if (!in) {
        cerr << "[Loader] Failed to open exam file: " << path << '\n';
        return false;
    }
    string s;
    in >> s;
    if (s.empty()) {
        cerr << "[Loader] Empty student id in file: " << path << '\n';
        return false;
    }
    strncpy(dest, s.c_str(), 7);
    dest[7] = '\0';
    return true;
}

// Build list of exam files: exam_0001.txt ... exam_0019.txt and exam_9999.txt
static vector<string> buildExamList(const string &folder) {
    vector<string> exams;
    // Regular exams 0001..0019
    for (int i = 1; i <= 19; ++i) {
        ostringstream name;
        name << folder << "/exam_" << setw(4) << setfill('0') << i << ".txt";
        exams.push_back(name.str());
    }
    // Sentinel exam
    exams.push_back(folder + "/exam_9999.txt");
    return exams;
}

static void runTAProcess(int taId, SharedRegion *shared, const vector<string> &exams) {
    srand(static_cast<unsigned>(time(nullptr)) ^ static_cast<unsigned>(getpid()));

    cout << "[TA " << taId << "] process started (PID " << getpid() << ")\n";

    while (true) {
        int currentId = atoi(shared->studentId);
        if (currentId == 9999) {
            cout << "[TA " << taId << "] saw sentinel 9999, exiting\n";
            _exit(0);
        }

        cout << "[TA " << taId << "] Reviewing rubric for student "
             << shared->studentId << '\n';

        //Phase 1: possibly adjust rubric
        for (int q = 0; q < 5; ++q) {
            sleepRandomUsec(500000, 1000000);  

            bool willCorrect = (rand() % 4 == 0);  
            if (!willCorrect) continue;

            bumpRubricLetter(shared->rubricLines[q]);
            writeRubricToFile(shared);

            cout << "[TA " << taId << "] Updated rubric line "
                 << (q + 1) << " -> " << shared->rubricLines[q] << '\n';
        }

        // Check sentinel again before doing more work
        if (atoi(shared->studentId) == 9999) {
            cout << "[TA " << taId << "] sentinel reached after rubric phase, exiting\n";
            _exit(0);
        }

        //Phase 2: mark questions for the current student 
        for (int q = 0; q < 5; ++q) {
            if (shared->questionState[q] == 0) {
                cout << "[TA " << taId << "] Starting question "
                     << (q + 1) << " for student " << shared->studentId << '\n';

                shared->questionState[q] = 1;   

                sleepRandomUsec(1000000, 2000000);  

                cout << "[TA " << taId << "] Finished question "
                     << (q + 1) << " for student " << shared->studentId << '\n';
            }
        }

        //Phase 3: if all questions are done, try to load next exam
        bool allDone = true;
        for (int q = 0; q < 5; ++q) {
            if (shared->questionState[q] == 0) {
                allDone = false;
                break;
            }
        }

        if (allDone) {
            shared->activeExamIdx++;

            if (shared->activeExamIdx >= static_cast<int>(exams.size())) {
                strncpy(shared->studentId, "9999", 5);
                shared->studentId[5] = '\0';
                cout << "[TA " << taId << "] No more exams, setting sentinel and exiting\n";
                _exit(0);
            }

            if (!loadStudentFromFile(exams[shared->activeExamIdx], shared->studentId)) {
                strncpy(shared->studentId, "9999", 5);
                shared->studentId[5] = '\0';
                cout << "[TA " << taId << "] Failed to load next exam, forcing sentinel\n";
                _exit(0);
            }

            for (int q = 0; q < 5; ++q) {
                shared->questionState[q] = 0;
            }

            cout << "[TA " << taId << "] Loaded next exam index "
                 << shared->activeExamIdx
                 << " (student " << shared->studentId << ")\n";
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cerr << "Usage: ./ta_marking_a_101308579_101299663 <num_TAs> <exam_folder>\n";
        return 1;
    }

    int numTAs = atoi(argv[1]);
    if (numTAs < 2) {
        cerr << "Number of TAs must be at least 2\n";
        return 1;
    }
    string examFolder = argv[2];

    srand(static_cast<unsigned>(time(nullptr)));

    int shmid = shmget(IPC_PRIVATE, sizeof(SharedRegion), 0666 | IPC_CREAT);
    if (shmid < 0) {
        perror("shmget");
        return 1;
    }

    auto *shared = static_cast<SharedRegion *>(shmat(shmid, nullptr, 0));
    if (shared == reinterpret_cast<void *>(-1)) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, nullptr);
        return 1;
    }

    //Load rubric into shared memory 
    ifstream rubIn("rubric.txt");
    if (!rubIn) {
        cerr << "Cannot open rubric.txt\n";
        shmdt(shared);
        shmctl(shmid, IPC_RMID, nullptr);
        return 1;
    }
    for (int i = 0; i < 5; ++i) {
        rubIn.getline(shared->rubricLines[i], sizeof(shared->rubricLines[i]));
    }
    rubIn.close();

    //Prepare exam list and load first exam
    vector<string> exams = buildExamList(examFolder);
    if (exams.empty()) {
        cerr << "No exam files configured\n";
        shmdt(shared);
        shmctl(shmid, IPC_RMID, nullptr);
        return 1;
    }

    shared->activeExamIdx = 0;
    for (int q = 0; q < 5; ++q) {
        shared->questionState[q] = 0;
    }

    if (!loadStudentFromFile(exams[0], shared->studentId)) {
        shmdt(shared);
        shmctl(shmid, IPC_RMID, nullptr);
        return 1;
    }

    cout << "[Parent] Starting Part 2.a with " << numTAs
         << " TAs, first student " << shared->studentId << '\n';

    //Spawn TA processes 
    for (int i = 0; i < numTAs; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            for (int k = 0; k < i; ++k) {
                wait(nullptr);
            }
            shmdt(shared);
            shmctl(shmid, IPC_RMID, nullptr);
            return 1;
        }
        if (pid == 0) {
            runTAProcess(i, shared, exams);
            _exit(0);  
        }
    }

    //Parent waits for all TAs 
    for (int i = 0; i < numTAs; ++i) {
        wait(nullptr);
    }

    cout << "[Parent] All TAs finished. Cleaning up shared memory.\n";

    shmdt(shared);
    shmctl(shmid, IPC_RMID, nullptr);

    return 0;
}