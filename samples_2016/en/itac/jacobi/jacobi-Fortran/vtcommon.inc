      common /classes/ calc_class, setup_class,
     $     communication_class, initialization_class
      integer calc_class, setup_class, communication_class
      integer initialization_class
      common /states/ main_state, diff2d_state, mpe_decomp1d_state,
     $     exchng2_state, fnd2ddecomp_state,
     $     fnd2dnbrs_state, sweep2d_state, twodinit_state
      integer main_state, diff2d_state, mpe_decomp1d_state
      integer exchng2_state, fnd2ddecomp_state
      integer fnd2dnbrs_state, sweep2d_state, twodinit_state
