/* file: implicit_als_sparse_distributed_mpi.cpp */
/*******************************************************************************
!  Copyright(C) 2014-2015 Intel Corporation. All Rights Reserved.
!
!  The source code, information  and  material ("Material") contained herein is
!  owned  by Intel Corporation or its suppliers or licensors, and title to such
!  Material remains  with Intel Corporation  or its suppliers or licensors. The
!  Material  contains proprietary information  of  Intel or  its  suppliers and
!  licensors. The  Material is protected by worldwide copyright laws and treaty
!  provisions. No  part  of  the  Material  may  be  used,  copied, reproduced,
!  modified, published, uploaded, posted, transmitted, distributed or disclosed
!  in any way  without Intel's  prior  express written  permission. No  license
!  under  any patent, copyright  or  other intellectual property rights  in the
!  Material  is  granted  to  or  conferred  upon  you,  either  expressly,  by
!  implication, inducement,  estoppel or  otherwise.  Any  license  under  such
!  intellectual  property  rights must  be express  and  approved  by  Intel in
!  writing.
!
!  *Third Party trademarks are the property of their respective owners.
!
!  Unless otherwise  agreed  by Intel  in writing, you may not remove  or alter
!  this  notice or  any other notice embedded  in Materials by Intel or Intel's
!  suppliers or licensors in any way.
!
!*******************************************************************************
!  Content:
!    C++ example of the implicit alternating least squares (ALS) algorithm in
!    the distributed processing mode.
!
!    The program trains the implicit ALS model on a training data set.
!******************************************************************************/

/**
 * <a name="DAAL-EXAMPLE-CPP-IMPLICIT_ALS_CSR_DISTRIBUTED"></a>
 * \example implicit_als_csr_distributed_mpi.cpp
 */

#include "mpi.h"
#include "daal.h"
#include "service.h"

using namespace std;
using namespace daal;
using namespace daal::algorithms::implicit_als;

/* Input data set parameters */
const size_t nBlocks         = 4;

int rankId, comm_size;
#define mpi_root 0

/* Number of observations in training data set blocks */
const string trainDatasetFileNames[nBlocks] =
{
    "./data/distributed/implicit_als_sparse_1.csv",
    "./data/distributed/implicit_als_sparse_2.csv",
    "./data/distributed/implicit_als_sparse_3.csv",
    "./data/distributed/implicit_als_sparse_4.csv"
};

/* Number of observations in transposed training data set blocks */
const string transposedTrainDatasetFileNames[nBlocks] =
{
    "./data/distributed/implicit_als_trans_sparse_1.csv",
    "./data/distributed/implicit_als_trans_sparse_2.csv",
    "./data/distributed/implicit_als_trans_sparse_3.csv",
    "./data/distributed/implicit_als_trans_sparse_4.csv"
};

static size_t usersPartition[nBlocks + 1];
static size_t itemsPartition[nBlocks + 1];

static int usersPartitionTmp[nBlocks + 1];
static int itemsPartitionTmp[nBlocks + 1];

typedef double  algorithmFPType;    /* Algorithm floating-point type */
typedef double  dataFPType;         /* Input data floating-point type */

/* Algorithm parameters */
const size_t nUsers = 46;           /* Full number of users */

const size_t nFactors = 2;          /* Number of factors */
const size_t maxIterations = 5;     /* Number of iterations in the implicit ALS training algorithm */

byte *nodeResults;
byte *nodeCPs[nBlocks];
byte *crossProductBuf;
int crossProductLen;
int perNodeCPLength[nBlocks];
int displs[nBlocks];
int sdispls[nBlocks];
int rdispls[nBlocks];
int perNodeArchLengthMaster[nBlocks];
int perNodeArchLengthsRecv[nBlocks];

string rowFileName;
string colFileName;

services::SharedPtr<CSRNumericTable> dataTable;
services::SharedPtr<CSRNumericTable> transposedDataTable;

