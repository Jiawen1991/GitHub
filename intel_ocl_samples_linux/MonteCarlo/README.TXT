******************************************************************************
**              Intel(R) SDK for OpenCL* Applications - Samples             **
**             Monte Carlo method for stock options pricing Sample          **
**                                 README                                   **
******************************************************************************

*****  Contents  *****

  - Overview
  - Software Requirements
  - Sample Directory Content
  - Building the Sample
  - Running the Sample
  - Sample Command-Line Options
  - Disclaimer and Legal Information


*****  Overview  *****

This sample demonstrates implementation of the Monte Carlo simulation for the
European stock option pricing. Underlying algorithm is an OpenCL* kernel that
unifies three major algorithm components:

  - Mersenne twister - generation of uniformly distributed pseudorandom numbers
  - Box-Muller transform - generation of normally distributed random numbers
  - Option price calculation using Black-Scholes stock pricing model.
The exact Black-Scholes model is implemented as native code on the host for
comparison with the results, generated with Monte Carlo.

For more information about the sample, refer to the sample User's Guide in the
dedicated sample directory.

For complete list of supported operating systems and hardware, refer to the
release notes.


*****  Software Requirements  *****

To correctly build and run the sample, the following is required:

  - GNU Make utility (make)
  - GNU C++ compiler (g++)
  - Intel SDK for OpenCL Applications XE 2013 R2 and higher


*****  Sample Directory Content  *****

All the files necessary for sample build and run reside at the sample directory
(MonteCarlo) and in common directory of the root directory to which you extract
the samples.

The sample directory contains the following:

  - montecarlo.cpp -- Monte Carlo host side implementation, including the
                      application entry point, validation routine, all OpenCL
                      resources allocation and kernel invocation.
  - cmdoptions.hpp,
    cmdoptions.cpp -- sample command-line parameters definition and checking
                      for correct values based on the OpenCL device capabilities.
  - montecarlo.cl  -- Monte Carlo sample OpenCL kernels, necessary for correct
                      application run.
  - Makefile       -- builds the sample binary.
  - user_guide.pdf -- this sample User's Guide
  - README.TXT     -- instruction on how to build and run sample, information
                      on understanding the sample output.


*****  Building the Sample  *****

To build this sample, in the current directory (MonteCarlo) call make:

  $ make

After successful build, 'montecarlo' executable binary is created in the
current directory. If build error occurs, refer to the 'Software
Requirements' section.


*****  Running the Sample  *****

This is a console sample. To run it, you need to run 'montecarlo'
executable in the terminal:

  $ ./montecarlo

By default, the sample searches for OpenCL platform that contains
"Intel" as a substring in platform name. If the application fails to find
such platform, it prints error message and exits. In this case you need
to select platform manually either by name (as a substring) or by
index through the -p command-line option. For example, to select the platform
that contains "Different platform name" as a sub-name, run this command:

  $ ./montecarlo -p "Different platform name"

or to select platform with index 0, run this one:

  $ ./montecarlo -p 0

By default, sample chooses the first device available for the platform
(device with index 0). It can be also selected through the command-line argument
(-d or -t parameters).


*****  Sample Command-Line Options  *****

The sample supports the following command-line options:

-h, --help     Shows help text.

-p, --platform number-or-string
               Selects the platform, the devices of which are used.

-t, --type all | cpu | gpu | acc | default | <OpenCL constant for device type>
               Selects the device by type on which the OpenCL kernel is
               executed.

-d, --device number-or-string
               Selects the device on which all stuff is executed.

-o, --options <integer>
               Number of options to model.

-g, --work-group-size <integer>
               OpenCL work group size. Set to 0 for work group size auto
               selection.

-s, --samples <integer>
               Number of Monte Carlo samples.

-i, --iterations <integer>
               Number of kernel invocations. For each invoction, performance
               information will be printed.

-e, --threshold <float>
               Validation threshold (if validation is enabled).

-a, --arithmetic float | double
               Type of elements and all calculations.

    --validation
               Enables validation procedure on host (slow for big task sizes).

Specific OpenCL device performance depends on the command-line option values.

Running the sample without any command-line options (default run) is
equivalent to the following:

  $ ./montecarlo -p Intel -d 0 -a float -s 262144 -o 65536 -g 0 -e 0.1 -i 10

