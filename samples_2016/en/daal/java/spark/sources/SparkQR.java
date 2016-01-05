/* file: SparkQR.java */
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
 //     Java sample of computing QR decomposition in the distributed processing
 //     mode
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

import com.intel.daal.data_management.data.HomogenNumericTable;
import com.intel.daal.data_management.data.NumericTable;
import com.intel.daal.algorithms.qr.*;
import com.intel.daal.data_management.data.*;
import com.intel.daal.data_management.data_source.FileDataSource;
import com.intel.daal.data_management.data_source.DataSource;
import com.intel.daal.services.*;

public class SparkQR {
    /* Class containing the algorithm results */
    static class QRResult {
        public HomogenNumericTable R;
        public JavaPairRDD<Integer, HomogenNumericTable> Q;
    }

    static QRResult result = new QRResult();

    static JavaPairRDD<Integer, DataCollection> dataFromStep1ForStep2_RDD;
    static JavaPairRDD<Integer, DataCollection> dataFromStep1ForStep3_RDD;
    static JavaPairRDD<Integer, DataCollection> dataFromStep2ForStep3_RDD;

    public static QRResult runQR(DaalContext context, JavaPairRDD<Integer, HomogenNumericTable> dataRDD, JavaSparkContext sc) {

        computeStep1Local(dataRDD);

        computeStep2Master(context, sc);

        computeStep3Local();

        return result;
    }

    private static void computeStep1Local(JavaPairRDD<Integer, HomogenNumericTable> dataRDD) {
        /* Create an RDD containing partial results for steps 2 and 3 */
        JavaPairRDD<Integer, Tuple2<DataCollection, DataCollection>> dataFromStep1_RDD = dataRDD.mapToPair(
        new PairFunction<Tuple2<Integer, HomogenNumericTable>, Integer, Tuple2<DataCollection, DataCollection>>() {
            public Tuple2<Integer, Tuple2<DataCollection, DataCollection>> call(Tuple2<Integer, HomogenNumericTable> tup) {
                DaalContext context = new DaalContext();

                /* Create an algorithm to compute QR decomposition on local nodes */
                DistributedStep1Local qrStep1Local = new DistributedStep1Local(context, Double.class, Method.defaultDense);
                tup._2.unpack(context);
                qrStep1Local.input.set( InputId.data, tup._2 );

                /* Compute QR decomposition in step 1  */
                DistributedStep1LocalPartialResult pres = qrStep1Local.compute();
                DataCollection dataFromStep1ForStep2 = pres.get( PartialResultId.outputOfStep1ForStep2 );
                dataFromStep1ForStep2.pack();
                DataCollection dataFromStep1ForStep3 = pres.get( PartialResultId.outputOfStep1ForStep3 );
                dataFromStep1ForStep3.pack();

                context.dispose();

                return new Tuple2<Integer, Tuple2<DataCollection, DataCollection>>(
                           tup._1, new Tuple2<DataCollection, DataCollection>(dataFromStep1ForStep2, dataFromStep1ForStep3));
            }
        }).cache();

        /* Extract partial results for step 3 */
        dataFromStep1ForStep3_RDD = dataFromStep1_RDD.mapToPair(
        new PairFunction<Tuple2<Integer, Tuple2<DataCollection, DataCollection>>, Integer, DataCollection>() {
            public Tuple2<Integer, DataCollection> call(Tuple2<Integer, Tuple2<DataCollection, DataCollection>> tup) {
                return new Tuple2<Integer, DataCollection>(tup._1, tup._2._2);
            }
        });

        /* Extract partial results for step 2 */
        dataFromStep1ForStep2_RDD = dataFromStep1_RDD.mapToPair(
        new PairFunction<Tuple2<Integer, Tuple2<DataCollection, DataCollection>>, Integer, DataCollection>() {
            public Tuple2<Integer, DataCollection> call(Tuple2<Integer, Tuple2<DataCollection, DataCollection>> tup) {
                return new Tuple2<Integer, DataCollection>(tup._1, tup._2._1);
            }
        });
    }

