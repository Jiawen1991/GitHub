/* file: LowOrderMomentsSparseStep1Mapper.java */
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
 */

package DAAL;

import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.BufferedWriter;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.util.Arrays;

import org.apache.hadoop.fs.*;
import org.apache.hadoop.io.*;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Mapper.Context;
import org.apache.hadoop.conf.Configuration;

import com.intel.daal.data_management.data.HomogenNumericTable;
import com.intel.daal.data_management.data.CSRNumericTable;
import com.intel.daal.algorithms.low_order_moments.*;
import com.intel.daal.data_management.data.*;
import com.intel.daal.services.*;

public class LowOrderMomentsSparseStep1Mapper extends Mapper<Object, Text, IntWritable, WriteableData> {

    /* Index is supposed to be a sequence number for the split */
    private int index = 0;
    private int totalTasks = 0;

    @Override
    public void setup(Context context) {
        index = context.getTaskAttemptID().getTaskID().getId();
        Configuration conf = context.getConfiguration();
        totalTasks = conf.getInt("mapred.map.tasks", 0);
    }

    @Override
    public void map(Object key, Text value,
                    Context context) throws IOException, InterruptedException {

        DaalContext daalContext = new DaalContext();

        /* Read a data set */
        String filePath = "/Hadoop/LowOrderMomentsSparse/data/" + value;

        CSRNumericTable ntData = createSparseTable(daalContext, filePath);

        /* Create an algorithm to compute low order moments on local nodes */
        DistributedStep1Local momentsLocal = new DistributedStep1Local(daalContext, Double.class, Method.fastCSR);
        momentsLocal.input.set( InputId.data, ntData );

        /* Compute low order moments on local nodes */
        PartialResult pres = momentsLocal.compute();

        /* Write the data prepended with a data set sequence number. Needed to know the position of the data set in the input data */
        context.write(new IntWritable(0), new WriteableData(index, pres));

        daalContext.dispose();
        index += totalTasks;
    }

    public static CSRNumericTable createSparseTable(DaalContext daalContext, String dataset) throws IOException {
        Path pt = new Path(dataset);
        FileSystem fs = FileSystem.get(new Configuration());
        BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(fs.open(pt)));

        String rowIndexLine = bufferedReader.readLine();
        int nVectors = getRowLength(rowIndexLine);
        long[] rowOffsets = new long[nVectors];

        readRow(rowIndexLine, 0, nVectors, rowOffsets);
        nVectors = nVectors - 1;

        String columnsLine = bufferedReader.readLine();
        int nCols = getRowLength(columnsLine);

        long[] colIndices = new long[nCols];
        readRow(columnsLine, 0, nCols, colIndices);

        String valuesLine = bufferedReader.readLine();
        int nNonZeros = getRowLength(valuesLine);

        double[] data = new double[nNonZeros];
        readRow(valuesLine, 0, nNonZeros, data);

        long maxCol = 0;
        for(int i = 0; i < nCols; i++) {
            if(colIndices[i] > maxCol) {
                maxCol = colIndices[i];
            }
        }
        int nFeatures = (int)maxCol;

        if (nCols != nNonZeros || nNonZeros != (rowOffsets[nVectors] - 1) || nFeatures == 0 || nVectors == 0) {
            throw new IOException("Unable to read input dataset");
        }

        return new CSRNumericTable(daalContext, data, colIndices, rowOffsets, nFeatures, nVectors);
    }

    private static int getRowLength(String line) {
        String[] elements = line.split(",");
        return elements.length;
    }

    public static void readRow(String line, int offset, int nCols, double[] data) throws IOException {
        if (line == null) {
            throw new IOException("Unable to read input dataset");
        }

        String[] elements = line.split(",");
        for (int j = 0; j < nCols; j++) {
            data[offset + j] = Double.parseDouble(elements[j]);
        }
    }

    public static void readRow(String line, int offset, int nCols, long[] data) throws IOException {
        if (line == null) {
            throw new IOException("Unable to read input dataset");
        }

        String[] elements = line.split(",");
        for (int j = 0; j < nCols; j++) {
            data[offset + j] = Long.parseLong(elements[j]);
        }
    }
}
