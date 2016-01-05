##******************************************************************************
##  Copyright(C) 2014-2015 Intel Corporation. All Rights Reserved.
##
##  The source code, information  and  material ("Material") contained herein is
##  owned  by Intel Corporation or its suppliers or licensors, and title to such
##  Material remains  with Intel Corporation  or its suppliers or licensors. The
##  Material  contains proprietary information  of  Intel or  its  suppliers and
##  licensors. The  Material is protected by worldwide copyright laws and treaty
##  provisions. No  part  of  the  Material  may  be  used,  copied, reproduced,
##  modified, published, uploaded, posted, transmitted, distributed or disclosed
##  in any way  without Intel's  prior  express written  permission. No  license
##  under  any patent, copyright  or  other intellectual property rights  in the
##  Material  is  granted  to  or  conferred  upon  you,  either  expressly,  by
##  implication, inducement,  estoppel or  otherwise.  Any  license  under  such
##  intellectual  property  rights must  be express  and  approved  by  Intel in
##  writing.
##
##  *Third Party trademarks are the property of their respective owners.
##
##  Unless otherwise  agreed  by Intel  in writing, you may not remove  or alter
##  this  notice or  any other notice embedded  in Materials by Intel or Intel's
##  suppliers or licensors in any way.
##
##******************************************************************************
##  Content:
##     Intel(R) Data Analytics Acceleration Library samples creation and run
##******************************************************************************

help:
	@echo "Usage: make {libintel64|sointel64|help}"
	@echo "[sample=name] [mode=mode_name]"
	@echo
	@echo "name                - sample name. Please see daal.lst file"
	@echo
	@echo "mode_name           - can be build or run. Default is run"

##------------------------------------------------------------------------------
## samples of using:
##
## make sointel64                - build by Intel(R) C++ Compiler
##                                 and run all samples for Intel(R)64 processor
##                                 family  applications, dynamic linking
##
## make sointel64 mode=build     - build only (not run) by Intel(R) C++ Compiler
##                                 all samples for Intel(R)64
##                                 processor family  applications, dynamic linking
##
## make help                     - show help
##
##------------------------------------------------------------------------------

include daal.lst

compiler = intel

ifndef DAALROOT
    DAALROOT = ../..
endif

DAAL_PATH = "$(DAALROOT)/lib/$(_IA)_lin"
DAAL_LIB := $(DAAL_PATH)/libdaal_core.$(RES_EXT) $(DAAL_PATH)/libdaal_thread.$(RES_EXT)
COPTS := -Wall -w -DDAAL_CHECK_PARAMETER
RES_DIR=_results/$(compiler)_$(_IA)_$(RES_EXT)

ifneq ($(mode),build)
    override mode = run
endif

ifndef sample
    sample = $(MPI)
endif

EXT_LIB := -ltbb -liomp5 -ldl

override proc = 4

RES = $(addprefix $(RES_DIR)/, $(if $(filter run, $(mode)), $(addsuffix .res ,$(sample)), $(addsuffix .exe,$(sample))))

CC = mpiicc
CRUN = mpirun
RUNOPTS = -n $(proc) -ppn 1


.SECONDARY:
$(RES_DIR)/%.exe: ./sources/%.cpp | $(RES_DIR)/.
	$(CC) $(COPTS) $< -o $@ $(LOPTS)

$(RES_DIR)/%.res: $(RES_DIR)/%.exe
	$(CRUN) $(RUNOPTS) $< > $@

LOPTS := $(DAAL_LIB) $(EXT_LIB)

_make_ex: $(RES)

%/.:; mkdir -p $*

libintel64:
	$(MAKE) _make_ex _IA=intel64 RES_EXT=a
sointel64:
	$(MAKE) _make_ex _IA=intel64 RES_EXT=so
