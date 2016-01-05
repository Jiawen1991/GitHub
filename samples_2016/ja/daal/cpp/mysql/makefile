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
	@echo "Usage: make {libia32|soia32|libintel64|sointel64|help}"
	@echo "[compiler=compiler_name] [mode=mode_name] [sample=sample_name]"
	@echo
	@echo "compiler_name       - intel only"
	@echo "                      Intel(R) C++ Compiler as default"
	@echo
	@echo "mode_name           - can be build or run. Default is run"
	@echo "sample_name         - sample name. Please see daal.lst file"

##------------------------------------------------------------------------------
## samples of using:
##
## make sointel64                - build by Intel(R) C++ Compiler
##                                 and run all samples for Intel(R)64 processor
##                                 family  applications, dynamic linking
##
## make sointel64 mode=build     - build only (not run) by Intel(R) C++ Compiler
##                                 (as default) all samples for Intel(R)64
##                                 processor family  applications, dynamic linking
##
## make help                     - show help
##
##------------------------------------------------------------------------------

include daal.lst

ifndef compiler
    compiler = intel
endif

ifndef DAALROOT
    DAALROOT = ../..
endif

ifndef sample
    sample = $(MYSQL)
endif

DAAL_PATH = "$(DAALROOT)/lib/$(_IA)_lin"
DAAL_LIB := $(DAAL_PATH)/libdaal_core.$(RES_EXT) $(DAAL_PATH)/libdaal_thread.$(RES_EXT)

ifneq ($(mode),build)
    override mode = run
endif

EXT_LIB := -ltbb -liomp5 -lodbc -lpthread -ldl

COPTS := -Wall -w -DDAAL_CHECK_PARAMETER
LOPTS := $(DAAL_LIB) $(EXT_LIB)

RES_DIR=_results/$(compiler)_$(_IA)_$(RES_EXT)
RES = $(addprefix $(RES_DIR)/, $(if $(filter run, $(mode)), $(addsuffix .res ,$(sample)), $(addsuffix .exe,$(sample))))


ifeq ($(compiler),intel)
    CC = icc
endif

ifeq ($(compiler),gnu)
    CC = g++
    COPTS += $(if $(filter ia32, $(_IA)), -m32, -m64)
endif



.SECONDARY:
$(RES_DIR)/%.exe: ./sources/%.cpp | $(RES_DIR)/.
	$(CC) $(COPTS) $< -o $@ $(LOPTS)

$(RES_DIR)/%.res: $(RES_DIR)/%.exe
	$< > $@

_make_ex: $(RES)

%/.:; mkdir -p $*

libia32:
	$(MAKE) _make_ex _IA=ia32 RES_EXT=a
soia32:
	$(MAKE) _make_ex _IA=ia32 RES_EXT=so
libintel64:
	$(MAKE) _make_ex _IA=intel64 RES_EXT=a
sointel64:
	$(MAKE) _make_ex _IA=intel64 RES_EXT=so