services::SharedPtr<KeyValueDataCollection> usersOutBlocks;
services::SharedPtr<KeyValueDataCollection> itemsOutBlocks;

services::SharedPtr<training::DistributedPartialResultStep4> itemsPartialResultLocal;
services::SharedPtr<training::DistributedPartialResultStep4> usersPartialResultLocal;
services::SharedPtr<training::DistributedPartialResultStep4> itemsPartialResultsMaster[nBlocks];

services::SharedPtr<training::DistributedPartialResultStep1> step1LocalResults[nBlocks];
services::SharedPtr<training::DistributedPartialResultStep1> step1LocalResult;
services::SharedPtr<NumericTable> step2MasterResult;

services::SharedPtr<KeyValueDataCollection> step3LocalResult;
services::SharedPtr<KeyValueDataCollection> step4LocalInput;

services::SharedPtr<NumericTable> predictedRatingsLocal[nBlocks];
services::SharedPtr<NumericTable> predictedRatingsMaster[nBlocks][nBlocks];

services::SharedPtr<KeyValueDataCollection> step1LocalPredictionResult;
services::SharedPtr<KeyValueDataCollection> step2LocalPredictionInput;

services::SharedPtr<KeyValueDataCollection> itemsPartialResultPrediction;

services::SharedPtr<byte> serializedData;
services::SharedPtr<byte> serializedSendData;
services::SharedPtr<byte> serializedRecvData;

void initializeModel();
void readData();
void trainModel();
void testModel();
void predictRatings();

services::SharedPtr<training::DistributedPartialResultStep4> deserializeStep4PartialResult(byte *buffer, size_t length);
services::SharedPtr<training::DistributedPartialResultStep1> deserializeStep1PartialResult(byte *buffer, size_t length);
services::SharedPtr<NumericTable> deserializeNumericTable(byte *buffer, int length);
services::SharedPtr<PartialModel> deserializePartialModel(byte *buffer, size_t length);

void serializeDAALObject(services::SharedPtr<SerializationIface> data, byte **buffer, int *length);
void gatherStep1(byte *nodeResults, int perNodeArchLength);
void gatherItems(byte *nodeResults, int perNodeArchLength);
void gatherPrediction(byte *nodeResults, int perNodeArchLength, int iNode);
void all2all(byte **nodeResults, int *perNodeArchLength, services::SharedPtr<KeyValueDataCollection> result);

services::SharedPtr<KeyValueDataCollection> computeOutBlocks(
    size_t nRows, services::SharedPtr<CSRNumericTable> dataBlock, size_t *dataBlockPartition);
void computeStep1Local(services::SharedPtr<training::DistributedPartialResultStep4> partialResultLocal);
void computeStep2Master();
void computeStep3Local(size_t offset, services::SharedPtr<training::DistributedPartialResultStep4> partialResultLocal,
                       services::SharedPtr<KeyValueDataCollection> outBlocks);
