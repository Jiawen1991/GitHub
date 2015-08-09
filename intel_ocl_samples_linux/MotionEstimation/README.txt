******************************************************************************
**              Intel(R) SDK for OpenCL(tm) Applications - Samples          **
**                                 README                                   **
******************************************************************************


*****  Overview  *****

This package contains the sample that targets the Intel Processor Graphics
device. The sample is supported on Microsoft Windows* and Linux* OS.

For details about the technology implemented in this sample, refer to the
sample User's Guide (PDF), residing in the sample root directory.

For the complete list of supported operating systems and hardware, refer to
the release notes.

*****  Software Requirements  *****

The following software is required to correctly build and run the sample under
Windows* OS:

  - Intel OpenCL implementation for target hardware set
  - Microsoft Visual Studio* 2010, 2012, or 2013
  - OpenGL implementation

To correctly build and run the sample under Linux* OS, the following is
required:

  - Intel OpenCL implementation for target hardware set
  - GNU Make utility (make)
  - GNU C++ compiler (g++)
  - OpenGL implementation

Visit the SDK website at http://software.intel.com/en-us/vcsource/tools/opencl
to get the relevant SDK version.

*****  Sample Directory Content  *****

Sample files reside in the dedicated sample directory and in the 'common'
directory in the root-level (where the sample is extracted) directory.

The sample directory contains the following:

  - common                 -- directory with common utilities and helpers;
                              this functionality is used as a basic
                              infrastructure in the sample code
  - MotionEstimation       -- directory with sample files:

        - main.cpp         -- host code for the sample, including the
                              application entry point, extensions
                              initialization, resources management and VME
                              kernel invocation.
        - yuv_utils.h      -- basic routine to read/write simple YUV streams
                              (YV12)
        - yuv_utils.cpp    -- YV12 is 8 bit Y plane followed by 8 bit 2x2
                              subsampled (which is a value per 4 pixels)
                              V and U planes
        - video_1920x1080_5frames.yuv -- example YUV file, 5 frames@1920x1080
                                         in YV12 (8 bit Y plane followed by 8
                                         bit 2x2 subsampled U and V planes)
        - cmdparser.cpp    -- command-line parameters parsing routines
          cmdparser.hpp
        - utils.cpp        -- general routines like writing bmp files
          utils.h
        - oclobject.cpp    -- general OpenCL* initialization routine
          oclobject.hpp
        - Makefile         -- builds the sample binary (Linux* only).
  - templates              -- project Property files (Windows* only)
  - user_guide.pdf         -- sample User's Guide
  - README.TXT             -- readme file


*****  Building the Sample (Windows* OS) *****

The sample package contains the Microsoft Visual Studio solution files for
Visual Studio IDE version 2010, 2012, and 2013.
To build the sample, do the following:

1. Open the relevant solution file.
2. In Microsoft Visual Studio select Build > Build Solution.

*****  Building the Sample (Linux*) *****

To build this sample, call make in the current directory (MotionEstimation):

  $ make

After successful build, 'MotionEstimation' executable binary is created in the
current directory. If build error occurs, please refer to the 'Software
Requirements' section.


***** Running the Sample (Windows*) *****

You can run the sample application using the standard interface of the
Microsoft Visual Studio IDE, or using the command line.

To run the sample using the Visual Studio IDE, do the following:

1. Open and build the appropriate solution file.
2. Press Ctrl+F5 to run the application.
To run the application in debug mode, press F5.

To run the sample using command line, do the following:

1. Open the command prompt.
2. Navigate to the sample directory.
3. Then go to the directory according to the configuration you built in the
previous step:
    - \Win32 - for Win32 configuration
    - \x64 - for x64 configuration
4. Select the appropriate configuration folder  (Debug or Release).
5. Run the sample by entering the name of the executable.
6. You can run the sample with command-line option -h or --help to print
   available command line options for the sample.

*****  Running the Sample (Linux*) *****

This is a console sample. To run it you need to run the 'MotionEstimation' 
executable in the terminal:

  $ ./MotionEstimation


*****  Sample Command-Line Options  *****

The sample supports the following command-line options:

      -h, --help Shows help text.

      --input  <string> Input video sequence filename - YUV file (YV12 format).
               Default input file is 1920x1080_5frames.yuv
      --output <string> Output video sequence with overlaid motion vectors 
	           filename - YUV file (YV12 format).
               Default name for the output file is output.yuv.

      --nobmp  Disable output frames to the sequence of BMP files in addition
               to the YUV file.
               By default the output is on.

      --width <int> Set frame width for the input file.
               Default value is 1920.

      --height <int> Set frame height for the input file.
               Default value is 1080.

Specific OpenCL device performance depends on the command-line option values.


*****  Disclaimer and Legal Information *****
By using this document, in addition to any agreements you have with Intel,
you accept the terms set forth below.
You may not use or facilitate the use of this document in connection with
any infringement or other legal analysis concerning Intel products described
herein. You agree to grant Intel a non-exclusive, royalty-free license to
any patent claim thereafter drafted which includes subject matter disclosed
herein.

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

Intel Core, HD Graphics and Iris Graphics are trademarks of Intel
Corporation in the U.S. and/or other countries.

* Other names and brands may be claimed as the property of others.

OpenCL and the OpenCL logo are trademarks of Apple Inc. used by
permission from Khronos.

Copyright (c) 2014 Intel Corporation. All rights reserved.
