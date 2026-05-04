#include <iostream>
#include <mpi.h>
#include <vector>   
#include <fstream>  

int ProcNum;
int ProcRank;

bool isPrime(int n) {
    if (n <= 1) return false;
    for (int i = 2; i < n; i++) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

int SerialPrimeCount(int maxRange, std::vector<int>& foundPrimes) {
    int count = 0;

    if (maxRange >= 2) {
        foundPrimes.push_back(2);
        count++;
    }

    for (int i = 3; i <= maxRange; i += 2) {
        if (isPrime(i)) {
            foundPrimes.push_back(i);
            count++;
        }
    }
    return count;
}

int ParallelPrimeCount(int maxRange, std::vector<int>& foundPrimes) {
    int localCount = 0;

    if (ProcRank == 0 && maxRange >= 2) {
        foundPrimes.push_back(2);
        localCount++;
    }

    for (int i = 3 + (ProcRank * 2); i <= maxRange; i += (ProcNum * 2)) {
        if (isPrime(i)) {
            foundPrimes.push_back(i);
            localCount++;
        }
    }
    return localCount;
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);

    int maxRange = 0;
    int totalPrimes = 0;
    int serialPrimes = 0;
    int localPrimes = 0;
    double Start, Finish, Duration;

    std::vector<int> myPrimes;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
    MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

    if (ProcRank == 0) {
        printf("Parallel Prime Number Search Program (Balanced)\n");
        do {
            printf("\nEnter the maximum range (e.g., 100000): ");
            scanf_s("%d", &maxRange);
            if (maxRange < 2) {
                printf("Range must be >= 2!\n");
            }
        } while (maxRange < 2);
    }

    MPI_Bcast(&maxRange, 1, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    Start = MPI_Wtime();

    localPrimes = ParallelPrimeCount(maxRange, myPrimes);
    MPI_Reduce(&localPrimes, &totalPrimes, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    for (int i = 0; i < ProcNum; i++) {
        if (ProcRank == i) {
            std::ofstream outFile;
            if (i == 0) {
                outFile.open("results_parallel.txt", std::ios::out);
            }
            else {
                outFile.open("results_parallel.txt", std::ios::app);
            }

            if (outFile.is_open()) {
                outFile << "Processor " << ProcRank << ": ";
                for (int prime : myPrimes) {
                    outFile << prime << " ";
                }
                outFile << "\n";
                outFile.close();
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    Finish = MPI_Wtime();
    Duration = Finish - Start;

    if (ProcRank == 0) {
        printf("\nParallel Execution Time: %f seconds\n", Duration);
        printf("Total primes found (Parallel): %d\n", totalPrimes);

        printf("\nRunning Serial Algorithm for comparison...\n");

        std::vector<int> serialPrimesList;

        double serialStart = MPI_Wtime();

        serialPrimes = SerialPrimeCount(maxRange, serialPrimesList);

        std::ofstream serialFile("results_serial.txt", std::ios::out);
        if (serialFile.is_open()) {
            serialFile << "Serial Algorithm Results:\n";
            for (int prime : serialPrimesList) {
                serialFile << prime << " ";
            }
            serialFile << "\n";
            serialFile.close();
        }

        double serialFinish = MPI_Wtime();
        double serialDuration = serialFinish - serialStart;

        printf("Serial Execution Time: %f seconds\n", serialDuration);
        printf("Total primes found (Serial): %d\n", serialPrimes);

        if (totalPrimes == serialPrimes) {
            printf("\nThe results are identical.\n");
            printf("Speedup: %f\n", serialDuration / Duration);
            printf("Files 'results_parallel.txt' and 'results_serial.txt' have been saved.\n");
        }
        else {
            printf("\nERROR: Results are NOT identical.\n");
        }
    }

    MPI_Finalize();
    return 0;
}