void computeStep4Local(services::SharedPtr<CSRNumericTable> dataTable,
                       services::SharedPtr<training::DistributedPartialResultStep4> *partialResultLocal);

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankId);

    rowFileName = trainDatasetFileNames[rankId];
    colFileName = transposedTrainDatasetFileNames[rankId];

    readData();
    initializeModel();

    step4LocalInput              = services::SharedPtr<KeyValueDataCollection>(new KeyValueDataCollection());
    itemsPartialResultPrediction = services::SharedPtr<KeyValueDataCollection>(new KeyValueDataCollection());

    int userNum = dataTable->getNumberOfRows();
    int itemNum = transposedDataTable->getNumberOfRows();

    MPI_Allgather(&userNum, sizeof(int), MPI_CHAR, usersPartitionTmp, sizeof(int), MPI_CHAR, MPI_COMM_WORLD);
    MPI_Allgather(&itemNum, sizeof(int), MPI_CHAR, itemsPartitionTmp, sizeof(int), MPI_CHAR, MPI_COMM_WORLD);

    usersPartition[0] = 0;
    itemsPartition[0] = 0;
    for (size_t i = 0; i < nBlocks; i++)
    {
        usersPartition[i + 1] = usersPartition[i] + usersPartitionTmp[i];
        itemsPartition[i + 1] = itemsPartition[i] + itemsPartitionTmp[i];
    }

    usersOutBlocks = computeOutBlocks(dataTable->getNumberOfRows(),           dataTable,           itemsPartition);
    itemsOutBlocks = computeOutBlocks(transposedDataTable->getNumberOfRows(), transposedDataTable, usersPartition);

    trainModel();

    testModel();

    MPI_Finalize();

    cout << "Compute OK" << endl;
    // for (size_t i = 0; i < nBlocks; i++)
    // {
        // for (size_t j = 0; j < nBlocks; j++)
        // {
            // printf("prediction %lu, %lu\n", i, j);
            // printNumericTable(predictedRatingsMaster[i][j]);
        // }
    // }
    return 0;
}

void readData()
{
    /* Read trainDatasetFileName from a file and create a numeric table to store the input data */
    dataTable = services::SharedPtr<CSRNumericTable>(
        createSparseTable<dataFPType>(rowFileName));
    /* Read trainDatasetFileName from a file and create a numeric table to store the input data */
    transposedDataTable = services::SharedPtr<CSRNumericTable>(
        createSparseTable<dataFPType>(colFileName));
}

services::SharedPtr<KeyValueDataCollection> computeOutBlocks(
            size_t nRows, services::SharedPtr<CSRNumericTable> dataBlock, size_t *dataBlockPartition)
{
    char *blockIdFlags = new char[nRows * nBlocks];
    memset(blockIdFlags, 0, nRows * nBlocks);

    size_t *colIndices, *rowOffsets;
    dataBlock->getArrays(NULL, &colIndices, &rowOffsets);
    size_t dataSize = dataBlock->getDataSize();
    for (size_t i = 0; i < nRows; i++)
    {
        for (size_t j = rowOffsets[i] - 1; j < rowOffsets[i+1] - 1; j++)
        {
            char blockId;
            for (size_t k = 1; k < nBlocks + 1; k++)
            {
                if (dataBlockPartition[k-1] <= colIndices[j] - 1 && colIndices[j] - 1 < dataBlockPartition[k])
                {
                    blockIdFlags[(k-1) * nRows + i] = 1;
                }
            }
        }
    }

    size_t nNotNull[nBlocks];
    for (size_t i = 0; i < nBlocks; i++)
    {
        nNotNull[i] = 0;
        for (size_t j = 0; j < nRows; j++)
        {
            nNotNull[i] += blockIdFlags[i * nRows + j];
        }
    }

    services::SharedPtr<KeyValueDataCollection> result(new KeyValueDataCollection());

    for (size_t i = 0; i < nBlocks; i++)
    {
        HomogenNumericTable<int> * indicesTable = new HomogenNumericTable<int>(1, nNotNull[i], NumericTableIface::doAllocate);
        services::SharedPtr<SerializationIface> indicesTableShPtr(indicesTable);
        int *indices = indicesTable->getArray();
        size_t indexId = 0;

        for (size_t j = 0; j < nRows; j++)
        {
            if (blockIdFlags[i * nRows + j])
            {
                indices[indexId++] = j;
            }
        }
        (*result)[i] = indicesTableShPtr;
    }

    delete [] blockIdFlags;
    return result;
}

