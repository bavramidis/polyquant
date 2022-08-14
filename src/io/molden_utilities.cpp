#include "io/molden_utilities.hpp"

using namespace polyquant;

POLYQUANT_MOLDEN::POLYQUANT_MOLDEN(const std::string &fname) { this->create_file(fname); }

void POLYQUANT_MOLDEN::create_file(const std::string &fname) {
  this->filename = fname;
  if (std::filesystem::exists(filename)) {
    std::stringstream s;
    s << "MOLDEN file " << filename << " exists. Overwriting it." << std::endl;
    Polyquant_cout(s.str());
  }
  this->molden_file.open(this->filename);
}

void POLYQUANT_MOLDEN::dump(std::vector<libint2::Atom> &atoms, libint2::BasisSet &basis, Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> &MO_a_coeff,
                            Eigen::Matrix<double, Eigen::Dynamic, 1> &MO_a_energy, std::vector<std::string> &MO_a_symmetry_labels, Eigen::Matrix<double, Eigen::Dynamic, 1> &MO_a_occupation,
                            Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> &MO_b_coeff, Eigen::Matrix<double, Eigen::Dynamic, 1> &MO_b_energy, std::vector<std::string> &MO_b_symmetry_labels,
                            Eigen::Matrix<double, Eigen::Dynamic, 1> &MO_b_occupation) {
  this->dump_header();
  this->dump_atoms(atoms);
  this->dump_basis(atoms, basis);
  this->dump_orbitals(MO_a_coeff, MO_a_energy, MO_a_symmetry_labels, MO_a_occupation, MO_b_coeff, MO_b_energy, MO_b_symmetry_labels, MO_b_occupation);
}

void POLYQUANT_MOLDEN::dump_header() {
  Polyquant_cout("dumping molden header");
  this->molden_file << "[Molden Format]" << std::endl;
  this->molden_file << "Generated by Polyquant" << std::endl;
}

void POLYQUANT_MOLDEN::dump_atoms(std::vector<libint2::Atom> &atoms) {
  // write atom parameters
  Polyquant_cout("dumping molden atom parameters");
  this->molden_file << "[Atoms] (AU)" << std::endl;
  auto atom_count = 0;
  for (const auto &atom : atoms) {
    auto Z = atom.atomic_number;
    if (Z == 0) {
      this->molden_file << std::setw(4) << "X" << std::setw(6) << (atom_count + 1) << std::setw(6) << Z << std::setw(14) << atom.x << std::setw(14) << atom.y << std::setw(14) << atom.z << std::endl;
    } else {
      this->molden_file << std::setw(4) << libint2::chemistry::get_element_info().at(Z - 1).symbol << std::setw(6) << (atom_count + 1) << std::setw(6) << Z << std::setw(14) << atom.x << std::setw(14)
                        << atom.y << std::setw(14) << atom.z << std::endl;
    }
    atom_count++;
  }
}

