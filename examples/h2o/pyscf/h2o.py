from pyscf import gto, scf
import numpy as np
from PyscfToQmcpack import savetoqmcpack


mol = gto.Mole()
mol.verbose = 1
# mol.output = 'out_h2o'
mol.atom = """
O   0.0000000 0.0000000  0.0000000,
H   0.7569685 0.0000000 -0.5858752,
H  -0.7569685 0.0000000 -0.5858752"""
mol.symmetry = 0
mol.unit = "Angstrom"
#mol.cart=True
mol.basis={ "O": gto.basis.parse('''
    O    S
      1.172000E+04           7.100000E-04          -1.600000E-04           0.000000E+00
      1.759000E+03           5.470000E-03          -1.263000E-03           0.000000E+00
      4.008000E+02           2.783700E-02          -6.267000E-03           0.000000E+00
      1.137000E+02           1.048000E-01          -2.571600E-02           0.000000E+00
      3.703000E+01           2.830620E-01          -7.092400E-02           0.000000E+00
      1.327000E+01           4.487190E-01          -1.654110E-01           0.000000E+00
      5.025000E+00           2.709520E-01          -1.169550E-01           0.000000E+00
      1.013000E+00           1.545800E-02           5.573680E-01           0.000000E+00
      3.023000E-01           0.0000000E+00          0.0000000E+00          1.000000E+00
O    P
      1.770000E+01           4.301800E-02           0.000000E+00
      3.854000E+00           2.289130E-01           0.000000E+00
      1.046000E+00           5.087280E-01           0.000000E+00
      2.753000E-01           0.0000000E+00          1.000000E+00
O    D
      1.185000E+00           1.0000000
'''),
"H" : gto.basis.parse('''
    H    S
      1.301000E+01           1.968500E-02           0.000000E+00
      1.962000E+00           1.379770E-01           0.000000E+00
      4.446000E-01           4.781480E-01           0.000000E+00
      1.220000E-01           0.0000000E+00          1.000000E+00
H    P
      7.270000E-01           1.0000000
    ''')}
mol.cart=False
mol.build()

myhf = scf.RHF(mol)
b = myhf.get_hcore()
myhf.run()
print(myhf.e_tot)

from pyscf.scf.chkfile import dump_scf
dump_scf(mol, 'h2o.chk', myhf.e_tot, myhf.mo_energy, myhf.mo_coeff, myhf.mo_coeff)

savetoqmcpack(mol, myhf)