void initializeModel()
{
    /* Create an algorithm object to initialize the implicit ALS model with the default method */
    training::init::Distributed<step1Local, algorithmFPType, training::init::fastCSR> initAlgorithm;
    initAlgorithm.parameter.fullNUsers = nUsers;
    initAlgorithm.parameter.nFactors = nFactors;
    initAlgorithm.parameter.seed += rankId;
    /* Pass a training data set and dependent values to the algorithm */
    initAlgorithm.input.set(training::init::data, transposedDataTable);

    /* Initialize the implicit ALS model */
    initAlgorithm.compute();

    services::SharedPtr<PartialModel> partialModelLocal = initAlgorithm.getPartialResult()->get(training::init::partialModel);

    itemsPartialResultLocal = services::SharedPtr<training::DistributedPartialResultStep4>(new training::DistributedPartialResultStep4());
    itemsPartialResultLocal->set(training::outputOfStep4ForStep1, partialModelLocal);
}

void computeStep1Local(services::SharedPtr<training::DistributedPartialResultStep4> partialResultLocal)
{
    /* Create algorithm objects to compute a dense variance-covariance matrix in the distributed processing mode using the default method */
    training::Distributed<step1Local> algorithm;
    algorithm.parameter.nFactors = nFactors;

    /* Set input objects for the algorithm */
    algorithm.input.set(training::partialModel, partialResultLocal->get(training::outputOfStep4ForStep1));

    /* Compute partial estimates on local nodes */
    algorithm.compute();

    /* Get the computed partial estimates */
    step1LocalResult = algorithm.getPartialResult();
}

void computeStep2Master()
{
    /* Create algorithm objects to compute a dense variance-covariance matrix in the distributed processing mode using the default method */
    training::Distributed<step2Master> algorithm;
    algorithm.parameter.nFactors = nFactors;

    /* Set input objects for the algorithm */
    for (size_t i = 0; i < nBlocks; i++)
    {
        algorithm.input.add(training::inputOfStep2FromStep1, step1LocalResults[i]);
    }

    /* Compute a partial estimate on the master node from the partial estimates on local nodes */
    algorithm.compute();

    step2MasterResult = algorithm.getPartialResult()->get(training::outputOfStep2ForStep4);
}

void computeStep3Local(size_t offset, services::SharedPtr<training::DistributedPartialResultStep4> partialResultLocal,
                       services::SharedPtr<KeyValueDataCollection> outBlocks)
{
    training::Distributed<step3Local> algorithm;
    algorithm.parameter.nFactors = nFactors;

    services::SharedPtr<HomogenNumericTable<size_t> > offsetTable(
            new HomogenNumericTable<size_t>(&offset, 1, 1));
    algorithm.input.set(training::partialModel,             partialResultLocal->get(training::outputOfStep4ForStep3));
    algorithm.input.set(training::partialModelBlocksToNode, outBlocks);
    algorithm.input.set(training::offset,                   offsetTable);

    algorithm.compute();

    step3LocalResult = algorithm.getPartialResult()->get(training::outputOfStep3ForStep4);

    /* MPI_Alltoallv to populate step4LocalInput */
    for (size_t i = 0; i < nBlocks; i++)
    {
        serializeDAALObject((*step3LocalResult)[i], &nodeCPs[i], &perNodeCPLength[i]);
    }
    all2all(nodeCPs, perNodeCPLength, step4LocalInput);
}

void computeStep4Local(services::SharedPtr<CSRNumericTable> dataTable,
                       services::SharedPtr<training::DistributedPartialResultStep4> *partialResultLocal)
{
    training::Distributed<step4Local> algorithm;
    algorithm.parameter.nFactors = nFactors;

    algorithm.input.set(training::partialModels,         step4LocalInput);
    algorithm.input.set(training::partialData,           dataTable);
    algorithm.input.set(training::inputOfStep4FromStep2, step2MasterResult);

    algorithm.compute();

    *partialResultLocal = algorithm.getPartialResult();
}

