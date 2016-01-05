/*
 * Copyright 2012-2015 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel
 * Corporation or its suppliers or licensors. Title to the Material
 * remains with Intel Corporation or its suppliers and licensors. The
 * Material may contain trade secrets and proprietary and confidential
 * information of Intel Corporation and its suppliers and licensors,
 * and is protected by worldwide copyright and trade secret laws and
 * treaty provisions. No part of the Material may be used, copied,
 * reproduced, modified, published, uploaded, posted, transmitted,
 * distributed, or disclosed in any way without Intel's prior express
 * written permission.
 *
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you by
 * disclosure or delivery of the Materials, either expressly, by
 * implication, inducement, estoppel or otherwise. Any license under
 * such intellectual property rights must be express and approved by
 * Intel in writing.
 *
 * Unless otherwise agreed by Intel in writing, you may not remove or
 * alter this notice or any other notice embedded in Materials by Intel
 * or Intel's suppliers or licensors in any way.
 */

/* Example demonstrating how to embed Intel(R) Cluster Checker
 * functionality into an MPI program to verify the cluster before
 * running the main code.
 */

#define _GNU_SOURCE 1

#include <clck.h>

#include <cstring>
#include <iostream>
#include <iterator>
#include <list>
#include <sstream>
#include <string>
#include <vector>

#define _STR(s) __STR(s)
#define __STR(s) #s

#ifdef __cplusplus
extern "C" {
#endif

/* function signature differences due to Fortran / C++ type differences */
void clck_check_(int *, char *, int *, int *, int);
bool clck_check(bool, const char *, size_t, size_t);

#ifdef __cplusplus
}
#endif

#ifndef CLCK_DB
#define CLCK_DB "$HOME/.clck/3.1.0/clck.db"
#endif

#ifndef CLCK_ROOT
#define CLCK_ROOT "/opt/intel/clck_latest"
#endif

const std::string prefix = _STR(CLCK_ROOT);

/* data collection would normally occur as a separate step prior to
   running the analysis */
const std::string db_file = _STR(CLCK_DB);

/* list of checks to perform.  only check performance in this
   example. */
const std::vector<std::string> checks = {"dgemm",
                                         "imb_pingpong",
                                         "hpl",
                                         "iozone",
                                         "stream"};

/* knowledge base */
const std::vector<std::string> kb_mods = {"clck3.clp"};
const std::string kb_path = prefix + "/kb";
const std::vector<clck::Layer::ConfigParam> config_params =
  {{"", "clck-checks", checks}};

/* connector extensions */
const std::vector<std::string> extension_mods = checks;
const std::string extension_path = prefix + "/connector/intel64/";

/**
 * @brief Fortran wrapper for clck_check()
 */
void clck_check_(int *verbose_, char *hosts, int *count_, int *ierr_, int len) {
  bool verbose = false;
  int count = 0;

  if (verbose_ && *verbose_ != 0) {
    verbose = true;
  }

  if (count_) {
    count = *count_;
  }

  bool rc = clck_check(verbose, hosts, len, count);

  if (ierr_) {
    if (rc == true) {
      *ierr_ = 0;
    }
    else {
      *ierr_ = 1;
    }
  }
  else {
    std::cerr << "ierr is a null pointer, so unable to return check status"
              << std::endl;
  }
}

/**
 * @brief Convert array of host names into a vector of Nodes
 */
static std::vector<clck::Node> nodevec(const char *hosts, size_t len,
                                       size_t count) {
  std::list<std::string> l;
  std::vector<clck::Node> v;

  for (size_t i = 0 ; i < count ; i++) {
    std::string h = std::string(hosts+i*len, strnlen(hosts+i*len, len));
    /* Fortran strings are padded with whitespace, not nulls */
    h.erase(h.find_last_not_of(" \n\r\t")+1);
    l.push_back(h);
  }

  /* remove duplicates */
  l.sort();
  l.unique();

  v.reserve(l.size());

  for (auto name : l) {
    clck::Node n;
    n.name = name;
    n.roles.push_back(clck::Node::ROLE_COMPUTE); // assume compute
    n.subcluster = "";
    v.push_back(n);
  }

  return v;
}

/**
 * @brief Example demonstrating how to embed Intel(R) Cluster Checker
 *        functionality using the API.
 *
 * @param verbose Control whether to print issues found on stderr.
 *
 * @param hosts Contiguous block of memory containing the list of
 *              hostnames to check
 *
 * @param len Length of a hostname, including padding
 *
 * @param count Number of hostnames
 *
 * @return true if the cluster passes checks, false otherwise.
 */
bool clck_check(bool verbose, const char *hosts, size_t len, size_t count) {
  try{
    /* Configure the behavior of Intel(R) Cluster Checker */
    clck::Layer::Config config;
    config.config_params = config_params;
    config.db = std::make_shared<clck::SQLite>(db_file);
    config.extension_mods = extension_mods;
    config.extension_path = extension_path;
    config.kb_mods = kb_mods;
    config.kb_path = kb_path;
    config.nodes = nodevec(hosts, len, count);

    /* Create the Intel(R) Cluster Checker instance */
    clck::Layer c = clck::Layer(config);

    std::cout << "Starting Intel(R) Cluster Checker pre-run analysis..."
              << std::endl;

    /* Run the analysis */
    if (c.analyze({}) == true) {
      /* Set a filter to only react to the most serious issues, e.g.,
         severity greater than or equal to 80. */
      clck::Layer::Filter filter;
      filter.severity = 80;

      /* Set the sort order, first by severity in descending order,
         then by node name. */
      clck::Layer::Sorting sort1 =
        clck::Layer::Sorting(false, clck::Layer::Sorting::SEVERITY);
      clck::Layer::Sorting sort2 =
        clck::Layer::Sorting(true, clck::Layer::Sorting::NODE);

      /* Get the list of faults */
      std::vector<std::shared_ptr<clck::Fault>> faults =
        c.get_faults(filter, {sort1, sort2});

      /* If there are any faults, return false */
      if (faults.size() > 0) {
        std::cerr << "Intel(R) Cluster Checker detected " << faults.size()
                  << " serious issues." << std::endl;

        if (verbose == true) {
          for (std::shared_ptr<clck::Fault> &fault : faults) {
            /* Concatenate list of nodes */
            std::stringstream s;
            std::copy(fault->nodes.begin(), fault->nodes.end()-1,
                      std::ostream_iterator<std::string>(s, ", "));
            s << *fault->nodes.rbegin();

            /* print the node and the issue description */
            std::cerr << "[" << s.str() << "] " << fault->msg << std::endl;
          }
        }

        std::cerr << "Use the standalone Intel(R) Cluster Checker to "
                  << "get more information." << std::endl;

        return false;
      }
      else {
        /* No faults that match the filter found, so return true */
        std::cout << "No serious issues found by Intel(R) Cluster Checker."
                  << std::endl;

        return true;
      }
    }
    else {
      /* analyze returned false - consider the cluster innocent until
         proven guilty, so return true. */
      std::cerr << "Intel(R) Cluster Checker analysis could not be performed"
                << std::endl;

      return true;
    }
  }
  catch (std::exception &e) {
    /* Intel(R) Cluster Checker generated an exception - consider the
       cluster innocent until proven guilty, so return true. */
    std::cerr << "Intel(R) Cluster Checker exception: " << e.what()
              << std::endl;

    return true;
  }

  return true;
}
