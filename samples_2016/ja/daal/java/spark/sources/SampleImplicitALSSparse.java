/* file: SampleImplicitALSSparse.java */
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
 //     Java sample of the implicit alternating least squares (ALS) algorithm.
 //
 //     The program trains the implicit ALS trainedModel on a supplied training data
 //     set.
 ////////////////////////////////////////////////////////////////////////////////
 */

package DAAL;

import java.util.List;
import java.nio.DoubleBuffer;
import java.io.IOException;
import java.lang.ClassNotFoundException;

import org.apache.spark.api.java.*;
import org.apache.spark.api.java.function.*;
import org.apache.spark.SparkConf;

import scala.Tuple2;
import scala.Tuple3;

import com.intel.daal.data_management.data.*;
import com.intel.daal.data_management.data_source.*;
import com.intel.daal.algorithms.implicit_als.*;
import com.intel.daal.algorithms.implicit_als.training.*;
import com.intel.daal.algorithms.implicit_als.prediction.ratings.*;
import com.intel.daal.services.*;

public class SampleImplicitALSSparse {
    public static void main(String[] args) throws IOException, ClassNotFoundException {
        DaalContext context = new DaalContext();

        /* Create JavaSparkContext that loads defaults from the system properties and the classpath and sets the name */
        JavaSparkContext sc = new JavaSparkContext(new SparkConf().setAppName("Spark Implicit ALS"));

        /* Read from the distributed HDFS data set at a specified path */
        StringDataSource templateDataSource = new StringDataSource(context, "");

        DistributedHDFSDataSet ddTrain = new DistributedHDFSDataSet("/Spark/ImplicitALSSparse/data/ImplicitALSSparse_*",
                templateDataSource);
        DistributedHDFSDataSet ddTrainTransposed = new DistributedHDFSDataSet("/Spark/ImplicitALSSparse/data/ImplicitALSSparseTrans_*",
                templateDataSource );
        JavaPairRDD<Integer, CSRNumericTable> dataRDD = ddTrain.getCSRAsPairRDDWithIndex(sc);
        JavaPairRDD<Integer, CSRNumericTable> transposedDataRDD = ddTrainTransposed.getCSRAsPairRDDWithIndex(sc);

        SparkImplicitALSSparse.TrainingResult trainedModel = SparkImplicitALSSparse.trainModel(sc, dataRDD, transposedDataRDD);
        printTrainedModel(trainedModel);

        JavaRDD<Tuple3<Integer, Integer, RatingsResult>> predictedRatings =
            SparkImplicitALSSparse.testModel(trainedModel.usersFactors, trainedModel.itemsFactors);
        printPredictedRatings(predictedRatings);

        context.dispose();
    }

    public static void printTrainedModel(SparkImplicitALSSparse.TrainingResult trainedModel) {
        DaalContext context = new DaalContext();
        List<Tuple2<Integer, DistributedPartialResultStep4>> itemsFactorsList = trainedModel.itemsFactors.collect();
        for (Tuple2<Integer, DistributedPartialResultStep4> tup : itemsFactorsList) {
            tup._2.unpack(context);
            printNumericTable("Partial items factors " + tup._1 + " :",
                              tup._2.get(DistributedPartialResultStep4Id.outputOfStep4ForStep1).getFactors());
            tup._2.pack();
        }

        List<Tuple2<Integer, DistributedPartialResultStep4>> usersFactorsList = trainedModel.usersFactors.collect();
        for (Tuple2<Integer, DistributedPartialResultStep4> tup : usersFactorsList) {
            tup._2.unpack(context);
            printNumericTable("Partial users factors " + tup._1 + " :",
                              tup._2.get(DistributedPartialResultStep4Id.outputOfStep4ForStep1).getFactors());
            tup._2.pack();
        }
        context.dispose();
    }

    public static void printPredictedRatings(JavaRDD<Tuple3<Integer, Integer, RatingsResult>> predictedRatings) {
        DaalContext context = new DaalContext();
        List<Tuple3<Integer, Integer, RatingsResult>> predictedRatingsList = predictedRatings.collect();
        for (Tuple3<Integer, Integer, RatingsResult> tup : predictedRatingsList) {
            tup._3().unpack(context);
            printNumericTable("Ratings [" + tup._1() + ", " + tup._2() + "]" , tup._3().get(RatingsResultId.prediction));
        }
        context.dispose();
    }

    public static void printNumericTable(String header, NumericTable nt, long nPrintedRows, long nPrintedCols) {
        long nNtCols = nt.getNumberOfColumns();
        long nNtRows = nt.getNumberOfRows();
        long nRows = nNtRows;
        long nCols = nNtCols;

        if (nPrintedRows > 0) {
            nRows = Math.min(nNtRows, nPrintedRows);
        }

        DoubleBuffer result = DoubleBuffer.allocate((int) (nNtCols * nRows));
        result = nt.getBlockOfRows(0, nRows, result);

        if (nPrintedCols > 0) {
            nCols = Math.min(nNtCols, nPrintedCols);
        }

        StringBuilder builder = new StringBuilder();
        builder.append(header);
        builder.append("\n");
        for (long i = 0; i < nRows; i++) {
            for (long j = 0; j < nCols; j++) {
                String tmp = String.format("%-6.3f   ", result.get((int) (i * nNtCols + j)));
                builder.append(tmp);
            }
            builder.append("\n");
        }
        System.out.println(builder.toString());
    }

    public static void printNumericTable(String header, NumericTable nt, long nRows) {
        printNumericTable(header, nt, nRows, nt.getNumberOfColumns());
    }

    public static void printNumericTable(String header, NumericTable nt) {
        printNumericTable(header, nt, nt.getNumberOfRows());
    }
}