void trainModel()
{
    int perNodeArchLength;
    for (size_t iteration = 0; iteration < maxIterations; iteration++)
    {
        computeStep1Local(itemsPartialResultLocal);

        serializeDAALObject(step1LocalResult, &nodeResults, &perNodeArchLength);
        /*Gathering step1LocalResult on the master*/
        gatherStep1(nodeResults, perNodeArchLength);

        if(rankId == mpi_root) {
            computeStep2Master();
            serializeDAALObject(step2MasterResult, &crossProductBuf, &crossProductLen);
        }

        MPI_Bcast(&crossProductLen, sizeof(int), MPI_CHAR, mpi_root, MPI_COMM_WORLD);
        if(rankId != mpi_root)
        {
            crossProductBuf = new byte[crossProductLen];
        }
        MPI_Bcast(crossProductBuf, crossProductLen, MPI_CHAR, mpi_root, MPI_COMM_WORLD);
        step2MasterResult = deserializeNumericTable(crossProductBuf, crossProductLen);

        computeStep3Local(itemsPartition[rankId], itemsPartialResultLocal, itemsOutBlocks);
        computeStep4Local(dataTable, &usersPartialResultLocal);

        computeStep1Local(usersPartialResultLocal);

        serializeDAALObject(step1LocalResult, &nodeResults, &perNodeArchLength);
        /*Gathering step1LocalResult on the master*/
        gatherStep1(nodeResults, perNodeArchLength);

        if(rankId == mpi_root) {
            computeStep2Master();
            serializeDAALObject(step2MasterResult, &crossProductBuf, &crossProductLen);
        }

        MPI_Bcast(&crossProductLen, sizeof(int), MPI_CHAR, mpi_root, MPI_COMM_WORLD);
        if(rankId != mpi_root)
        {
            crossProductBuf = new byte[crossProductLen];
        }
        MPI_Bcast(crossProductBuf, crossProductLen, MPI_CHAR, mpi_root, MPI_COMM_WORLD);
        step2MasterResult = deserializeNumericTable(crossProductBuf, crossProductLen);

        computeStep3Local(usersPartition[rankId], usersPartialResultLocal, usersOutBlocks);
        computeStep4Local(transposedDataTable, &itemsPartialResultLocal);
    }

    /*Gather all itemsPartialResultLocal to itemsPartialResultsMaster on the master and distributing the result over other ranks*/
    serializeDAALObject(itemsPartialResultLocal, &nodeResults, &perNodeArchLength);
    gatherItems(nodeResults, perNodeArchLength);
}

void testModel()
{
    /* Create an algorithm object to predict recommendations of the implicit ALS model */
    for (size_t i = 0; i < nBlocks; i++)
    {
        int perNodeArchLength;
        prediction::ratings::Distributed<step1Local> algorithm;
        algorithm.parameter.nFactors = nFactors;

        algorithm.input.set(prediction::ratings::usersPartialModel, usersPartialResultLocal->get(training::outputOfStep4ForStep1));
        algorithm.input.set(prediction::ratings::itemsPartialModel,
                services::staticPointerCast<training::DistributedPartialResultStep4,
                        SerializationIface>(itemsPartialResultsMaster[i])->get(training::outputOfStep4ForStep1));

        algorithm.compute();

        predictedRatingsLocal[i] = algorithm.getResult()->get(prediction::ratings::prediction);

        // serializeDAALObject(predictedRatingsLocal[i], &nodeResults, &perNodeArchLength);
        // gatherPrediction(nodeResults, perNodeArchLength, i);
    }
}

