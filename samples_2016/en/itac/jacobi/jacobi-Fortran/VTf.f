      subroutine VTSETUP()
c     
c     Setup the Intel(R) Trace Collector instrumentation
c     
      integer ierr

      include 'VT.inc'
      include 'vtcommon.inc'

      call VTCLASSDEF( 'Calculation', calc_class ,ierr )
      call VTCLASSDEF( 'Setup' , setup_class , ierr )
      call VTCLASSDEF( 'Communication' , communication_class , ierr )
      call VTCLASSDEF( 'Initialization' , initialization_class , ierr )

      call VTFUNCDEF( 'main', calc_class, main_state, ierr)
      call VTFUNCDEF( 'diff2d', calc_class, diff2d_state, ierr)
      call VTFUNCDEF( 'mpe_decomp1d', setup_class, 
     $ mpe_decomp1d_state, ierr )
      call VTFUNCDEF( 'exchng2', communication_class, 
     $     exchng2_state , ierr)
      call VTFUNCDEF( 'fnd2ddecomp', setup_class, 
     $     fnd2ddecomp_state, ierr )
      call VTFUNCDEF( 'fnd2dnbrs', setup_class, 
     $     fnd2dnbrs_state, ierr)
      call VTFUNCDEF( 'sweep2d', calc_class, 
     $     sweep2d_state, ierr)
      call VTFUNCDEF( 'twodinit', initialization_class, 
     $     twodinit_state, ierr)

      end




