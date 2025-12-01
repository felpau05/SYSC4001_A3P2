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
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

struct SharedRegion {
    char rubricLines[5][16];
    char studentId[8];
    int  activeExamIdx;
    int  questionState[5];   
};

union SemArg {
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
};

static void sleepRandomUsec(useconds_t minUsec, useconds_t maxUsec) {
    if (maxUsec < minUsec) maxUsec = minUsec;
    useconds_t delta = maxUsec - minUsec;
    useconds_t offset = (delta == 0) ? 0 : (rand() % (delta + 1));
    usleep(minUsec + offset);
}

static void bumpRubricLetter(char line[16]) {
    char *comma = strchr(line, ',');
    if (!comma) return;
    char *letter = comma + 2;
    if (*letter >= 'A' && *letter < 'Z') {
        (*letter)++;
    }
}

static void writeRubricToFile(const SharedRegion *shared) {
    ofstream out("rubric.txt");
    if (!out) {
        cerr << "[TA] WARNING: could not reopen rubric.txt for writing\n";
        return;
    }
    for (int i = 0; i < 5; ++i) {
        out << shared->rubricLines[i] << '\n';
    }
}

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

static vector<string> buildExamList(const string &folder) {
    vector<string> exams;
    for (int i = 1; i <= 19; ++i) {
        ostringstream name;
        name << folder << "/exam_" << setw(4) << setfill('0') << i << ".txt";
        exams.push_back(name.str());
    }
    exams.push_back(folder + "/exam_9999.txt");
    return exams;
}

// Semaphore helpers
static void P(int semid, unsigned short idx) {
    struct sembuf op{};
    op.sem_num = idx;
    op.sem_op  = -1;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) == -1) {
        perror("semop P");
    }
}

static void V(int semid, unsigned short idx) {
    struct sembuf op{};
    op.sem_num = idx;
    op.sem_op  = 1;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) == -1) {
        perror("semop V");
    }
}

static void runTAProcess(int taId, SharedRegion *shared,
                         const vector<string> &exams, int semid) {
    srand(static_cast<unsigned>(time(nullptr)) ^ static_cast<unsigned>(getpid()));

    cout << "[TA " << taId << "] process started (PID " << getpid() << ")\n";

    while (true) {
        // Snapshot current student under exam lock 
        char localStudent[8];

        P(semid, 0);
        strncpy(localStudent, shared->studentId, sizeof(localStudent));
        localStudent[sizeof(localStudent) - 1] = '\0';
        int idNow = atoi(shared->studentId);
        V(semid, 0);

        if (idNow == 9999) {
            cout << "[TA " << taId << "] saw sentinel 9999, exiting\n";
            _exit(0);
        }

        cout << "[TA " << taId << "] Checking rubric for student "
             << localStudent << '\n';

        // Phase 1: rubric corrections, protected by rubric semaphore (1)
        for (int q = 0; q < 5; ++q) {
            sleepRandomUsec(500000, 1000000);  // 0.5–1s

            bool willCorrect = (rand() % 4 == 0);
            if (!willCorrect) continue;

            P(semid, 1);  // lock rubric
            if (atoi(shared->studentId) == 9999) {
                V(semid, 1);
                cout << "[TA " << taId << "] sentinel reached while correcting rubric, exiting\n";
                _exit(0);
            }

            bumpRubricLetter(shared->rubricLines[q]);
            writeRubricToFile(shared);

            cout << "[TA " << taId << "] Corrected rubric line "
                 << (q + 1) << " -> " << shared->rubricLines[q] << '\n';

            V(semid, 1);
        }

        //Phase 2: claim questions one by one under exam lockk
        while (true) {
            int qIndex = -1;
            char studentForQuestion[8];

            P(semid, 0);
            int currentId = atoi(shared->studentId);
            if (currentId == 9999) {
                V(semid, 0);
                cout << "[TA " << taId << "] sentinel reached before marking, exiting\n";
                _exit(0);
            }

            strncpy(studentForQuestion, shared->studentId,
                    sizeof(studentForQuestion));
            studentForQuestion[sizeof(studentForQuestion) - 1] = '\0';

            for (int q = 0; q < 5; ++q) {
                if (shared->questionState[q] == 0) {
                    shared->questionState[q] = 1;  
                    qIndex = q;
                    break;
                }
            }
            V(semid, 0);

            if (qIndex == -1) {
                break;
            }

            cout << "[TA " << taId << "] Marking question "
                 << (qIndex + 1) << " for student "
                 << studentForQuestion << '\n';

            sleepRandomUsec(1000000, 2000000); 

            cout << "[TA " << taId << "] Finished question "
                 << (qIndex + 1) << " for student "
                 << studentForQuestion << '\n';
        }

        //Phasee 3: possibly l0ad next exam (single TA at a time) ---
        P(semid, 0);

        if (atoi(shared->studentId) == 9999) {
            V(semid, 0);
            cout << "[TA " << taId << "] sentinel reached while checking next exam, exiting\n";
            _exit(0);
        }

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
                for (int q = 0; q < 5; ++q) {
                    shared->questionState[q] = 1;
                }
                cout << "[TA " << taId << "] No more exams. Sentinel set.\n";
                V(semid, 0);
                _exit(0);
            }

            if (!loadStudentFromFile(exams[shared->activeExamIdx],
                                      shared->studentId)) {
                strncpy(shared->studentId, "9999", 5);
                shared->studentId[5] = '\0';
                for (int q = 0; q < 5; ++q) {
                    shared->questionState[q] = 1;
                }
                cout << "[TA " << taId << "] Failed to load next exam. Sentinel set.\n";
                V(semid, 0);
                _exit(0);
            }

            for (int q = 0; q < 5; ++q) {
                shared->questionState[q] = 0;
            }

            cout << "[TA " << taId << "] Loaded next exam index "
                 << shared->activeExamIdx
                 << " (student " << shared->studentId << ")\n";
        }

        V(semid, 0);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cerr << "Usage: ./ta_marking_b_101308579_101299663 <num_TAs> <exam_folder>\n";
        return 1;
    }

    int numTAs = atoi(argv[1]);
    if (numTAs < 2) {
        cerr << "Number of TAs must be at least 2\n";
        return 1;
    }
    string examFolder = argv[2];

    srand(static_cast<unsigned>(time(nullptr)));

    //Shared memmory setup 
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

    //Load rubric
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

    //Exam list and first exam 
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

    //Create semaphore set: 2 semaphores
    int semid = semget(IPC_PRIVATE, 2, 0666 | IPC_CREAT);
    if (semid < 0) {
        perror("semget");
        shmdt(shared);
        shmctl(shmid, IPC_RMID, nullptr);
        return 1;
    }

    SemArg arg{};
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1 ||
        semctl(semid, 1, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        shmdt(shared);
        shmctl(shmid, IPC_RMID, nullptr);
        semctl(semid, 0, IPC_RMID);
        return 1;
    }

    cout << "[Parent] Starting Part 2.b with " << numTAs
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
            semctl(semid, 0, IPC_RMID);
            return 1;
        }
        if (pid == 0) {
            runTAProcess(i, shared, exams, semid);
            _exit(0);
        }
    }

    //Parent: wait for all TAs 
    for (int i = 0; i < numTAs; ++i) {
        wait(nullptr);
    }

    cout << "[Parent] All TAs finished. Cleaning up shared memory and semaphores.\n";

    shmdt(shared);
    shmctl(shmid, IPC_RMID, nullptr);
    semctl(semid, 0, IPC_RMID);

    return 0;
}