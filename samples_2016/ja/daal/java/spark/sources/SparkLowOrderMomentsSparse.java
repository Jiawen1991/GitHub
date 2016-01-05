/* file: SparkLowOrderMomentsSparse.java */
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
//      Java sample of computing low order moments in the distributed
//      processing mode.
//
//      Input matrix is stored in the compressed sparse row (CSR) format.
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

import com.intel.daal.algorithms.low_order_moments.*;
import com.intel.daal.data_management.data.*;
import com.intel.daal.services.*;

public class SparkLowOrderMomentsSparse {
    /* Class containing the algorithm results */
    static class MomentsResult {
        public HomogenNumericTable minimum;
        public HomogenNumericTable maximum;
        public HomogenNumericTable sum;
        public HomogenNumericTable sumSquares;
        public HomogenNumericTable sumSquaresCentered;
        public HomogenNumericTable mean;
        public HomogenNumericTable secondOrderRawMoment;
        public HomogenNumericTable variance;
        public HomogenNumericTable standardDeviation;
        public HomogenNumericTable variation;
    }

    static JavaPairRDD<Integer, PartialResult> partsRDD;
    static MomentsResult result = new MomentsResult();

    public static MomentsResult runMoments(DaalContext context, JavaPairRDD<Integer, CSRNumericTable> dataRDD) {
        computestep1Local(dataRDD);

        finalizeMergeOnMasterNode(context);

        return result;
    }

    private static void computestep1Local(JavaPairRDD<Integer, CSRNumericTable> dataRDD) {
        partsRDD = dataRDD.mapToPair(
        new PairFunction<Tuple2<Integer, CSRNumericTable>, Integer, PartialResult>() {
            public Tuple2<Integer, PartialResult> call(Tuple2<Integer, CSRNumericTable> tup) {

                DaalContext context = new DaalContext();

                /* Create an algorithm to compute low order moments on local nodes */
                DistributedStep1Local momentsLocal = new DistributedStep1Local(context, Double.class, Method.fastCSR);

                /* Set the input data on local nodes */
                tup._2.unpack(context);
                momentsLocal.input.set( InputId.data, tup._2 );

                /* Compute low order moments on local nodes */
                PartialResult pres = momentsLocal.compute();
                pres.pack();

                context.dispose();

                return new Tuple2<Integer, PartialResult>(tup._1, pres);
            }
        });
    }

    private static void finalizeMergeOnMasterNode(DaalContext context) {

        /* Create an algorithm to compute low order moments on the master node */
        DistributedStep2Master momentsMaster = new DistributedStep2Master(context, Double.class, Method.fastCSR);

        List<Tuple2<Integer, PartialResult>> parts_List = partsRDD.collect();

        /* Add partial results computed on local nodes to the algorithm on the master node */
        for (Tuple2<Integer, PartialResult> value : parts_List) {
            value._2.unpack(context);
            momentsMaster.input.add( DistributedStep2MasterInputId.partialResults, value._2 );
        }

        /* Compute low order moments on the master node */
        momentsMaster.compute();

        /* Finalize computations and retrieve the results */
        Result res = momentsMaster.finalizeCompute();

        result.minimum              = (HomogenNumericTable)res.get(ResultId.minimum);
        result.maximum              = (HomogenNumericTable)res.get(ResultId.maximum);
        result.sum                  = (HomogenNumericTable)res.get(ResultId.sum);
        result.sumSquares           = (HomogenNumericTable)res.get(ResultId.sumSquares);
        result.sumSquaresCentered   = (HomogenNumericTable)res.get(ResultId.sumSquaresCentered);
        result.mean                 = (HomogenNumericTable)res.get(ResultId.mean);
        result.secondOrderRawMoment = (HomogenNumericTable)res.get(ResultId.secondOrderRawMoment);
        result.variance             = (HomogenNumericTable)res.get(ResultId.variance);
        result.standardDeviation    = (HomogenNumericTable)res.get(ResultId.standardDeviation);
        result.variation            = (HomogenNumericTable)res.get(ResultId.variation);
    }
}
