#!/bin/sh
# This script will make kernels, and then package them for release.

PRODUCT1=KangBang-Tuna
# PRODUCT2=<insert product name>
# PRODUCT3=<insert product name>

T=$PWD

echo "Gon' make some kernels!"

# Clean up out files from last build if they exist
echo "Cleaning up"
rm -rf $T/out
mkdir -p $T/out

# Product 1 setup

echo "Now building $PRODUCT1 and packaging"
./build-scripts/$PRODUCT1.sh

if [ -f $T/out/$PRODUCT1/kernel/zImage ];
    then
        echo "$PRODUCT1 found"
    else
        echo "$PRODUCT1 did not compile successfully, please fix your shit"
fi

# Product 2 setup

#echo "Now building $PRODUCT2 and packaging"
#./build-scripts/$PRODUCT2.sh

#if [ -f $T/out/$PRODUCT2/kernel/zImage ];
 #   then
  #      echo "$PRODUCT2 found"
   # else
    #    echo "$PRODUCT2 did not compile successfully, please fix your shit"
#fi

# Product 3 setup

# echo "Now building $PRODUCT3 and packaging"
# ./build-scripts/$PRODUCT3.sh

# if [ -f $T/out/$PRODUCT3/kernel/zImage ];
#     then
#         echo "$PRODUCT3 found"
#     else
#         echo "$PRODUCT3 did not compile successfully, please fix your shit"
#         exit 0
# fi

echo "All kernels are done..."
