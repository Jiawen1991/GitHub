/* file: SparkPcaSvd.java */
/*
//  Copyright(C) 2014-2015 Intel Corporation. All Rights Reserved.
//
//  The source code, information  and  material ("Material") contained herein is
//  owned  by Intel Corporation or its suppliers or licensors, and title to such
//  Material remains  with Intel Corporation  or its suppliers or licensors. The
//  Material  contains proprietary information  of  Intel or  its  suppliers and
//  licensors. The  Material is protected by worldwide copyright laws and treaty
//  provisions. No  part  of  the  Material  may  be  used,  copied, reproduced,
//  modified, published, uploaded, posted, transmitted, distributed or disclosed
//  in any way  without Intel's  prior  express written  permission. No  license
//  under  any patent, copyright  or  other intellectual property rights  in the
//  Material  is  granted  to  or  conferred  upon  you,  either  expressly,  by
//  implication, inducement,  estoppel or  otherwise.  Any  license  under  such
//  intellectual  property  rights must  be express  and  approved  by  Intel in
//  writing.
//
//  *Third Party trademarks are the property of their respective owners.
//
//  Unless otherwise  agreed  by Intel  in writing, you may not remove  or alter
//  this  notice or  any other notice embedded  in Materials by Intel or Intel's
//  suppliers or licensors in any way.
//
////////////////////////////////////////////////////////////////////////////////
//  Content:
//      Java sample of principal component analysis (PCA) using the singular
//      value decomposition (SVD) method in the distributed processing mode
////////////////////////////////////////////////////////////////////////////////
*/

package DAAL;

import java.util.ArrayList;
import java.util.List;
import java.util.Arrays;
import java.util.Map;
import java.util.HashMap;

import org.apache.spark.api.java.*;
import org.apache.spark.api.java.function.*;
import org.apache.spark.SparkConf;

import scala.Tuple2;
import com.intel.daal.algorithms.pca.*;
import com.intel.daal.algorithms.PartialResult;
import com.intel.daal.data_management.data.*;
import com.intel.daal.services.*;

public class SparkPcaSvd {
    /* Class containing the algorithm results */
    static class PCAResult {
        public HomogenNumericTable eigenVectors;
        public HomogenNumericTable eigenValues;
    }

    static int status;
    static JavaPairRDD<Integer, PartialResult> partsRDD;
    static NumericTable[] mergedTables = new NumericTable[2];
    static PCAResult result = new PCAResult();
    static int nFeatures;

    public static PCAResult runPCA(DaalContext context, JavaPairRDD<Integer, HomogenNumericTable> dataRDD) {
        computestep1Local(dataRDD);

        finalizeMergeOnMasterNode(context);

        return result;
    }

    private static void computestep1Local(JavaPairRDD<Integer, HomogenNumericTable> dataRDD) {
        partsRDD = dataRDD.mapToPair(
        new PairFunction<Tuple2<Integer, HomogenNumericTable>, Integer, PartialResult>() {
            public Tuple2<Integer, PartialResult> call(Tuple2<Integer, HomogenNumericTable> tup) {
                DaalContext context = new DaalContext();

                /* Create an algorithm to compute PCA decomposition using the SVD method on local nodes */
                DistributedStep1Local pcaLocal = new DistributedStep1Local(context, Double.class, Method.svdDense);

                /* Set the input data on local nodes */
                tup._2.unpack(context);
                pcaLocal.input.set( InputId.data, tup._2 );

                /* Compute PCA decomposition on local nodes */
                PartialResult pres = pcaLocal.compute();
                pres.pack();

                context.dispose();

                return new Tuple2<Integer, PartialResult>(tup._1, pres);
            }
        });
    }

    private static void finalizeMergeOnMasterNode(DaalContext context) {

        /* Create an algorithm to compute PCA decomposition using the SVD method on the master node */
        DistributedStep2Master pcaMaster = new DistributedStep2Master(context, Double.class, Method.svdDense);

        List<Tuple2<Integer, PartialResult>> parts_List = partsRDD.collect();

        /* Add partial results computed on local nodes to the algorithm on the master node */
        for (Tuple2<Integer, PartialResult> value : parts_List) {
            value._2.unpack(context);
            pcaMaster.input.add( MasterInputId.partialResults, value._2 );
        }

        /* Compute PCA decomposition on the master node */
        pcaMaster.compute();

        /* Finalize computations and retrieve the results */
        Result res = pcaMaster.finalizeCompute();

        result.eigenVectors = (HomogenNumericTable)res.get(ResultId.eigenVectors);
        result.eigenValues  = (HomogenNumericTable)res.get(ResultId.eigenValues);
    }
}