void POLYQUANT_MOLDEN::dump_basis(std::vector<libint2::Atom> &atoms, libint2::BasisSet &basis) {
  const auto shell2ao = basis.shell2bf();
  const auto atom2shell = basis.atom2shell(atoms);
  this->molden_file << "[GTO]" << std::endl;

  // ao map change from the libint ordering to the molden ordering
  const auto nao = libint2::nbf(basis);
  ao_map.resize(nao);
  long ao_molden = 0;
  for (size_t atom_i = 0; atom_i < atoms.size(); atom_i++) {
    for (auto shell_i : atom2shell[atom_i]) {
      auto ao = shell2ao[shell_i];
      const auto &shell = basis[shell_i];
      const auto ncontr = shell.contr.size();
      for (int c = 0; c != ncontr; ++c) {
        const auto l = shell.contr[c].l;
        const auto pure = shell.contr[c].pure;
        if (pure) {
          using namespace libint2;
          int m;
          FOR_SOLIDHARM_MOLDEN(l, m)
          const auto ao_in_shell = libint2::INT_SOLIDHARMINDEX(l, m);
          ao_map[ao_molden] = ao + ao_in_shell;
          ++ao_molden;
          END_FOR_SOLIDHARM_MOLDEN
          ao += 2 * l + 1;
        } else {
          using namespace libint2;
          int i, j, k;
          FOR_CART_MOLDEN(i, j, k, l)
          const auto ao_in_shell = libint2::INT_CARTINDEX(l, i, j);
          ao_map[ao_molden] = ao + ao_in_shell;
          ++ao_molden;
          END_FOR_CART_MOLDEN
          ao += libint2::INT_NCART(l);
        }
      }
    }
  }

  auto atom_count = 0;
  for (size_t atom_count = 0; atom_count < atoms.size(); atom_count++) {
    this->molden_file << std::setw(4) << (atom_count + 1) << std::setw(4) << 0 << std::endl;
    for (auto shell_i : atom2shell[atom_count]) {
      const libint2::Shell &sh = basis.at(shell_i);
      if (sh.contr.size() == 1) {
        const auto &contr = sh.contr[0];
        const auto l = contr.l;
        if (l > 4) {
          throw std::invalid_argument("Molden can not have l > 4 (h functions and higher).");
        }
        const auto nprim = contr.coeff.size();
        this->molden_file << std::setw(4) << libint2::Shell::am_symbol(contr.l) << std::setw(6) << nprim << std::setw(6) << "1.00" << std::endl;
        for (int iprim = 0; iprim < nprim; ++iprim) {
          this->molden_file << std::scientific << std::uppercase << std::setprecision(10) << std::setw(20) << sh.alpha[iprim] << std::setw(20) << sh.coeff_normalized(0, iprim) << std::endl;
        }
      }
    }
    this->molden_file << std::endl;
  }
  if (basis.at(0).contr[0].pure) {
    this->molden_file << "[5D]" << std::endl;
    this->molden_file << "[7F]" << std::endl;
    this->molden_file << "[9G]" << std::endl;
    this->molden_file << std::endl;
  }
}

