/* file: SampleCovarianceDense.java */
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
 //     Java sample of dense variance-covariance matrix computation
 ////////////////////////////////////////////////////////////////////////////////
 */

package DAAL;

import java.util.ArrayList;
import java.util.List;
import java.util.Arrays;
import java.util.Iterator;
import java.util.Map;

import org.apache.spark.api.java.*;
import org.apache.spark.api.java.function.*;
import org.apache.spark.SparkConf;

import scala.Tuple2;

import com.intel.daal.data_management.data.*;
import com.intel.daal.data_management.data_source.*;
import com.intel.daal.services.*;

public class SampleCovarianceDense {
    public static void main(String[] args) {
        DaalContext context = new DaalContext();

        /* Create JavaSparkContext that loads defaults from the system properties and the classpath and sets the name */
        JavaSparkContext sc = new JavaSparkContext(new SparkConf().setAppName("Spark covariance(dense)"));

        /* Read from the distributed HDFS data set at a specified path */
        StringDataSource templateDataSource = new StringDataSource( context, "" );
        DistributedHDFSDataSet dd = new DistributedHDFSDataSet( "/Spark/CovarianceDense/data/", templateDataSource );
        JavaPairRDD<Integer, HomogenNumericTable> dataRDD = dd.getAsPairRDD(sc);

        /* Compute a dense variance-covariance matrix for dataRDD */
        SparkCovarianceDense.CovarianceResult result = SparkCovarianceDense.runCovariance(context, dataRDD);

        /* Print the results */
        HomogenNumericTable Covariance  = result.covariance;
        HomogenNumericTable Mean = result.mean;

        printNumericTable("Covariance:",  Covariance);
        printNumericTable("Mean:", Mean);

        context.dispose();
    }

    private static void printNumericTable(String header, HomogenNumericTable nt) {
        long nRows = nt.getNumberOfRows();
        long nCols = nt.getNumberOfColumns();
        double[] result = nt.getDoubleArray();

        int resultIndex = 0;
        StringBuilder builder = new StringBuilder();
        builder.append(header);
        builder.append("\n");
        for (long i = 0; i < nRows; i++) {
            for (long j = 0; j < nCols; j++) {
                String tmp = String.format("%-6.3f   ", result[resultIndex++]);
                builder.append(tmp);
            }
            builder.append("\n");
        }
        System.out.println(builder.toString());
    }
}
