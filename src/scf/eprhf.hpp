#include <basis/basis.hpp>
#include <integral/integral.hpp>
#include <io/io.hpp>
#include <molecule/molecule.hpp>
#include <scf/rhf.hpp>
#include <scf/scf.hpp>
#include <string>

#ifndef PYCI_EPRHF_H
#define PYCI_EPRHF_H
namespace selci {

class PYCI_EPRHF : PYCI_SCF {
public:
  PYCI_EPRHF() = default;
  PYCI_EPRHF(const PYCI_INPUT &input_params,
             const PYCI_MOLECULE &input_molecule, const PYCI_BASIS &input_basis,
             const PYCI_INTEGRAL &input_integral)
      : PYCI_SCF(input_params, input_molecule, input_basis, input_integral){};

  void form_H_core() override;
  void form_fock() override;
  void diag_fock() override;
  void form_DM() override;
  void calculate_E_elec() override{};
  void calculate_E_total() override;
  void check_stop() override;
  void run_iteration() override;
  void guess_DM() override;
  void run() override;
  void set_electronic_system(PYCI_RHF &elec_RHF);
  void set_polarization_potential();
  void calculate_E_elec_plus_excess_particle();
  void set_relax_target();
  void set_excess_particle_type(std::string particle_type);

  PYCI_RHF elec_RHF;
  /**
   * @brief H_core matrix
   *
   */
  xt::xarray<double> H_core;

  /**
   * @brief One particle density matrix
   *
   */
  xt::xarray<double> D;
  /**
   * @brief One particle density matrix from the previous iteration
   *
   */
  xt::xarray<double> D_last;

  /**
   * @brief Fock matrix
   *
   */
  xt::xarray<double> F;

  /**
   * @brief MO Coefficient matrix
   *
   */
  xt::xarray<double> C;

  /**
   * @brief MO energy vector
   *
   */
  xt::xarray<double> E_orbitals;
  /**
   * @brief Electronic energy
   *
   */
  double E_elec_plus_ep = 0.0;
  /**
   * @brief Electronic energy from the previous iteration
   *
   */
  double E_elec_plus_ep_last = 0.0;
  /**
   * @brief Total energy
   *
   */
  double E_total = 0.0;
  /**
   * @brief Iteration number
   *
   */
  int iteration_num = 0;
  /**
   * @brief Iteration energy difference
   *
   */
  double iteration_E_diff = 0.0;
  /**
   * @brief Iteration rmsc DM
   *
   */
  double iteration_rmsc_dm = 0.0;
  /**
   * @brief Stop running iterations?
   *
   */
  bool stop = false;
  /**
   * @brief Converged?
   *
   */
  bool converged = false;
  /**
   * @brief Exceeded iterations?
   *
   */
  bool exceeded_iterations = false;
  /**
   * @brief Energy convergence
   *
   */
  double convergence_E = 1e-6;
  /**
   * @brief Root mean squared change in DM convergence
   *
   */
  double convergence_DM = 1e-8;
  /**
   * @brief Maximum iteration number
   *
   */
  int iteration_max = 500;

  int excess_particle_charge;
  bool exchange;
  bool polarization_potential = false;
  bool relax_target = false;
};
} // namespace selci
#endif
