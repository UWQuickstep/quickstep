set -e

SCALE=10
LOC=`pwd`"/tpch_$SCALE"

if [ ! -d $LOC ]; then
  mkdir $LOC
  cp dbgen/dists.dss $LOC
fi

pushd .
cd dbgen
make
DBGEN=`pwd`/dbgen
cd $LOC
$DBGEN -s $SCALE
popd