void gatherStep1(byte *nodeResults, int perNodeArchLength)
{
    MPI_Gather(&perNodeArchLength, sizeof(int), MPI_CHAR, perNodeArchLengthMaster, sizeof(int), MPI_CHAR, mpi_root, MPI_COMM_WORLD);

    if (rankId == mpi_root)
    {
        int memoryBuf = 0;
        for (int i = 0; i < nBlocks; i++)
        {
            memoryBuf += perNodeArchLengthMaster[i];
        }
        serializedData = services::SharedPtr<byte>(new byte[memoryBuf]);

        size_t shift = 0;
        for(size_t i = 0; i < nBlocks ; i++)
        {
            displs[i] = shift;
            shift += perNodeArchLengthMaster[i];
        }
    }

    /* Transfer partial results to step 2 on the root node */
    MPI_Gatherv( nodeResults, perNodeArchLength, MPI_CHAR, serializedData.get(), perNodeArchLengthMaster, displs, MPI_CHAR, mpi_root,
                 MPI_COMM_WORLD);

    if (rankId == mpi_root)
    {
        for(size_t i = 0; i < nBlocks ; i++)
        {
            /* Deserialize partial results from step 1 */
            step1LocalResults[i] = deserializeStep1PartialResult(serializedData.get() + displs[i], perNodeArchLengthMaster[i]);
        }
    }

    delete[] nodeResults;
}

void gatherItems(byte *nodeResults, int perNodeArchLength)
{
    MPI_Allgather(&perNodeArchLength, sizeof(int), MPI_CHAR, perNodeArchLengthMaster, sizeof(int), MPI_CHAR, MPI_COMM_WORLD);

    int memoryBuf = 0;
    for (int i = 0; i < nBlocks; i++)
    {
        memoryBuf += perNodeArchLengthMaster[i];
    }
    serializedData = services::SharedPtr<byte>(new byte[memoryBuf]);

    size_t shift = 0;
    for(size_t i = 0; i < nBlocks ; i++)
    {
        displs[i] = shift;
        shift += perNodeArchLengthMaster[i];
    }

    /* Transfer partial results to step 2 on the root node */
    MPI_Allgatherv( nodeResults, perNodeArchLength, MPI_CHAR, serializedData.get(), perNodeArchLengthMaster, displs, MPI_CHAR, MPI_COMM_WORLD);

    for(size_t i = 0; i < nBlocks ; i++)
    {
        /* Deserialize partial results from step 4 */
        itemsPartialResultsMaster[i] = deserializeStep4PartialResult(serializedData.get() + displs[i], perNodeArchLengthMaster[i]);
    }

    delete[] nodeResults;
}

void gatherPrediction(byte *nodeResults, int perNodeArchLength, int iNode)
{
    MPI_Gather(&perNodeArchLength, sizeof(int), MPI_CHAR, perNodeArchLengthMaster, sizeof(int), MPI_CHAR, mpi_root, MPI_COMM_WORLD);

    if (rankId == mpi_root)
    {
        int memoryBuf = 0;
        for (int i = 0; i < nBlocks; i++)
        {
            memoryBuf += perNodeArchLengthMaster[i];
        }
        serializedData = services::SharedPtr<byte>(new byte[memoryBuf]);

        size_t shift = 0;
        for(size_t i = 0; i < nBlocks ; i++)
        {
            displs[i] = shift;
            shift += perNodeArchLengthMaster[i];
        }
    }

    /* Transfer partial results to step 2 on the root node */
    MPI_Gatherv( nodeResults, perNodeArchLength, MPI_CHAR, serializedData.get(), perNodeArchLengthMaster, displs, MPI_CHAR, mpi_root,
                 MPI_COMM_WORLD);

    if (rankId == mpi_root)
    {
        for(size_t i = 0; i < nBlocks ; i++)
        {
            /* Deserialize partial results from step 1 */
            predictedRatingsMaster[iNode][i] = deserializeNumericTable(serializedData.get() + displs[i], perNodeArchLengthMaster[i]);
        }
    }

    delete[] nodeResults;
}

