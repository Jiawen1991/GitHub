/*********************************************************************
 * Copyright 2003-2014 Intel Corporation.
 * All Rights Reserved.
 * 
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel
 * Corporation or its suppliers or licensors. Title to the Material
 * remains with Intel Corporation or its suppliers and licensors. The
 * Material contains trade secrets and proprietary and confidential
 * information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and
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
 *********************************************************************/

#define    NUM_SINGLE_RANKS              93
#define    NUM_COLLECTIVES               14
#define    NUM_P2P                       21

//MPI Single Rank Functions
#define    MPI_Abort                     0
#define    MPI_Address                   1 
#define    MPI_Attr_delete               2 
#define    MPI_Attr_get                  3
#define    MPI_Attr_put                  4
#define    MPI_Buffer                    5
#define    MPI_Buffer_attach             6
#define    MPI_Buffer_detach             7
#define    MPI_Cancel                    8
#define    MPI_Cart_coords               9
#define    MPI_Cart_create               10
#define    MPI_Cartdim_get               11
#define    MPI_Cart_get                  12
#define    MPI_Cart_map                  13
#define    MPI_Cart_rank                 14
#define    MPI_Cart_shift                15
#define    MPI_Cart_sub                  16
#define    MPI_Comm_compare              17
#define    MPI_Comm_create               18
#define    MPI_Comm_dup                  19
#define    MPI_Comm_free                 20
#define    MPI_Comm_group                21
#define    MPI_Comm_rank                 22
#define    MPI_Comm_remote_group         23
#define    MPI_Comm_remote_size          24
#define    MPI_Comm_test_inter           25
#define    MPI_Comm_size                 26
#define    MPI_Comm_split                27
#define    MPI_Dims_create               28
#define    MPI_Errhandler_create         29
#define    MPI_Errhandler_free           30
#define    MPI_Errhander_get             31
#define    MPI_Errhandler_set            32
#define    MPI_Error_class               33
#define    MPI_Error_string              34
#define    MPI_Finalize                  35
#define    MPI_Graph                     36
#define    MPI_Get_elements              37
#define    MPI_Get_processor_name        38
#define    MPI_Get_count                 39
#define    MPI_Graph_create              40
#define    MPI_Graphdims_get             41
#define    MPI_Graph_get                 42
#define    MPI_Graph_map                 43
#define    MPI_Graph_neighbors           44
#define    MPI_Graph_neighbors_count     45
#define    MPI_Group_compare             46
#define    MPI_Group_difference          47
#define    MPI_Group_excl                48
#define    MPI_Group_free                49
#define    MPI_Group_incl                50
#define    MPI_Group_intersection        51
#define    MPI_Group_range_excl          52
#define    MPI_Group_range_incl          53
#define    MPI_Group_rank                54
#define    MPI_Group_size                55
#define    MPI_Group_translate_ranks     56
#define    MPI_Group_union               57
#define    MPI_Intercomm_create          58
#define    MPI_Intercomm_merge           59
#define    MPI_Init                      60
#define    MPI_Initialized               61
#define    MPI_Iprobe                    62
#define    MPI_Keyval_create             63
#define    MPI_Keyvale_free              64
#define    MPI_Pack                      65
#define    MPI_Pack_size                 66
#define    MPI_Pcontrol                  67
#define    MPI_Probe                     68
#define    MPI_Request_free              69
#define    MPI_Test_cancelled            70
#define    MPI_Topo_test                 71
#define    MPI_Type_commit               72
#define    MPI_Type_contiguous           73
#define    MPI_Type_extent               74
#define    MPI_Type_free                 75
#define    MPI_Type_hindexed             76
#define    MPI_Type_hvector              77
#define    MPI_Type_indexed              78
#define    MPI_Type_lb                   79
#define    MPI_Type_size                 80
#define    MPI_Type_struct               81
#define    MPI_Type_ub                   82
#define    MPI_Type_vector               83
#define    MPI_Unpack                    84
#define    MPI_Wtick                     85
#define    MPI_Wtime                     86
#define    MPI_Bsend_init                87
#define    MPI_Irecv                     88
#define    MPI_Recv_init                 89
#define    MPI_Rsend_init                90
#define    MPI_Send_init                 91
#define    MPI_Ssend_init                92

//MPI Collective Operations
#define    MPI_Alltoall                  0
#define    MPI_Alltoallv                 1
#define    MPI_Allgather                 2
#define    MPI_Allgatherv                3
#define    MPI_Allreduce                 4
#define    MPI_Barrier                   5
#define    MPI_Bcast                     6
#define    MPI_Gather                    7
#define    MPI_Gatherv                   8
#define    MPI_Reduce                    9
#define    MPI_Reduce_Scatter            10
#define    MPI_Scan                      11
#define    MPI_Scatter                   12
#define    MPI_Scatterv                  13

//MPI P2P Messages
#define    MPI_Bsend                     0
#define    MPI_Ibsend                    1
#define    MPI_Irsend                    2
#define    MPI_Isend                     3
#define    MPI_Issend                    4
#define    MPI_Recv                      5
#define    MPI_Rsend                     6
#define    MPI_Send                      7
#define    MPI_Sendrecv                  8
#define    MPI_Sendrecv_replace          9
#define    MPI_Ssend                     10
#define    MPI_Start                     11
#define    MPI_Startall                  12
#define    MPI_Test                      13
#define    MPI_Testall                   14
#define    MPI_Testany                   15
#define    MPI_Testsome                  16
#define    MPI_Wait                      17
#define    MPI_Waitall                   18
#define    MPI_Waitany                   19
#define    MPI_Waitsome                  20
