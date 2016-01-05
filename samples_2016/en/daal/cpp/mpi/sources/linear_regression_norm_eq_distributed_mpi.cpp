/* file: linear_regression_norm_eq_distributed_mpi.cpp */
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
!    C++ sample of multiple linear regression in the distributed processing
!    mode.
!
!    The program trains the multiple linear regression model on a training
!    data set with the normal equations method and computes regression for the
!    test data.
!******************************************************************************/

/**
 * <a name="DAAL-SAMPLE-CPP-LINEAR_REGRESSION_NORM_EQ_DISTRIBUTED"></a>
 * \example linear_regression_norm_eq_distributed_mpi.cpp
 */

#include <mpi.h>
#include "daal.h"
#include "service.h"

using namespace std;
using namespace daal;
using namespace daal::algorithms::linear_regression;

const string trainDatasetFileNames[] =
{
    "./data/distributed/linear_regression_train_1.csv",
    "./data/distributed/linear_regression_train_2.csv",
    "./data/distributed/linear_regression_train_3.csv",
    "./data/distributed/linear_regression_train_4.csv"
};
string testDatasetFileName    = "./data/distributed/linear_regression_test.csv";

const size_t nBlocks              = 4;

const size_t nFeatures            = 10;
const size_t nDependentVariables  = 2;

int rankId, comm_size;
#define mpi_root 0

void trainModel();
void testModel();

services::SharedPtr<training::Result> trainingResult;
services::SharedPtr<prediction::Result> predictionResult;

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankId);

    trainModel();

    if(rankId == mpi_root)
    {
        testModel();
    }

    MPI_Finalize();

    return 0;
}

void trainModel()
{
    /* Initialize FileDataSource<CSVFeatureManager> to retrieve the input data from a .csv file */
    FileDataSource<CSVFeatureManager> trainDataSource(trainDatasetFileNames[rankId],
                                                      DataSource::notAllocateNumericTable,
                                                      DataSource::doDictionaryFromContext);

    /* Create Numeric Tables for training data and labels */
    services::SharedPtr<NumericTable> trainData(new HomogenNumericTable<double>(nFeatures, 0, NumericTable::notAllocate));
    services::SharedPtr<NumericTable> trainDependentVariables(new HomogenNumericTable<double>(nDependentVariables, 0, NumericTable::notAllocate));
    services::SharedPtr<NumericTable> mergedData(new MergedNumericTable(trainData, trainDependentVariables));

    /* Retrieve the data from the input file */
    trainDataSource.loadDataBlock(mergedData.get());

    /* Create an algorithm object to train the multiple linear regression model based on the local-node data */
    training::Distributed<step1Local> localAlgorithm;

    /* Pass a training data set and dependent values to the algorithm */
    localAlgorithm.input.set(training::data, trainData);
    localAlgorithm.input.set(training::dependentVariables, trainDependentVariables);

    /* Train the multiple linear regression model on local nodes */
    localAlgorithm.compute();

    /* Serialize partial results required by step 2 */
    services::SharedPtr<byte> serializedData;
    InputDataArchive dataArch;
    localAlgorithm.getPartialResult()->serialize(dataArch);
    size_t perNodeArchLength = dataArch.getSizeOfArchive();

    /* Serialized data is of equal size on each node if each node called compute() equal number of times */
    if (rankId == mpi_root)
    {
        serializedData = services::SharedPtr<byte>(new byte[perNodeArchLength * nBlocks]);
    }

    byte *nodeResults = new byte[perNodeArchLength];
    dataArch.copyArchiveToArray( nodeResults, perNodeArchLength );

    /* Transfer partial results to step 2 on the root node */
    MPI_Gather( nodeResults, perNodeArchLength, MPI_CHAR, serializedData.get(), perNodeArchLength, MPI_CHAR, mpi_root,
                MPI_COMM_WORLD);

    delete[] nodeResults;

    if(rankId == mpi_root)
    {
        /* Create an algorithm object to build the final multiple linear regression model on the master node */
        training::Distributed<step2Master> masterAlgorithm;

        for(size_t i = 0; i < nBlocks ; i++)
        {
            /* Deserialize partial results from step 1 */
            OutputDataArchive dataArch(serializedData.get() + perNodeArchLength * i, perNodeArchLength);

            services::SharedPtr<training::PartialResult> dataForStep2FromStep1 = services::SharedPtr<training::PartialResult>
                                                                       (new training::PartialResult());
            dataForStep2FromStep1->deserialize(dataArch);

            /* Set the local multiple linear regression model as input for the master-node algorithm */
            masterAlgorithm.input.add(training::partialModels, dataForStep2FromStep1);
        }

        /* Merge and finalizeCompute the multiple linear regression model on the master node */
        masterAlgorithm.compute();
        masterAlgorithm.finalizeCompute();

        /* Retrieve the algorithm results */
        trainingResult = masterAlgorithm.getResult();
        printNumericTable(trainingResult->get(training::model)->getBeta(), "Linear Regression coefficients:");
    }
}

void testModel()
{
    /* Initialize FileDataSource<CSVFeatureManager> to retrieve the input data from a .csv file */
    FileDataSource<CSVFeatureManager> testDataSource(testDatasetFileName, DataSource::doAllocateNumericTable,
                                                     DataSource::doDictionaryFromContext);

    /* Retrieve the data from an input file */
    testDataSource.loadDataBlock();

    /* Create an algorithm object to predict values of multiple linear regression */
    prediction::Batch<> algorithm;

    /* Pass a testing data set and the trained model to the algorithm */
    algorithm.input.set(prediction::data, testDataSource.getNumericTable());
    algorithm.input.set(prediction::model, trainingResult->get(training::model));

    /* Predict values of multiple linear regression */
    algorithm.compute();

    /* Retrieve the algorithm results */
    predictionResult = algorithm.getResult();
    printNumericTable(predictionResult->get(prediction::prediction), "Linear Regression prediction results:");
}