void all2all(byte **nodeResults, int *perNodeArchLengths, services::SharedPtr<KeyValueDataCollection> result)
{
    int memoryBuf = 0;
    size_t shift = 0;
    for (int i = 0; i < nBlocks; i++)
    {
        memoryBuf += perNodeArchLengths[i];
        sdispls[i] = shift;
        shift += perNodeArchLengths[i];
    }
    serializedSendData = services::SharedPtr<byte>(new byte[memoryBuf]);

    /* memcpy to avoid double compute */
    memoryBuf = 0;
    for (int i = 0; i < nBlocks; i++)
    {
        daal::services::daal_memcpy_s(serializedSendData.get()+memoryBuf, (size_t)perNodeArchLengths[i],
                                      nodeResults[i], (size_t)perNodeArchLengths[i]);
        memoryBuf += perNodeArchLengths[i];
        delete[] nodeResults[i];
    }

    MPI_Alltoall(perNodeArchLengths, sizeof(int), MPI_CHAR, perNodeArchLengthsRecv, sizeof(int), MPI_CHAR, MPI_COMM_WORLD);

    memoryBuf = 0;
    shift = 0;
    for (int i = 0; i < nBlocks; i++)
    {
        memoryBuf += perNodeArchLengthsRecv[i];
        rdispls[i] = shift;
        shift += perNodeArchLengthsRecv[i];
    }
    serializedRecvData = services::SharedPtr<byte>(new byte[memoryBuf]);

    /* Transfer partial results to step 2 on the root node */
    MPI_Alltoallv( serializedSendData.get(), perNodeArchLengths, sdispls, MPI_CHAR,
                  serializedRecvData.get(), perNodeArchLengthsRecv, rdispls, MPI_CHAR, MPI_COMM_WORLD);

    for (size_t i = 0; i < nBlocks; i++)
    {
        (*result)[i] = deserializePartialModel(serializedRecvData.get() + rdispls[i], perNodeArchLengthsRecv[i]);
    }
}

void serializeDAALObject(services::SharedPtr<SerializationIface> data, byte **buffer, int *length)
{
    /* Create a data archive to serialize the numeric table */
    InputDataArchive dataArch;

    /* Serialize the numeric table into the data archive */
    data->serialize(dataArch);

    /* Get the length of the serialized data in bytes */
    *length = dataArch.getSizeOfArchive();

    /* Store the serialized data in an array */
    *buffer = new byte[*length];
    dataArch.copyArchiveToArray(*buffer, *length);
}

template<typename T>
services::SharedPtr<SerializationIface> deserializeDAALObject(byte *buffer, size_t length)
{
    /* Create a data archive to deserialize the numeric table */
    OutputDataArchive dataArch(buffer, length);

    /* Create a numeric table object */
    services::SharedPtr<SerializationIface> data = services::SharedPtr<SerializationIface>(new T());

    /* Deserialize the numeric table from the data archive */
    data->deserialize(dataArch);

    return data;
}

services::SharedPtr<PartialModel> deserializePartialModel(byte *buffer, size_t length)
{
    return services::staticPointerCast<PartialModel, SerializationIface>(
        deserializeDAALObject<PartialModel>(buffer, length));
}

services::SharedPtr<NumericTable> deserializeNumericTable(byte *buffer, int length)
{
    OutputDataArchive outDataArch(buffer, length);

    algorithmFPType *values = NULL;
    services::SharedPtr<NumericTable> restoredDataTable = services::SharedPtr<NumericTable>( new HomogenNumericTable<algorithmFPType>(values) );

    restoredDataTable->deserialize(outDataArch);

    return restoredDataTable;
}

services::SharedPtr<training::DistributedPartialResultStep4> deserializeStep4PartialResult(byte *buffer, size_t length)
{
    return services::staticPointerCast<training::DistributedPartialResultStep4, SerializationIface>(
        deserializeDAALObject<training::DistributedPartialResultStep4>(buffer, length));
}

services::SharedPtr<training::DistributedPartialResultStep1> deserializeStep1PartialResult(byte *buffer, size_t length)
{
    return services::staticPointerCast<training::DistributedPartialResultStep1, SerializationIface>(
        deserializeDAALObject<training::DistributedPartialResultStep1>(buffer, length));
}
