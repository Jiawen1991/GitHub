/* file: SVDStep2Reducer.java */
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

import java.io.OutputStreamWriter;
import java.io.BufferedWriter;
import java.io.PrintWriter;
import java.io.IOException;
import java.util.Iterator;
import java.util.List;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.Set;

import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.*;
import org.apache.hadoop.fs.FileSystem;

import com.intel.daal.data_management.data.HomogenNumericTable;
import com.intel.daal.algorithms.svd.*;
import com.intel.daal.data_management.data.*;
import com.intel.daal.services.*;

public class SVDStep2Reducer extends Reducer<IntWritable, WriteableData, IntWritable, WriteableData> {

    private static final int nFeatures = 18;
    private static final int nVectors = 16000;
    private static final int nVectorsInBlock = 4000;
    private int nBlocks;
    private static Configuration config;
    private int index = 0;
    private int totalTasks = 0;

    @Override
    public void setup(Context context) {
        config = context.getConfiguration();
        totalTasks = config.getInt("mapred.map.tasks", 0);
        index = context.getTaskAttemptID().getTaskID().getId();
    }

    @Override
    public void reduce(IntWritable key, Iterable<WriteableData> values, Context context)
    throws IOException, InterruptedException {

        List<Integer> knownIds = new ArrayList<Integer>();

        DaalContext daalContext = new DaalContext();

        /* Create an algorithm to compute SVD on the master node */
        DistributedStep2Master svdStep2Master = new DistributedStep2Master( daalContext, Double.class, Method.defaultDense );

        for (WriteableData value : values) {
            DataCollection dc = (DataCollection)value.getObject(daalContext);
            svdStep2Master.input.add( DistributedStep2MasterInputId.inputOfStep2FromStep1, value.getId(), dc );
            knownIds.add( value.getId() );
        }

        /* Compute SVD in step 2 */
        DistributedStep2MasterPartialResult pres = svdStep2Master.compute();

        KeyValueDataCollection inputForStep3FromStep2 = pres.get( DistributedPartialResultCollectionId.outputOfStep2ForStep3 );

        for (Integer value : knownIds) {
            DataCollection dc = (DataCollection)inputForStep3FromStep2.get( value.intValue() );
            context.write(new IntWritable( value.intValue() ),
                          new WriteableData( value.intValue(), dc ));
        }

        Result res = svdStep2Master.finalizeCompute();
        NumericTable sigma = res.get( ResultId.singularValues );
        NumericTable v     = res.get( ResultId.rightSingularMatrix );

        SequenceFile.Writer writer = SequenceFile.createWriter(
                                         new Configuration(),
                                         SequenceFile.Writer.file(new Path("/Hadoop/SVD/Output/Sigma")),
                                         SequenceFile.Writer.keyClass(IntWritable.class),
                                         SequenceFile.Writer.valueClass(WriteableData.class));
        writer.append(new IntWritable(0), new WriteableData( sigma ));
        writer.close();

        writer = SequenceFile.createWriter(
                     new Configuration(),
                     SequenceFile.Writer.file(new Path("/Hadoop/SVD/Output/V")),
                     SequenceFile.Writer.keyClass(IntWritable.class),
                     SequenceFile.Writer.valueClass(WriteableData.class));
        writer.append(new IntWritable(0), new WriteableData( v ));
        writer.close();

        daalContext.dispose();
    }
}
