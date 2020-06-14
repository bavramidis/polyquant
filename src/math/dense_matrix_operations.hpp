#include <math/dense_matrix.hpp>
#include <math/dense_vector.hpp>

#include <cblas.h>
#include <lapacke.h>

#ifndef PYCI_DENSE_MATRIX_OPERATIONS_H
#define PYCI_DENSE_MATRIX_OPERATIONS_H
namespace selci {

int symmetric_matrix_triangular_idx(const int &i, const int &j) {
  if (i > j) {
    return ((i * (i + 1)) / 2) + j;
  } else {
    return ((j * (j + 1)) / 2) + i;
  }
}
/**
 * @brief define matrix operations
 *
 */
void eigenvalues_and_eigenvectors(DENSE_MATRIX<double> &input_matrix,
                                  DENSE_VECTOR<double> &eigenvalues,
                                  DENSE_MATRIX<double> &eigenvectors) {
  // assumes a symmetric matrix of doubles
  // todo check if symmetric
  DENSE_VECTOR<double> upper_triangle;
  auto shape = input_matrix.shape();
  auto n = shape.first;
  // resize and zero out eigenvalues and eigenvectors
  eigenvalues.resize(n);
  eigenvalues.fill(0.0);
  eigenvectors.resize(n, n);
  eigenvectors.fill(0.0);

  size_t upper_triangle_size = (n * (n + 1)) / 2;
  upper_triangle.resize(upper_triangle_size);
  for (auto i = 0; i < n; i++) {
    for (auto j = i; j < n; j++) {
      upper_triangle(symmetric_matrix_triangular_idx(i, j)) =
          input_matrix(i, j);
    }
  }
  // TODO make this an input parameter
  double tolerance = 1.0e-6;
  lapack_int *m;
  double *w;
  double *z;
  lapack_int ldz = n;
  lapack_int *isuppz;
  lapack_int info = LAPACKE_dsyevr(LAPACK_ROW_MAJOR, 'V', 'A', 'U', n,
                                   upper_triangle.get_data(), n, 0.0, 0.0, 0, 0,
                                   tolerance, m, w, z, ldz, isuppz);
  eigenvalues.resize(n);
  eigenvectors.resize(n, n);
  for (int i = 0; i < n; i++) {
    eigenvalues(i) = w[i];
    for (int j = i; j < n; j++) {
      eigenvectors(i, j) = z[symmetric_matrix_triangular_idx(i, j)];
      eigenvectors(j, i) = z[symmetric_matrix_triangular_idx(i, j)];
    }
  }
}
/**
 * @brief define matrix operations
 *
 */
void mm_dot(DENSE_MATRIX<double> &A, DENSE_MATRIX<double> &B,
            DENSE_MATRIX<double> &output, bool trans_A = false,
            bool trans_B = false) {
  // assumes a symmetric matrix of doubles
  auto TRANSA = trans_A ? CblasNoTrans : CblasNoTrans;
  auto TRANSB = trans_B ? CblasNoTrans : CblasNoTrans;

  auto A_shape = A.shape();
  auto B_shape = B.shape();

  int M = A_shape.first;
  int N = B_shape.first;
  int K = A_shape.second;

  int LDA = trans_A ? K : M;
  int LDB = trans_B ? K : N;

  double alpha = 1.0;
  double beta = 0.0;

  output.resize(M, N);
  output.fill(0.0);

  cblas_dgemm(CblasRowMajor, TRANSA, TRANSB, M, N, K, alpha, A.get_data(), LDA,
              B.get_data(), LDA, beta, output.get_data(), N);
} // namespace selci
#endif
