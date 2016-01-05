/* file: SparkKmeans.java */
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
//      Java sample of K-Means clustering in the distributed processing mode
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
import com.intel.daal.algorithms.kmeans.*;
import com.intel.daal.algorithms.kmeans.init.*;
import com.intel.daal.data_management.data.*;
import com.intel.daal.services.*;

import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;

public class SparkKmeans {
    /* Class containing the algorithm results */
    static class KmeansResult {
        public HomogenNumericTable centroids;
    }
    private static final long nBlocks   = 1;
    private static final long nClusters = 20;
    private static final int nIterations = 5;
    private static final int nVectorsInBlock = 10000;

    static JavaPairRDD<Integer, InitPartialResult> partsRDD;
    static JavaPairRDD<Integer, PartialResult> partsRDDcompute;

    static KmeansResult result = new KmeansResult();
    static HomogenNumericTable Centroids;

    public static KmeansResult runKmeans(DaalContext context, JavaPairRDD<Integer, HomogenNumericTable> dataRDD) {

        computeInitLocal(dataRDD);
        Centroids = computeInitMaster(context);

        for(int it = 0; it < nIterations; it++) {

            Centroids.pack();
            computeLocal(dataRDD, Centroids);
            Centroids = computeMaster(context);
        }

        return result;
    }

    private static void computeInitLocal(JavaPairRDD<Integer, HomogenNumericTable> dataRDD) {
        partsRDD = dataRDD.mapToPair(
        new PairFunction<Tuple2<Integer, HomogenNumericTable>, Integer, InitPartialResult>() {
            public Tuple2<Integer, InitPartialResult> call(Tuple2<Integer, HomogenNumericTable> tup) {

                DaalContext context = new DaalContext();

                /* Create an algorithm to initialize the K-Means algorithm on local nodes */
                InitDistributedStep1Local kmeansLocalInit = new InitDistributedStep1Local(context, Double.class, InitMethod.randomDense,
                                                                                          nClusters, nBlocks * nVectorsInBlock,
                                                                                          nVectorsInBlock * tup._1);

                /* Set the input data on local nodes */
                tup._2.unpack(context);
                kmeansLocalInit.input.set( InitInputId.data, tup._2 );

                /* Initialize the K-Means algorithm on local nodes */
                InitPartialResult pres = kmeansLocalInit.compute();
                pres.pack();

                context.dispose();

                return new Tuple2<Integer, InitPartialResult>(tup._1, pres);
            }
        });
    }

    private static HomogenNumericTable computeInitMaster(DaalContext context) {

        /* Create an algorithm to compute k-means on the master node */
        InitDistributedStep2Master kmeansMasterInit = new InitDistributedStep2Master(context, Double.class, InitMethod.randomDense,
                                                                                     nClusters);

        List<Tuple2<Integer, InitPartialResult>> partsList = partsRDD.collect();

        /* Add partial results computed on local nodes to the algorithm on the master node */
        for (Tuple2<Integer, InitPartialResult> value : partsList) {
            value._2.unpack(context);
            kmeansMasterInit.input.add( InitDistributedStep2MasterInputId.partialResults, value._2 );
        }

        /* Compute k-means on the master node */
        kmeansMasterInit.compute();

        /* Finalize computations and retrieve the results */
        InitResult initResult = kmeansMasterInit.finalizeCompute();

        return (HomogenNumericTable)initResult.get(InitResultId.centroids);
    }

    private static void computeLocal(JavaPairRDD<Integer, HomogenNumericTable> dataRDD, final
                                     HomogenNumericTable centroids) {
        partsRDDcompute = dataRDD.mapToPair(
        new PairFunction<Tuple2<Integer, HomogenNumericTable>, Integer, PartialResult>() {
            public Tuple2<Integer, PartialResult> call(Tuple2<Integer, HomogenNumericTable> tup) {

                DaalContext context = new DaalContext();

                /* Create an algorithm to compute k-means on local nodes */
                DistributedStep1Local kmeansLocal = new DistributedStep1Local(context, Double.class, Method.defaultDense, nClusters);

                /* Set the input data on local nodes */
                tup._2.unpack(context);
                centroids.unpack(context);
                kmeansLocal.input.set(InputId.data, tup._2);
                kmeansLocal.input.set(InputId.inputCentroids, centroids);

                /* Compute k-means on local nodes */
                PartialResult pres = kmeansLocal.compute();
                pres.pack();

                context.dispose();

                return new Tuple2<Integer, PartialResult>(tup._1, pres);
            }
        });
    }

    private static HomogenNumericTable computeMaster(DaalContext context) {

        /* Create an algorithm to compute k-means on the master node */
        DistributedStep2Master kmeansMaster = new DistributedStep2Master(context, Double.class, Method.defaultDense,
                                                                         nClusters);

        List<Tuple2<Integer, PartialResult>> parts_List = partsRDDcompute.collect();

        /* Add partial results computed on local nodes to the algorithm on the master node */
        for (Tuple2<Integer, PartialResult> value : parts_List) {
            kmeansMaster.input.add(DistributedStep2MasterInputId.partialResults, value._2);
            value._2.dispose();
        }

        /* Compute k-means on the master node */
        kmeansMaster.compute();

        /* Finalize computations and retrieve the results */
        Result res = kmeansMaster.finalizeCompute();

        result.centroids = (HomogenNumericTable)res.get(ResultId.centroids);
        HomogenNumericTable centr = (HomogenNumericTable)res.get(ResultId.centroids);

        return centr;
    }

}
