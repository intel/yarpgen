#!/bin/bash

is_result_mounted=`mount | grep "$RESULT_DIR"`
if [ -z "is_result_mounted" ]
then
    echo "Output directory for the result is not mounted, so we can't save the result"
    echo "If you want to fix that, add '--mount source=yarpgen-result,target=/testing/result' to docker run"
    cd $YARPGEN_HOME/scripts
else
    # Create output dir
    # date-time format is the following: 20230123_1622 for 2023 Jan 23, 16:22.
    DATETIME=$(date '+%F_%H%M')
    OUT_DIR=$RESULT_DIR/$DATETIME-$HOST_HOSTNAME
    mkdir $OUT_DIR

    # Save revisions
    cp $GCC_HOME/gcc_rev.txt $OUT_DIR/gcc_rev.txt
    cp $LLVM_HOME/llvm_rev.txt $OUT_DIR/llvm_rev.txt
    cp $YARPGEN_HOME/yarpgen_rev.txt $OUT_DIR/yarpgen_rev.txt
    sde --version > $OUT_DIR/sde_version.txt

    # Test
    cd $YARPGEN_HOME/scripts
    nice -n 10 ./run_gen.py --output $OUT_DIR --log-file $OUT_DIR/log.txt $@
fi

