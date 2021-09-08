#!/bin/bash

is_result_mounted=`mount | grep "$RESULT_DIR"`
if [ -z "is_result_mounted" ]
then
    echo "Output directory for the result is not mounted, so we can't save the result"
    cd $YARPGEN_HOME/scripts
else
    # Create output dir
    DATE=$(date '+%m-%d-%y')
    HOST_HOSTNAME=ohm
    OUT_DIR=$RESULT_DIR/$DATE-$HOST_HOSTNAME
    mkdir $OUT_DIR

    # Save revisions
    cp $GCC_HOME/gcc_rev.txt $OUT_DIR/gcc_rev.txt
    cp $LLVM_HOME/llvm_rev.txt $OUT_DIR/llvm_rev.txt
    cp $YARPGEN_HOME/yarpgen_rev.txt $OUT_DIR/yarpgen_rev.txt
    sde --version > $OUT_DIR/sde_version.txt

    # Test
    cd $YARPGEN_HOME/scripts
    ./run_gen.py --output $OUT_DIR --log-file $OUT_DIR/log.txt
fi