These parameters are selected to provide reasonable performance
for the Intel Xeon Phi coprocessor and the Intel CPU OpenCL devices.

If the Intel Xeon Phi coprocessor OpenCL device is available, to use it
provide one additional option: -t acc which means "device type: accelerator":

  $ ./montecarlo -t acc

The following set of parameters should be considered as reasonable for
the following devices:

  - For Intel CPU OpenCL Device:

      $ ./montecarlo -p Intel -t cpu -a float -s 262144 -o 65536
      $ ./montecarlo -p Intel -t cpu -a double -s 262144 -o 65536

  - For Intel Xeon Phi coprocessor OpenCL Device

      $ ./montecarlo -p Intel -t acc -a float -s 262144 -o 65536 -g 16
      $ ./montecarlo -p Intel -t acc -a double -s 262144 -o 65536 -g 16


*****  Disclaimer and Legal Information *****

THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

INFORMATION IN THIS DOCUMENT IS PROVIDED IN CONNECTION WITH INTEL
PRODUCTS. NO LICENSE, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE,
TO ANY INTELLECTUAL PROPERTY RIGHTS IS GRANTED BY THIS DOCUMENT.
EXCEPT AS PROVIDED IN INTEL'S TERMS AND CONDITIONS OF SALE FOR SUCH
PRODUCTS, INTEL ASSUMES NO LIABILITY WHATSOEVER AND INTEL DISCLAIMS
ANY EXPRESS OR IMPLIED WARRANTY, RELATING TO SALE AND/OR USE OF INTEL
PRODUCTS INCLUDING LIABILITY OR WARRANTIES RELATING TO FITNESS FOR
A PARTICULAR PURPOSE, MERCHANTABILITY, OR INFRINGEMENT OF ANY PATENT,
COPYRIGHT OR OTHER INTELLECTUAL PROPERTY RIGHT.

A "Mission Critical Application" is any application in which failure
of the Intel Product could result, directly or indirectly, in personal
injury or death. SHOULD YOU PURCHASE OR USE INTEL'S PRODUCTS FOR ANY
SUCH MISSION CRITICAL APPLICATION, YOU SHALL INDEMNIFY AND HOLD INTEL
AND ITS SUBSIDIARIES, SUBCONTRACTORS AND AFFILIATES, AND THE DIRECTORS,
OFFICERS, AND EMPLOYEES OF EACH, HARMLESS AGAINST ALL CLAIMS COSTS,
DAMAGES, AND EXPENSES AND REASONABLE ATTORNEYS' FEES ARISING OUT OF,
DIRECTLY OR INDIRECTLY, ANY CLAIM OF PRODUCT LIABILITY, PERSONAL INJURY,
OR DEATH ARISING IN ANY WAY OUT OF SUCH MISSION CRITICAL APPLICATION,
WHETHER OR NOT INTEL OR ITS SUBCONTRACTOR WAS NEGLIGENT IN THE DESIGN,
MANUFACTURE, OR WARNING OF THE INTEL PRODUCT OR ANY OF ITS PARTS.

Intel may make changes to specifications and product descriptions at
any time, without notice. Designers must not rely on the absence or
characteristics of any features or instructions marked "reserved" or
"undefined". Intel reserves these for future definition and shall have
no responsibility whatsoever for conflicts or incompatibilities arising
from future changes to them. The information here is subject to change
without notice. Do not finalize a design with this information.

The products described in this document may contain design defects or
errors known as errata which may cause the product to deviate from
published specifications. Current characterized errata are available
on request.

Contact your local Intel sales office or your distributor to obtain the
latest specifications and before placing your product order.

Copies of documents which have an order number and are referenced in
this document, or other Intel literature, may be obtained
by calling 1-800-548-4725, or go to:
http://www.intel.com/design/literature.htm

Intel Corporation is the author of the Materials, and requests that all
problem reports or change requests be submitted to it directly.

Intel Xeon Phi and Xeon are trademarks of Intel Corporation in the U.S.
and/or other countries.

* Other names and brands may be claimed as the property of others.

OpenCL and the OpenCL logo are trademarks of Apple Inc. used by
permission from Khronos.

Copyright (c) 2013 Intel Corporation. All rights reserved.