    private static void computeStep2Master(DaalContext context, JavaSparkContext sc) {

        int nBlocks = (int)(dataFromStep1ForStep2_RDD.count());

        List<Tuple2<Integer, DataCollection>> dataFromStep1ForStep2_List = dataFromStep1ForStep2_RDD.collect();

        /* Create an algorithm to compute QR decomposition on the master node */
        DistributedStep2Master qrStep2Master = new DistributedStep2Master( context, Double.class, Method.defaultDense );

        for (Tuple2<Integer, DataCollection> value : dataFromStep1ForStep2_List) {
            value._2.unpack(context);
            qrStep2Master.input.add( DistributedStep2MasterInputId.inputOfStep2FromStep1, value._1, value._2 );
        }

        /* Compute QR decomposition in step 2 */
        DistributedStep2MasterPartialResult pres = qrStep2Master.compute();

        KeyValueDataCollection inputForStep3FromStep2 = pres.get( DistributedPartialResultCollectionId.outputOfStep2ForStep3 );

        List<Tuple2<Integer, DataCollection>> list = new ArrayList<Tuple2<Integer, DataCollection>>(nBlocks);
        for (Tuple2<Integer, DataCollection> value : dataFromStep1ForStep2_List) {
            DataCollection dc = (DataCollection) inputForStep3FromStep2.get( value._1 );
            dc.pack();
            list.add(new Tuple2<Integer, DataCollection>(value._1, dc));
        }

        /* Make PairRDD from the list */
        dataFromStep2ForStep3_RDD = JavaPairRDD.fromJavaRDD( sc.parallelize(list, nBlocks) );

        Result res = qrStep2Master.finalizeCompute();

        HomogenNumericTable ntR = (HomogenNumericTable)res.get( ResultId.matrixR );
        result.R = ntR;
    }

    private static void computeStep3Local() {
        /* Group partial results from steps 1 and 2 */
        JavaPairRDD<Integer, Tuple2<Iterable<DataCollection>, Iterable<DataCollection>>> dataForStep3_RDD =
            dataFromStep1ForStep3_RDD.cogroup(dataFromStep2ForStep3_RDD);

        JavaPairRDD<Integer, HomogenNumericTable> ntQ_RDD = dataForStep3_RDD.mapToPair(
        new PairFunction<Tuple2<Integer, Tuple2<Iterable<DataCollection>, Iterable<DataCollection>>>, Integer, HomogenNumericTable>() {

            public Tuple2<Integer, HomogenNumericTable>
            call(Tuple2<Integer, Tuple2<Iterable<DataCollection>, Iterable<DataCollection>>> tup) {
                DaalContext context = new DaalContext();

                DataCollection ntQPi = tup._2._1.iterator().next();
                ntQPi.unpack(context);
                DataCollection ntPi  = tup._2._2.iterator().next();
                ntPi.unpack(context);

                /* Create an algorithm to compute QR decomposition on the master node */
                DistributedStep3Local qrStep3Local = new DistributedStep3Local(context, Double.class, Method.defaultDense);
                qrStep3Local.input.set( DistributedStep3LocalInputId.inputOfStep3FromStep1, ntQPi );
                qrStep3Local.input.set( DistributedStep3LocalInputId.inputOfStep3FromStep2, ntPi );

                /* Compute QR decomposition in step 3 */
                qrStep3Local.compute();
                Result result = qrStep3Local.finalizeCompute();

                HomogenNumericTable Qi = (HomogenNumericTable)result.get( ResultId.matrixQ );
                Qi.pack();

                context.dispose();

                return new Tuple2<Integer, HomogenNumericTable>(tup._1, Qi);
            }
        });
        result.Q = ntQ_RDD;
    }
}