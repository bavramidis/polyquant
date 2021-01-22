#ifndef POLYQUANT_DETSET_H
#define POLYQUANT_DETSET_H
#include "basis/basis.hpp"
#include "integral/integral.hpp"
#include "io/io.hpp"
#include "molecule/molecule.hpp"
#include "molecule/quantum_particles.hpp"
#include <bit>
#include <bitset>
#include <combinations.hpp>
#include <inttypes.h>
#include <iostream>
#include <string>
#include <unordered_set>

namespace polyquant {

// https://stackoverflow.com/a/29855973
template <typename T> struct VectorHash {
  size_t operator()(const std::vector<T> &v) const {
    std::hash<T> hasher;
    size_t seed = 0;
    for (T i : v) {
      seed ^= hasher(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

template <typename T> class POLYQUANT_DETSET {
public:
  POLYQUANT_DETSET() = default;
  int det_idx(std::vector<int> idx);

  int num_excitation(std::vector<T> &Di, std::vector<T> &Dj) const;
  void create_det(std::vector<std::vector<std::vector<int>>> &occ);
  std::vector<std::vector<T>> create_excitation(std::vector<T> &det,
                                                int excitation_level);

  void get_holes(std::vector<T> &Di, std::vector<T> &Dj,
                 std::vector<int> &holes) const;
  void get_parts(std::vector<T> &Di, std::vector<T> &Dj,
                 std::vector<int> &parts) const;
  double get_phase(std::vector<T> &Di, std::vector<T> &Dj,
                   std::vector<int> &holes, std::vector<int> &parts) const;
  void get_occ_virt(std::vector<T> &D, std::vector<int> &occ,
                    std::vector<int> &virt) const;
  double same_part_ham_diag(int idx_part,
                            std::vector<std::vector<int>> i_unfold,
                            std::vector<std::vector<int>> j_unfold) const;
  double same_part_ham_single(int idx_part,
                              std::vector<std::vector<int>> i_unfold,
                              std::vector<std::vector<int>> j_unfold) const;
  double same_part_ham_double(int idx_part,
                              std::vector<std::vector<int>> i_unfold,
                              std::vector<std::vector<int>> j_unfold) const;

  std::vector<T> get_det(int idx_part, int idx_spin, int i) const;
  void print_determinants();
  /**
   * @brief determinant set (number of quantum particles, alpha/beta, det num
   * bitstrings)
   *
   */
  std::vector<std::vector<std::unordered_set<std::vector<T>, VectorHash<T>>>>
      dets;
  int max_orb;
  POLYQUANT_INTEGRAL input_integral;

  void set_integral(POLYQUANT_INTEGRAL &integral) {
    this->input_integral = integral;
  };

  double Slater_Condon(int i_det, int j_det) const;
  // for diagonalization stuff
  using Scalar = double; // A typedef named "Scalar" is required
  int rows() const {
    const int rows = this->N_dets;
    return rows;
  }
  int cols() const {
    const int cols = this->N_dets;
    return cols;
  }
  int N_dets;
  // y_out = M * x_in
  void perform_op(const double *x_in, double *y_out) const;
  std::vector<std::vector<int>> det_idx_unfold(std::size_t det_idx) const;
};

template <typename T>
int POLYQUANT_DETSET<T>::num_excitation(std::vector<T> &Di,
                                        std::vector<T> &Dj) const {
  int excitation_degree = 0;
  for (auto i = 0; i < Di.size(); i++) {
    excitation_degree += std::popcount(Di[i] ^ Dj[i]);
  }
  return excitation_degree / 2;
}
template <typename T>
void POLYQUANT_DETSET<T>::create_det(
    std::vector<std::vector<std::vector<int>>> &occ) {
  dets.resize(occ.size());
  for (auto i_part = 0; i_part < occ.size(); i_part++) {
    dets[i_part].resize(occ[i_part].size());
    for (auto i_spin = 0; i_spin < occ[i_part].size(); i_spin++) {
      std::vector<T> det;
      std::string bit_string;
      bit_string.resize(max_orb, '0');
      for (auto i_occ : occ[i_part][i_spin]) {
        bit_string[i_occ] = '1';
      }
      Polyquant_cout("Creating det " + bit_string);
      std::reverse(bit_string.begin(), bit_string.end());
      for (auto i = 0ul; i < bit_string.length(); i += 64) {
        det.push_back(std::stoull(bit_string.substr(i, 64), 0, 2));
      }
      dets[i_part][i_spin].insert(det);
    }
  }
}
template <typename T>
std::vector<std::vector<T>>
POLYQUANT_DETSET<T>::create_excitation(std::vector<T> &det,
                                       int excitation_level) {
  std::vector<std::vector<T>> created_dets;
  std::vector<int> occ, virt;
  this->get_occ_virt(det, occ, virt);

  if (excitation_level > virt.size()) {
    APP_ABORT("Excitation level exceeds virtual size!");
  }
  for (auto &&iocc : iter::combinations(occ, excitation_level)) {
    for (auto &&ivirt : iter::combinations(virt, excitation_level)) {
      std::vector<T> temp_det(det);
      // https://stackoverflow.com/a/47990
      for (auto &occbit : iocc) {
        temp_det[occbit / 64ul] &= ~(1UL << (occbit % 64ul));
      }
      for (auto &virtbit : ivirt) {
        temp_det[virtbit / 64ul] |= 1UL << (virtbit % 64ul);
      }
      created_dets.push_back(temp_det);
    }
  }
  return created_dets;
}

template <typename T>
void POLYQUANT_DETSET<T>::get_holes(std::vector<T> &Di, std::vector<T> &Dj,
                                    std::vector<int> &holes) const {
  for (auto i = 0; i < Di.size(); i++) {
    auto H = (Di[i] ^ Dj[i]) & Di[i];
    while (H != 0) {
      auto position = std::countr_zero(H);
      holes.push_back((64 * i) + position);
      H &= ~(1UL << position);
    }
  }
}
template <typename T>
void POLYQUANT_DETSET<T>::get_parts(std::vector<T> &Di, std::vector<T> &Dj,
                                    std::vector<int> &parts) const {
  for (auto i = 0; i < Di.size(); i++) {
    auto P = (Di[i] ^ Dj[i]) & Dj[i];
    while (P != 0) {
      auto position = std::countr_zero(P);
      parts.push_back((64 * i) + position);
      P &= ~(1UL << position);
    }
  }
}
template <typename T>
double POLYQUANT_DETSET<T>::get_phase(std::vector<T> &Di, std::vector<T> &Dj,
                                      std::vector<int> &holes,
                                      std::vector<int> &parts) const {
  T nperm = 0;
  std::vector<T> mask;
  for (auto i = 0; i < holes.size(); i++) {
    T high = std::max(parts[i], holes[i]);
    T low = std::min(parts[i], holes[i]);
    T k = high / 64;
    T m = high % 64;
    T j = low / 64;
    T n = low % 64;
    for (auto l = j; l < k; l++) {
      mask[l] = ~(0ul);
    }
    mask[k] = (1UL << m) - 1;
    mask[j] = mask[j] & (~(1ul << (n + 1) + 1));
    for (auto l = j; j < k; l++) {
      nperm = nperm + std::popcount(Di[j] & mask[l]);
    }
  }
  if ((holes.size() == 2) && (holes[1] < parts[0] || holes[0] > parts[1])) {
    nperm++;
  }
  return std::pow(-1.0, nperm);
}

template <typename T>
void POLYQUANT_DETSET<T>::get_occ_virt(std::vector<T> &D, std::vector<int> &occ,
                                       std::vector<int> &virt) const {
  for (auto offset = 0; offset < D.size(); offset++) {
    for (auto i = 0; i < sizeof(T) * 8; i++) {
      if (offset * 64 + i > max_orb) {
        break;
      }
      auto bit = (D[offset] >> i) & 1U;
      if (bit == 1) {
        occ.push_back(i + (offset * 64));
      } else {
        virt.push_back(i + (offset * 64));
      }
    }
  }
}
template <typename T> void POLYQUANT_DETSET<T>::print_determinants() {
  Polyquant_cout("Printing Determinants");
  for (auto i_part = 0; i_part < dets.size(); i_part++) {
    Polyquant_cout("Particle " + std::to_string(i_part));
    for (auto i_spin = 0; i_spin < dets[i_part].size(); i_spin++) {
      Polyquant_cout("spin " + std::to_string(i_spin));
      auto idet_idx = 0;
      for (auto i_det : dets[i_part][i_spin]) {
        std::stringstream ss;
        ss << std::setw(10) << idet_idx;
        ss << "    ";
        std::string det;
        for (auto i_detframe : i_det) {
          det += std::bitset<64>(i_detframe).to_string();
          det += " ";
        }
        std::reverse(det.begin(), det.end());
        ss << det;
        ss << "    ";
        Polyquant_cout(ss.str());
        idet_idx++;
      }
    }
  }

  Polyquant_cout("Det num to idx");
  for (auto i = 0ul; i < this->N_dets; i++) {
    auto idxs = this->det_idx_unfold(i);
    std::stringstream ss;
    ss << " Det idx:" << i << "     ";
    for (auto j : idxs) {
      for (auto k : j) {
        ss << k << " ";
      }
    }
    Polyquant_cout(ss.str());
  }
}

template <typename T>
std::vector<std::vector<int>>
POLYQUANT_DETSET<T>::det_idx_unfold(std::size_t det_idx) const {
  std::vector<std::vector<int>> unfolded_idx;
  auto running_size = 1ul;
  for (auto i_part = dets.size(); i_part > 0; i_part--) {
    std::vector<int> tmp_vector;
    for (auto i_spin = dets[i_part - 1].size(); i_spin > 0; i_spin--) {
      running_size *= dets[i_part - 1][i_spin - 1].size();
      tmp_vector.push_back(det_idx % running_size);
      det_idx /= running_size;
    }
    std::reverse(tmp_vector.begin(), tmp_vector.end());
    unfolded_idx.push_back(tmp_vector);
  }
  std::reverse(unfolded_idx.begin(), unfolded_idx.end());
  return unfolded_idx;
}

template <typename T>
std::vector<T> POLYQUANT_DETSET<T>::get_det(int idx_part, int idx_spin,
                                            int i) const {
  auto it = this->dets[idx_part][idx_spin].begin();
  std::advance(it, i);
  return *it;
}

template <typename T>
double POLYQUANT_DETSET<T>::same_part_ham_diag(
    int idx_part, std::vector<std::vector<int>> i_unfold,
    std::vector<std::vector<int>> j_unfold) const {
  auto alpha_spin_idx = 0;
  auto beta_spin_idx = 1 % this->dets[idx_part].size();

  std::vector<int> aocc, avirt;
  auto det_i_a = this->get_det(idx_part, alpha_spin_idx,
                               i_unfold[idx_part][alpha_spin_idx]);
  this->get_occ_virt(det_i_a, aocc, avirt);

  std::vector<int> bocc, bvirt;

  if (beta_spin_idx == 1) {
    auto det_i_b = this->get_det(idx_part, beta_spin_idx,
                                 i_unfold[idx_part][beta_spin_idx]);
    this->get_occ_virt(det_i_b, bocc, bvirt);
  } else {
    bocc = aocc;
    bvirt = avirt;
  }
  double elem = 0.0;
  for (auto orb_a_i : aocc) {
    elem += this->input_integral.mo_one_body_ints[idx_part][alpha_spin_idx](
        orb_a_i, orb_a_i);
  }
  for (auto orb_b_i : bocc) {
    elem += this->input_integral.mo_one_body_ints[idx_part][beta_spin_idx](
        orb_b_i, orb_b_i);
  }
  for (auto orb_a_i : aocc) {
    for (auto orb_a_j : aocc) {
      elem += 0.5 *
              (this->input_integral
                   .mo_two_body_ints[idx_part][alpha_spin_idx][idx_part]
                                    [alpha_spin_idx][this->input_integral.idx8(
                                        orb_a_i, orb_a_i, orb_a_j, orb_a_j)]);
      elem -= 0.5 *
              (this->input_integral
                   .mo_two_body_ints[idx_part][alpha_spin_idx][idx_part]
                                    [alpha_spin_idx][this->input_integral.idx8(
                                        orb_a_i, orb_a_j, orb_a_j, orb_a_i)]);
    }
    for (auto orb_b_j : bocc) {
      elem += this->input_integral
                  .mo_two_body_ints[idx_part][alpha_spin_idx][idx_part]
                                   [beta_spin_idx][this->input_integral.idx8(
                                       orb_a_i, orb_a_i, orb_b_j, orb_b_j)];
    }
  }
  for (auto orb_b_i : bocc) {
    for (auto orb_b_j : bocc) {
      elem +=
          0.5 * (this->input_integral
                     .mo_two_body_ints[idx_part][beta_spin_idx][idx_part]
                                      [beta_spin_idx][this->input_integral.idx8(
                                          orb_b_i, orb_b_i, orb_b_j, orb_b_j)]);
      elem -=
          0.5 * (this->input_integral
                     .mo_two_body_ints[idx_part][beta_spin_idx][idx_part]
                                      [beta_spin_idx][this->input_integral.idx8(
                                          orb_b_i, orb_b_j, orb_b_j, orb_b_i)]);
    }
  }
  return elem;
}

template <typename T>
double POLYQUANT_DETSET<T>::same_part_ham_single(
    int idx_part, std::vector<std::vector<int>> i_unfold,
    std::vector<std::vector<int>> j_unfold) const {
  auto elem = 0.0;
  auto alpha_spin_idx = 0;
  auto beta_spin_idx = 1 % this->dets[idx_part].size();
  auto alpha_det_i_idx = i_unfold[idx_part][alpha_spin_idx];
  auto beta_det_i_idx = i_unfold[idx_part][beta_spin_idx];
  auto alpha_det_j_idx = j_unfold[idx_part][alpha_spin_idx];
  auto beta_det_j_idx = j_unfold[idx_part][beta_spin_idx];
  auto det_i_a = this->get_det(idx_part, alpha_spin_idx, alpha_det_i_idx);
  auto det_i_b = this->get_det(idx_part, beta_spin_idx, beta_det_i_idx);
  auto det_j_a = this->get_det(idx_part, alpha_spin_idx, alpha_det_j_idx);
  auto det_j_b = this->get_det(idx_part, beta_spin_idx, beta_det_j_idx);

  // spin = 0 alpha excitation, spin = 1 beta excitation
  auto spin = 0;
  if (alpha_det_i_idx == alpha_det_j_idx) {
    spin = 1;
  }

  std::vector<int> aocc, avirt;
  std::vector<int> bocc, bvirt;
  this->get_occ_virt(det_i_a, aocc, avirt);
  if (beta_spin_idx == 1) {
    this->get_occ_virt(det_i_b, bocc, bvirt);
  } else {
    bocc = aocc;
    bvirt = avirt;
  }

  // get hole
  // get part
  std::vector<int> holes, parts;
  double phase = 1.0;
  if (spin = 0) {
    get_holes(det_i_a, det_j_a, holes);
    get_parts(det_i_a, det_j_a, parts);
    phase = get_phase(det_i_a, det_j_a, holes, parts);
    elem += this->input_integral.mo_one_body_ints[idx_part][alpha_spin_idx](
        parts[0], holes[0]);
    for (auto orb_a_i : aocc) {
      elem += this->input_integral
                  .mo_two_body_ints[idx_part][alpha_spin_idx][idx_part]
                                   [alpha_spin_idx][this->input_integral.idx8(
                                       parts[0], holes[0], orb_a_i, orb_a_i)];
      elem -= this->input_integral
                  .mo_two_body_ints[idx_part][alpha_spin_idx][idx_part]
                                   [alpha_spin_idx][this->input_integral.idx8(
                                       parts[0], orb_a_i, orb_a_i, holes[0])];
    }
    for (auto orb_b_i : bocc) {
      elem += this->input_integral
                  .mo_two_body_ints[idx_part][beta_spin_idx][idx_part]
                                   [beta_spin_idx][this->input_integral.idx8(
                                       parts[0], holes[0], orb_b_i, orb_b_i)];
    }
  } else {
    get_holes(det_i_b, det_j_b, holes);
    get_parts(det_i_b, det_j_b, parts);
    phase = get_phase(det_i_b, det_j_b, holes, parts);
    elem += this->input_integral.mo_one_body_ints[idx_part][beta_spin_idx](
        parts[0], holes[0]);
    for (auto orb_b_i : bocc) {
      elem += this->input_integral
                  .mo_two_body_ints[idx_part][beta_spin_idx][idx_part]
                                   [beta_spin_idx][this->input_integral.idx8(
                                       parts[0], holes[0], orb_b_i, orb_b_i)];
      elem -= this->input_integral
                  .mo_two_body_ints[idx_part][beta_spin_idx][idx_part]
                                   [beta_spin_idx][this->input_integral.idx8(
                                       parts[0], orb_b_i, orb_b_i, holes[0])];
    }
    for (auto orb_a_i : aocc) {
      elem += this->input_integral
                  .mo_two_body_ints[idx_part][alpha_spin_idx][idx_part]
                                   [alpha_spin_idx][this->input_integral.idx8(
                                       parts[0], holes[0], orb_a_i, orb_a_i)];
    }
  }
  elem *= phase;
  return elem;
}

template <typename T>
double POLYQUANT_DETSET<T>::same_part_ham_double(
    int idx_part, std::vector<std::vector<int>> i_unfold,
    std::vector<std::vector<int>> j_unfold) const {
  auto elem = 0.0;
  auto alpha_spin_idx = 0;
  auto beta_spin_idx = 1 % this->dets[idx_part].size();
  auto alpha_det_i_idx = i_unfold[idx_part][alpha_spin_idx];
  auto beta_det_i_idx = i_unfold[idx_part][beta_spin_idx];
  auto alpha_det_j_idx = j_unfold[idx_part][alpha_spin_idx];
  auto beta_det_j_idx = j_unfold[idx_part][beta_spin_idx];
  auto det_i_a = this->get_det(idx_part, alpha_spin_idx, alpha_det_i_idx);
  auto det_i_b = this->get_det(idx_part, beta_spin_idx, beta_det_i_idx);
  auto det_j_a = this->get_det(idx_part, alpha_spin_idx, alpha_det_j_idx);
  auto det_j_b = this->get_det(idx_part, beta_spin_idx, beta_det_j_idx);

  // spin = -1 mixed, spin = 0 alpha excitation, spin = 1 beta excitation
  auto spin = -1;
  if (alpha_det_i_idx == alpha_det_j_idx) {
    std::vector<int> holes, parts;
    double phase = 1.0;
    get_holes(det_i_b, det_j_b, holes);
    get_parts(det_i_b, det_j_b, parts);
    phase = get_phase(det_i_b, det_j_b, holes, parts);
    elem += this->input_integral
                .mo_two_body_ints[idx_part][alpha_spin_idx][idx_part]
                                 [alpha_spin_idx][this->input_integral.idx8(
                                     parts[0], holes[0], parts[1], holes[1])];
    elem -= this->input_integral
                .mo_two_body_ints[idx_part][alpha_spin_idx][idx_part]
                                 [alpha_spin_idx][this->input_integral.idx8(
                                     parts[0], holes[1], parts[1], holes[0])];
    elem *= phase;
  } else if (beta_det_i_idx == beta_det_j_idx) {
    std::vector<int> holes, parts;
    double phase = 1.0;
    get_holes(det_i_a, det_j_a, holes);
    get_parts(det_i_a, det_j_a, parts);
    phase = get_phase(det_i_a, det_j_a, holes, parts);
    elem += this->input_integral
                .mo_two_body_ints[idx_part][beta_spin_idx][idx_part]
                                 [beta_spin_idx][this->input_integral.idx8(
                                     parts[0], holes[0], parts[1], holes[1])];
    elem -= this->input_integral
                .mo_two_body_ints[idx_part][beta_spin_idx][idx_part]
                                 [beta_spin_idx][this->input_integral.idx8(
                                     parts[0], holes[1], parts[1], holes[0])];
    elem *= phase;
  } else {
    std::vector<int> aholes, aparts;
    std::vector<int> bholes, bparts;
    double phase = 1.0;
    get_holes(det_i_a, det_j_a, aholes);
    get_parts(det_i_a, det_j_a, aparts);
    get_holes(det_i_b, det_j_b, bholes);
    get_parts(det_i_b, det_j_b, bparts);
    phase *= get_phase(det_i_a, det_j_a, aholes, aparts);
    phase *= get_phase(det_i_b, det_j_b, bholes, bparts);
    elem +=
        this->input_integral
            .mo_two_body_ints[idx_part][alpha_spin_idx][idx_part][beta_spin_idx]
                             [this->input_integral.idx8(aparts[0], aholes[0],
                                                        bparts[0], bholes[0])];
    elem *= phase;
  }
  return elem;
}

template <typename T>
double POLYQUANT_DETSET<T>::Slater_Condon(int i_det, int j_det) const {

  double matrix_elem = 0.0;
  auto i_unfold = det_idx_unfold(i_det);
  auto j_unfold = det_idx_unfold(j_det);
  std::vector<bool> iequalj;
  for (auto idx_part = 0; idx_part < dets.size(); idx_part++) {
    iequalj.push_back(true);
    for (auto idx_spin = 0; idx_spin < dets[idx_part].size(); idx_spin++) {
      iequalj[idx_part] = iequalj[idx_part] && (i_unfold[idx_part][idx_spin] ==
                                                j_unfold[idx_part][idx_spin]);
    }
  }
  for (auto idx_part = 0; idx_part < dets.size(); idx_part++) {
    std::vector<bool> iequalj_otherparts(iequalj.begin(), iequalj.end());
    iequalj_otherparts.erase(iequalj_otherparts.begin() + idx_part);
    if (std::find(iequalj_otherparts.begin(), iequalj_otherparts.end(),
                  false) == iequalj_otherparts.end()) {
      auto excitation_level = 0;
      for (auto idx_spin = 0; idx_spin < this->dets[idx_part].size();
           idx_spin++) {
        auto det_i =
            this->get_det(idx_part, idx_spin, i_unfold[idx_part][idx_spin]);
        auto det_j =
            this->get_det(idx_part, idx_spin, j_unfold[idx_part][idx_spin]);
        excitation_level += this->num_excitation(det_i, det_j);
      }

      if (excitation_level == 0) {
        // do 1+2 body
        matrix_elem += this->same_part_ham_diag(idx_part, i_unfold, j_unfold);
      } else if (excitation_level == 1) {
        // do 1+2 body
        matrix_elem += this->same_part_ham_single(idx_part, i_unfold, j_unfold);
      } else if (excitation_level == 2) {
        // do 2 body
        matrix_elem += this->same_part_ham_double(idx_part, i_unfold, j_unfold);
      }
    }
    // loop over other particles
    // if all other particle dets j,k,l etc are equal then add the particle i j
    // interaction
  }
  return matrix_elem;
}

template <typename T>
void POLYQUANT_DETSET<T>::perform_op(const double *x_in, double *y_out) const {
  Polyquant_cout("Performing M*v operation");
  for (auto i_det = 0; i_det < this->N_dets; i_det++) {
    auto matrix_elem = 0.0;
#pragma omp parallel for reduction(+ : matrix_elem)
    for (auto j_det = 0; j_det < this->N_dets; j_det++) {
      matrix_elem += x_in[j_det] * this->Slater_Condon(i_det, j_det);
    }
    y_out[i_det] = matrix_elem;
  }
  // auto det_idx = 0ul;
  // for (auto i_part = 0; i_part < dets.size(); i_part++) {
  //  for (auto i_spin = 0; i_spin < dets[i_part].size(); i_spin++) {
  //      dets[i_part][i_spin].size()
}
// template <typename T>
// std::vector<int> POLYQUANT_DETSET<T>::get_holes(std::vector<T> &Di,
//                                           std::vector<T> &Dj) {}
// template <typename T>
// std::vector<int> POLYQUANT_DETSET<T>::get_parts(std::vector<T> &Di,
//                                           std::vector<T> &Dj) {}
} // namespace polyquant
#endif
