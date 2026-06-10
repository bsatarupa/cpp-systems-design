#include <algorithm>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;
using Matrix = vector<vector<double>>;

void multiplyRange(const Matrix &A, const Matrix &B, Matrix &C, int rBegin,
                   int rEnd) {

  int colA = A[0].size(), colB = B[0].size(); // colA == rowB
  for (int i = rBegin; i <= rEnd; i++) {      // row_range
    for (int j = 0; j < colB; j++) {          // colB
      for (int k = 0; k < colA; k++) {        // colA
        C[i][j] += A[i][k] * B[k][j];
      }
    }
  }
}

Matrix ParallelMatrixMultiplication(const Matrix &A, const Matrix &B,
                                    int num_threads) {

  // dividing into non-overlapping row-range
  int rowA = A.size(), colB = B[0].size();
  Matrix C(rowA, vector<double>(colB, 0.0));

  num_threads = max(num_threads, 1); // there should be atleast 1 thread
  int rows_per_thread =
      (rowA + num_threads - 1) / num_threads; // VVIMP, to round up row-range

  vector<thread> thread_pool;

  for (int t = 0; t < num_threads; t++) {

    int rBegin = t * rows_per_thread;
    if (rBegin >= rowA)
      break;

    int rEnd = min(rowA - 1,
                   rBegin + rows_per_thread -
                       1); // considering the last non-full range

    // assigning jobs to threads
    thread_pool.emplace_back(multiplyRange, cref(A), cref(B), ref(C), rBegin,
                             rEnd);
  }

  for (auto &t : thread_pool)
    t.join();

  return C;
}

void print_matrix(const Matrix &A) {

  for (const auto &row : A) {

    for (const double &value : row)
      cout << value << " ";

    cout << endl;
  }
}

int main() {

  int const N = 3, M = 2;
  Matrix A(N, vector<double>(M, 1.0));
  Matrix B(M, vector<double>(N, 2.0));

  int num_threads = thread::hardware_concurrency(); // #CPU core
  Matrix C = ParallelMatrixMultiplication(A, B, num_threads);
  print_matrix(C);

  return 0;
}
/*
4 4 4
4 4 4
4 4 4
*/