void POLYQUANT_MOLDEN::dump_orbitals(Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> &MO_a_coeff, Eigen::Matrix<double, Eigen::Dynamic, 1> &MO_a_energy,
                                     std::vector<std::string> &MO_a_symmetry_labels, Eigen::Matrix<double, Eigen::Dynamic, 1> &MO_a_occupation,
                                     Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> &MO_b_coeff, Eigen::Matrix<double, Eigen::Dynamic, 1> &MO_b_energy,
                                     std::vector<std::string> &MO_b_symmetry_labels, Eigen::Matrix<double, Eigen::Dynamic, 1> &MO_b_occupation) {
  this->molden_file << "[MO]" << std::endl;

  auto nmo_a = MO_a_coeff.cols();
  for (auto mo_idx = 0; mo_idx < nmo_a; mo_idx++) {
    this->molden_file << "  Sym= " << std::uppercase << MO_a_symmetry_labels[mo_idx] << std::endl;
    this->molden_file << "  Ene= " << std::setprecision(10) << std::setw(20) << MO_a_energy[mo_idx] << std::endl;
    this->molden_file << "  Spin= Alpha" << std::endl;
    this->molden_file << "  Occup= " << std::setprecision(10) << std::setw(20) << MO_a_occupation[mo_idx] << std::endl;
    for (auto ao_idx = 0; ao_idx < MO_a_coeff.rows(); ao_idx++) {
      this->molden_file << "    " << std::setw(10) << ao_idx + 1 << std::scientific << std::setprecision(10) << std::setw(20) << MO_a_coeff(ao_map[ao_idx], mo_idx) << std::endl;
      // this->molden_file << "    " << std::setw(10) << ao_idx+1 << std::scientific << std::setprecision(10) << std::setw(20) << MO_a_coeff(ao_idx, mo_idx) << std::endl;
    }
  }
  auto nmo_b = MO_b_coeff.cols();
  for (auto mo_idx = 0; mo_idx < nmo_b; mo_idx++) {
    this->molden_file << "  Sym= " << std::uppercase << MO_b_symmetry_labels[mo_idx] << std::endl;
    this->molden_file << "  Ene= " << std::setprecision(10) << std::setw(20) << MO_b_energy[mo_idx] << std::endl;
    this->molden_file << "  Spin= Beta" << std::endl;
    this->molden_file << "  Occup= " << std::setprecision(10) << std::setw(20) << MO_b_occupation[mo_idx] << std::endl;
    for (auto ao_idx = 0; ao_idx < MO_b_coeff.rows(); ao_idx++) {
      this->molden_file << "    " << std::setw(10) << ao_idx + 1 << std::scientific << std::setprecision(10) << std::setw(20) << MO_b_coeff(ao_map[ao_idx], mo_idx) << std::endl;
      // this->molden_file << "    " << std::setw(10) << ao_idx+1 << std::scientific << std::setprecision(10) << std::setw(20) << MO_b_coeff(ao_idx, mo_idx) << std::endl;
    }
  }
}
// TODO
// void POLYQUANT_MOLDEN::dump_generalparameters(bool complex_vals, bool ecp, bool restricted, int num_ao, int num_mo, bool bohr_unit, int num_part_alpha, int num_part_beta, int num_part_total,
//                                              int multiplicity) {
//  // write General parameters
//  Polyquant_cout("dumping general parameters");
//  auto parameters_group = root_group.create_group("parameters");
//  // write COMPLEX
//  auto IsComplex_dataset = parameters_group.create_dataset("IsComplex", bool_type, simple_space);
//  IsComplex_dataset.write(complex_vals, bool_type, simple_space);
//  // write ECP
//  auto ECP_dataset = parameters_group.create_dataset("ECP", bool_type, simple_space);
//  ECP_dataset.write(ecp, bool_type, simple_space);
//  // write restricted
//  auto SpinRestricted_dataset = parameters_group.create_dataset("SpinRestricted", bool_type, simple_space);
//  SpinRestricted_dataset.write(restricted, bool_type, simple_space);
//  // write num_ao
//  auto numAO_dataset = parameters_group.create_dataset("numAO", int_type, simple_space);
//  numAO_dataset.write(num_ao, int_type, simple_space);
//  // write num_mo
//  auto numMO_dataset = parameters_group.create_dataset("numMO", int_type, simple_space);
//  numMO_dataset.write(num_mo, int_type, simple_space);
//  // write bohr_unit
//  auto bohr_unit_dataset = parameters_group.create_dataset("Unit", bool_type, simple_space);
//  bohr_unit_dataset.write(bohr_unit, bool_type, simple_space);
//  // write num_part_alpha
//  auto NbAlpha_dataset = parameters_group.create_dataset("NbAlpha", int_type, simple_space);
//  NbAlpha_dataset.write(num_part_alpha, int_type, simple_space);
//  // write num_part_beta
//  auto NbBeta_dataset = parameters_group.create_dataset("NbBeta", int_type, simple_space);
//  NbBeta_dataset.write(num_part_beta, int_type, simple_space);
//  // write num_part_total
//  auto NbTotElec_dataset = parameters_group.create_dataset("NbTotElec", int_type, simple_space);
//  NbTotElec_dataset.write(num_part_total, int_type, simple_space);
//  // write multiplicity
//  auto Spin_dataset = parameters_group.create_dataset("spin", int_type, simple_space);
//  Spin_dataset.write(multiplicity, int_type, simple_space);
//}
//// TODO
// void POLYQUANT_MOLDEN::dump_MOs(std::string quantum_part_name, int num_ao, int num_mo, std::vector<Eigen::Matrix<double, Eigen::Dynamic, 1>> E_orb,
//                                std::vector<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>> mo_coeff) {
//  // write MO parameters
//  Polyquant_cout("dumping MOs");
//  auto super_twist_group = root_group.create_group("Super_Twist");
//
//  // write orbital energies
//  for (auto spin_idx = 0ul; spin_idx < E_orb.size(); spin_idx++) {
//    std::string tag = "eigenval_" + std::to_string(spin_idx);
//    std::stringstream buffer;
//    buffer << "Dumping spin dataset " << tag << " in file " << this->filename << " for particle name " << quantum_part_name << "." << std::endl;
//    Polyquant_cout(buffer.str());
//    std::vector<double> orbital_energies(E_orb[spin_idx].data(), E_orb[spin_idx].data() + E_orb[spin_idx].size());
//    auto E_orb_dataset = super_twist_group.create_dataset(tag, vec_double_type, molden::dataspace::Simple({1, orbital_energies.size()}));
//    E_orb_dataset.write(orbital_energies);
//    // write orbital coeffs
//    std::vector<double> flattened_mo_coeff;
//    for (auto i = 0ul; i < num_ao; i++) {
//      for (auto j = 0ul; j < num_mo; j++) {
//        flattened_mo_coeff.push_back(mo_coeff[spin_idx](j, i));
//      }
//    }
//    tag = "eigenset_" + std::to_string(spin_idx);
//    auto mo_coeff_dataset = super_twist_group.create_dataset(tag, vec_double_type, molden::dataspace::Simple({static_cast<size_t>(num_ao), static_cast<size_t>(num_mo)}));
//    mo_coeff_dataset.write(flattened_mo_coeff);
//  }
//}
//// TODO
// void POLYQUANT_MOLDEN::dump_basis(std::vector<std::string> atomic_names, std::vector<std::vector<libint2::Shell>> unique_shells) {
//  // lambda for removing normalization
//  // auto gaussianint_lambda = [](auto n, auto alpha) {
//  //   auto n1 = (n + 1) * 0.5;
//  //   return std::tgamma(n1) / (2.0 * std::pow(alpha, n1));
//  // };
//  // auto gtonorm_lambda = [&gaussianint_lambda](auto l, auto exponent) {
//  //   auto gint_val = gaussianint_lambda((l * 2) + 2, 2.0 * exponent);
//  //   return 1.0 / std::sqrt(gint_val);
//  // };
//  Polyquant_cout("dumping basis parameters");
//  auto basis_group = root_group.create_group("basisset");
//
//  // dump number of basis types
//  auto NbElements_dataset = basis_group.create_dataset("NbElements", int_type, simple_space);
//  NbElements_dataset.write(unique_shells.size(), int_type, simple_space);
//  // dump basis name
//  std::string basis_name = "LCAOBSet";
//  auto str_type = molden::datatype::String::fixed(basis_name.size());
//  str_type.padding(molden::datatype::StringPad::NULLPAD);
//  str_type.encoding(molden::datatype::CharacterEncoding::ASCII);
//  auto dtpl = molden::property::DatasetTransferList();
//  auto basis_name_dataset = basis_group.create_dataset("name", str_type, simple_space);
//  basis_name_dataset.write(basis_name, str_type, simple_space, simple_space, dtpl);
//  // loop over shells
//  auto atom_idx = 0ul;
//  auto shell_idx = 0ul;
//  for (auto atom_shells : unique_shells) {
//    shell_idx = 0ul;
//    auto atom_basis_group = basis_group.create_group("atomicBasisSet" + std::to_string(atom_idx));
//    // dump number of basis types
//    auto NbBasisGroups_dataset = atom_basis_group.create_dataset("NbBasisGroups", int_type, simple_space);
//    NbBasisGroups_dataset.write(atom_shells.size(), int_type, simple_space);
//
//    if (atom_shells[0].contr[0].pure) {
//      // dump cartesian or spherical
//      std::string cart_sph = "spherical";
//      auto str_type = molden::datatype::String::fixed(cart_sph.size());
//      str_type.padding(molden::datatype::StringPad::NULLPAD);
//      str_type.encoding(molden::datatype::CharacterEncoding::ASCII);
//      auto dtpl = molden::property::DatasetTransferList();
//      auto cart_sph_dataset = atom_basis_group.create_dataset("angular", str_type, simple_space);
//      cart_sph_dataset.write(cart_sph, str_type, simple_space, simple_space, dtpl);
//      // dump expansion type
//      std::string expandYlm = "natural";
//      str_type = molden::datatype::String::fixed(expandYlm.size());
//      str_type.padding(molden::datatype::StringPad::NULLPAD);
//      str_type.encoding(molden::datatype::CharacterEncoding::ASCII);
//      dtpl = molden::property::DatasetTransferList();
//      auto expandYlm_dataset = atom_basis_group.create_dataset("expandYlm", str_type, simple_space);
//      expandYlm_dataset.write(expandYlm, str_type, simple_space, simple_space, dtpl);
//    } else {
//      // dump cartesian or spherical
//      std::string cart_sph = "cartesian";
//      auto str_type = molden::datatype::String::fixed(cart_sph.size());
//      str_type.padding(molden::datatype::StringPad::NULLPAD);
//      str_type.encoding(molden::datatype::CharacterEncoding::ASCII);
//      auto dtpl = molden::property::DatasetTransferList();
//      auto cart_sph_dataset = atom_basis_group.create_dataset("angular", str_type, simple_space);
//      cart_sph_dataset.write(cart_sph, str_type, simple_space, simple_space, dtpl);
//      // dump expansion type
//      std::string expandYlm = "Gamess";
//      str_type = molden::datatype::String::fixed(expandYlm.size());
//      str_type.padding(molden::datatype::StringPad::NULLPAD);
//      str_type.encoding(molden::datatype::CharacterEncoding::ASCII);
//      dtpl = molden::property::DatasetTransferList();
//      auto expandYlm_dataset = atom_basis_group.create_dataset("expandYlm", str_type, simple_space);
//      expandYlm_dataset.write(expandYlm, str_type, simple_space, simple_space, dtpl);
//    }
//    // dump atom type and name
//    auto str_type = molden::datatype::String::fixed(atomic_names[atom_idx].size());
//    str_type.padding(molden::datatype::StringPad::NULLPAD);
//    str_type.encoding(molden::datatype::CharacterEncoding::ASCII);
//    auto dtpl = molden::property::DatasetTransferList();
//    auto atom_basis_elementType_dataset = atom_basis_group.create_dataset("elementType", str_type, simple_space);
//    atom_basis_elementType_dataset.write(atomic_names[atom_idx], str_type, simple_space, simple_space, dtpl);
//    auto atom_basis_name_dataset = atom_basis_group.create_dataset("name", str_type, simple_space);
//    atom_basis_name_dataset.write(atomic_names[atom_idx], str_type, simple_space, simple_space, dtpl);
//    // dump normalization
//    std::string normalized = "no";
//    str_type = molden::datatype::String::fixed(normalized.size());
//    str_type.padding(molden::datatype::StringPad::NULLPAD);
//    str_type.encoding(molden::datatype::CharacterEncoding::ASCII);
//    dtpl = molden::property::DatasetTransferList();
//    auto atom_basis_normalization_dataset = atom_basis_group.create_dataset("normalized", str_type, simple_space);
//    atom_basis_normalization_dataset.write(normalized, str_type, simple_space, simple_space, dtpl);
//    // dump grid data
//    int grid_npts = 1001;
//    auto grid_npts_dataset = atom_basis_group.create_dataset("grid_npts", int_type, simple_space);
//    grid_npts_dataset.write(grid_npts, int_type, simple_space);
//
//    int grid_rf = 100;
//    auto grid_rf_dataset = atom_basis_group.create_dataset("grid_rf", int_type, simple_space);
//    grid_rf_dataset.write(grid_rf, int_type, simple_space);
//
//    double grid_ri = 1.0e-6;
//    auto grid_ri_dataset = atom_basis_group.create_dataset("grid_ri", double_type, simple_space);
//    grid_ri_dataset.write(grid_ri, double_type, simple_space);
//
//    std::string grid_type = "log";
//    str_type = molden::datatype::String::fixed(grid_type.size());
//    str_type.padding(molden::datatype::StringPad::NULLPAD);
//    str_type.encoding(molden::datatype::CharacterEncoding::ASCII);
//    dtpl = molden::property::DatasetTransferList();
//    auto grid_type_dataset = atom_basis_group.create_dataset("grid_type", str_type, simple_space);
//    grid_type_dataset.write(grid_type, str_type, simple_space, simple_space, dtpl);
//    shell_idx = 0ul;
//    for (auto shell : atom_shells) {
//      auto shell_group = atom_basis_group.create_group("basisGroup" + std::to_string(shell_idx));
//      // dump number of radial functions
//      auto NbRadFunc_dataset = shell_group.create_dataset("NbRadFunc", int_type, simple_space);
//      NbRadFunc_dataset.write(shell.alpha.size(), int_type, simple_space);
//      // write n and l
//      auto n_dataset = shell_group.create_dataset("n", int_type, simple_space);
//      n_dataset.write(shell_idx, int_type, simple_space);
//      auto l_dataset = shell_group.create_dataset("l", int_type, simple_space);
//      l_dataset.write(shell.contr[0].l, int_type, simple_space);
//      // dump basis type
//      std::string basis_type = "Gaussian";
//      str_type = molden::datatype::String::fixed(basis_type.size());
//      str_type.padding(molden::datatype::StringPad::NULLPAD);
//      str_type.encoding(molden::datatype::CharacterEncoding::ASCII);
//      dtpl = molden::property::DatasetTransferList();
//      auto basis_type_dataset = shell_group.create_dataset("type", str_type, simple_space);
//      basis_type_dataset.write(basis_type, str_type, simple_space, simple_space, dtpl);
//      // dump basis function id
//      std::string basis_id = atomic_names[atom_idx] + std::to_string(shell_idx) + std::to_string(shell.contr[0].l);
//      str_type = molden::datatype::String::fixed(basis_id.size());
//      str_type.padding(molden::datatype::StringPad::NULLPAD);
//      str_type.encoding(molden::datatype::CharacterEncoding::ASCII);
//      dtpl = molden::property::DatasetTransferList();
//      auto basis_id_dataset = shell_group.create_dataset("rid", str_type, simple_space);
//      basis_id_dataset.write(basis_id, str_type, simple_space, simple_space, dtpl);
//      // dump basis function location
//      std::vector<double> origin = {shell.O[0], shell.O[1], shell.O[2]};
//      auto basis_position_dataset = shell_group.create_dataset("Shell_coord", molden::datatype::create<std::vector<double>>(), molden::dataspace::create(origin));
//      basis_position_dataset.write(origin);
//      auto rad_func_group = shell_group.create_group("radfunctions");
//      for (auto i = 0ul; i < shell.alpha.size(); ++i) {
//        auto curr_func_group = rad_func_group.create_group("DataRad" + std::to_string(i));
//        // dump exponent and contraction coefficient
//        double exponent = shell.alpha[i];
//        auto exponent_dataset = curr_func_group.create_dataset("exponent", double_type, simple_space);
//        exponent_dataset.write(exponent, double_type, simple_space);
//        double contraction = shell.contr[0].coeff.at(i);
//        // // REMOVE NORMALIZATION FACTOR FROM LIBINT
//        // // SEE SHELL.H
//        // //
//        // https://github.com/evaleev/libint/blob/3bf3a07b58650fe2ed4cd3dc6517d741562e1249/include/libint2/shell.h#L263
//        // const auto sqrt_Pi_cubed =
//        // double{5.56832799683170784528481798212}; const auto two_alpha
//        // = 2.0 * exponent; const auto two_alpha_to_am32 =
//        //     std::pow(two_alpha, (shell.contr[0].l + 1)) *
//        //     std::sqrt(two_alpha);
//        // const auto normalization_factor =
//        //     std::sqrt(std::pow(2.0, shell.contr[0].l) * two_alpha_to_am32
//        //     /
//        //               (sqrt_Pi_cubed *
//        //                libint2::math::df_Kminus1[2 * shell.contr[0].l]));
//        // contraction /= normalization_factor;
//        // Remove pyscf norm
//        // aply pyscf _nomalize_contracted_ao
//        // std::cout << "before unnormalizing at output " << exponent << " "
//        //          << contraction << std::endl;
//        // contraction /= gtonorm_lambda(shell.contr[0].l, exponent);
//        // std::cout << "after unnormalizing at output " << exponent << " "
//        //           << contraction << std::endl;
//        // std::stringstream buffer;
//        // auto a = gtonorm_lambda(shell.contr[0].l, exponent);
//        // std::cout << a << std::endl;
//        auto contraction_dataset = curr_func_group.create_dataset("contraction", double_type, simple_space);
//        contraction_dataset.write(contraction, double_type, simple_space);
//      }
//      shell_idx++;
//    }
//    atom_idx++;
//  }
//}
//// TODO
// void POLYQUANT_MOLDEN::dump_mf_to_molden_for_QMCPACK(bool pbc, bool complex_vals, bool ecp, bool restricted, int num_ao, int num_mo, bool bohr_unit, int num_part_alpha, int num_part_beta,
//                                                     int num_part_total, int multiplicity, int num_atom, int num_species, std::string quantum_part_name,
//                                                     std::vector<Eigen::Matrix<double, Eigen::Dynamic, 1>> E_orb, std::vector<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>> mo_coeff,
//                                                     std::vector<int> atomic_species_ids, std::vector<int> atomic_number, std::vector<int> atomic_charge, std::vector<int> core_elec,
//                                                     std::vector<std::string> atomic_names, std::vector<std::vector<double>> atomic_centers, std::vector<std::vector<libint2::Shell>> unique_shells) {
//  // create file
//  Polyquant_cout("dumping file");
//  this->dump_application();
//  this->dump_PBC(pbc);
//  this->dump_generalparameters(complex_vals, ecp, restricted, num_ao, num_mo, bohr_unit, num_part_alpha, num_part_beta, num_part_total, multiplicity);
//  this->dump_MOs(quantum_part_name, num_ao, num_mo, E_orb, mo_coeff);
//  this->dump_atoms(num_atom, num_species, atomic_species_ids, atomic_number, atomic_charge, core_elec, atomic_names, atomic_centers);
//  this->dump_basis(atomic_names, unique_shells);
//}
//// TODO
// void POLYQUANT_MOLDEN::dump_post_mf_to_molden_for_QMCPACK(std::vector<std::vector<std::vector<std::vector<uint64_t>>>> dets, Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> C, int N_dets,
//                                                          int N_states, int N_mo) {
//  auto N_int_per_det = dets[0][0][0].size();
//  Polyquant_cout("dumping CI parameters");
//  auto multidet_group = root_group.create_group("MultiDet");
//  for (int part_idx = 0; part_idx < dets.size(); part_idx++) {
//    for (int spin_idx = 0; spin_idx < dets[part_idx].size(); spin_idx++) {
//      std::string tag = "CI_" + std::to_string(part_idx * 2 + spin_idx);
//      std::vector<uint64_t> flattened_dets;
//      for (int i = 0; i < N_dets; i++) {
//        for (int j = 0; j < N_int_per_det; j++) {
//          flattened_dets.push_back(dets[part_idx][spin_idx][i][j]);
//        }
//      }
//      auto det_dataset =
//          multidet_group.create_dataset(tag, molden::datatype::create<std::vector<uint64_t>>(), molden::dataspace::Simple({static_cast<size_t>(N_dets), static_cast<size_t>(N_int_per_det)}));
//      det_dataset.write(flattened_dets);
//    }
//  }
//
//  for (auto i = 0ul; i < N_states; i++) {
//    std::vector<double> coeff;
//    for (auto j = 0ul; j < N_dets; j++) {
//      coeff.push_back(C(j, i));
//    }
//    std::string tag = "Coeff";
//    if (i > 0) {
//      tag += "_" + std::to_string(i);
//    }
//    auto coeff_dataset = multidet_group.create_dataset(tag, molden::datatype::create<std::vector<double>>(), molden::dataspace::Simple({static_cast<size_t>(N_dets)}));
//    coeff_dataset.write(coeff);
//  }
//
//  auto NbDet_dataset = multidet_group.create_dataset("NbDet", int_type, simple_space);
//  NbDet_dataset.write(N_dets, int_type, simple_space);
//
//  auto Nbits_dataset = multidet_group.create_dataset("Nbits", int_type, simple_space);
//  Nbits_dataset.write(N_int_per_det, int_type, simple_space);
//
//  auto nexcitedstate_dataset = multidet_group.create_dataset("nexcitedstate", int_type, simple_space);
//  nexcitedstate_dataset.write(N_states, int_type, simple_space);
//
//  auto nstate_dataset = multidet_group.create_dataset("nstate", int_type, simple_space);
//  nstate_dataset.write(N_mo, int_type, simple_space);
//}
