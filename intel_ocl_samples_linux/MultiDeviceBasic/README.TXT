******************************************************************************
**              Intel(R) SDK for OpenCL* Applications - Samples             **
**                        Multi-Device Basic Sample                         **
**                                  README                                  **
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

Multi-Device Basic sample demonstrates the minimal sequence of steps to keep
simultaneously busy all devices of the system with multiple OpenCL devices.
The sample comprises a synthetic kernel that operates in 1D iteration space,
and simple work partitioning strategy: dividing work among devices equally,
regardless of device compute capabilities.

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

All files, necessary for sample build and execution, reside in the sample
directory (MultiDeviceBasic) and in the 'common' directory of the root
directory, to which you extract samples.

The sample directory contains the following:

  - Source files:
      - multidevice.hpp              -- declaration of main sample functions:
                                        three scenarios and kernel create
                                        function.
      - multidevice.cpp              -- entry point, command-line parameters
                                        definition and parsing, selecting and
                                        calling one of the scenarios.
      - kernel.cpp                   -- creating an OpenCL program from a
                                        string; kernel code is in-lined to
                                        this file.
      - system.cpp                   -- implementation of the system-level
                                        scenario.
      - multi.cpp                    -- implementation of the multi-context
                                        scenario.
      - shared.cpp                   -- implementation of the shared-context
                                        scenario.
  - Scripts to run system-level scenario on different hardware
    configurations:
      - cpu+mic.system-level.sh      -- runs the system-level scenario with
                                        an application instance per OpenCL
                                        device:
                                        - CPU OpenCL device
                                        - Intel Xeon Phi coprocessor device.
      - multimic.system-level.sh     -- runs the system-level scenario with
                                        several application instances. Each
                                        instance is mapped for a dedicated
                                        Intel Xeon Phi coprocessor OpenCL
                                        device.
      - cpu+multimic.system-level.sh -- runs the system-level scenario with
                                        several application instances, one
                                        for CPU OpenCL device and one per
                                        each dedicated Intel Xeon Phi
                                        coprocessor OpenCL device.
  - Other files:
      - Makefile                     -- makefile for building the sample
      - user_guide.pdf               -- this sample User's Guide
      - README.TXT                   -- Readme file


*****  Building the Sample  *****

To build this sample, call make in the current directory (MultiDeviceBasic):

  $ make

After successful build, 'multidevice' executable binary is created in the
current directory. If build error occurs, please refer to the 'Software
Requirements' section.


*****  Running the Sample  *****

This is a console sample. It consists of several files that you can execute:

  - multidevice       -- sample binary itself; a console application.

  - *.system-level.sh -- scripts to run the system-level scenario in various
                         hardware configurations

For the system-level scenario, besides using the multidevice binary file,
you also need to run several instances of the applications simultaneously.
You can use the following scripts for such purpose:

  - cpu+mic.system-level.sh for a system with a single CPU and single
    Intel(R) Xeon Phi(tm) coprocessor

  - multimic.system-level.sh for a system with multiple Intel Xeon Phi
    coprocessors

  - cpu+multimic.system-level.sh for a system with a single CPU and multiple
    Intel Xeon Phi coprocessors

You can use the –s option for the scripts above. Other options are set
automatically. Scripts that which contain the multimic substring in the
filename run the sample on multiple Intel Xeon Phi coprocessor devices
and accept one required unnamed parameter, which reflects the number of
the utilized Intel Xeon Phi coprocessors. The parameter must be the first
argument. For example, on system with eight Intel Xeon Phi coprocessors,
run the multimic.system-level.sh file as follows:
    ./multimic.system-level.sh 8
If you need to use the –s option, put it after the number of devices,
for example:
    ./multimic.system-level.sh 8 –s 1048576

You can execute the multi-context and shared-context scenarios by calling
the multidevice binary with a particular --context command-line option. When
running the executable, you can choose platform, devices, and other
parameters through the command line.

By default, the sample searches the OpenCL platform that contains
"Intel" as a substring in the platform name. If the application fails to
find such platform, it prints error message and exits. In this case, you
need to select platform manually either by name (as a substring) or by
index through the -p command-line option. For example, to select the
platform that contains "platform_name" as a sub-name, run this command:

  $ ./multidevice -p "platform_name"

To select platform with index 0, run this command:

  $ ./multidevice -p 0

By default, sample selects all devices of all types available for the
platform. It can be also selected through command-line argument
(-t parameter).


*****  Sample Command-Line Options  *****

The sample supports the following command-line options:

-h, --help     Shows help text.

-p, --platform number-or-string
               Selects the platform, the devices of which are used.

-t, --type all | cpu | gpu | acc | default | <OpenCL constant for device type>
               Selects the device by type on which the OpenCL kernel is
               executed.

-c, --context system | multi | shared
               Type of the multi-device scenario used: with system-level
               partitioning, with multiple devices and multiple contexts for
               each device or one shared context for all devices. For one
               device in the system, system = multiple = shared.

-s, --size <integer>
               Global number of work items to be divided among all devices.

    --instance-count <integer>
               Applicable for system-level scenario only. Number of
               application instances which will participate in system-level
               scenario. To identify particular instance, use --instance-index
               key.

    --instance-index <integer>
               Applicable for system-level scenario only. Index of instance
               among  all participating application instances which is set by
               --instance-count key.

For the system-level scenarios, besides using the multidevice binary file,
you also need to run several instances of the applications simultaneously.
You can use the following scripts for such purpose:

  - cpu+mic.system-level.sh
  - multimic.system-level.sh
  - cpu+multimic.system-level.sh

For the scripts you can use the -s option only among all available, scripts
set other options automatically, so you do not need to control the options
manually. Scripts that run on multiple Intel Xeon Phi coprocessor devices
(their name contains the 'multimic' part), accept one required no-name
parameter that should go as the first argument -- this is the number of
the Intel Xeon Phi coprocessor devices, on which you need to run the sample.
For example, on the system with eight Intel Xeon Phi coprocessors, run
multimic.system-level.sh file with the following parameters:

  $ ./multimic.system-level.sh 8

If you need to control the -s option, put it after the number of devices,
for example:

  $ ./multimic.system-level.sh 8 –s 1048576


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