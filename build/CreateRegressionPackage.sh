#!/bin/sh

CONFIG_FILE=config.txt

if [[ -f $CONFIG_FILE ]]; then
        . $CONFIG_FILE
fi

HG=hg
CWD=$WD/RegressionTest
LOG=$CWD/RegressionTester.log


if [ ! -d $CWD ]
	then 
		mkdir $CWD
		if [ $? -ne 0 ] 
			then	
				echo -e "Unable to create Working directory" >> $LOG
				exit
		fi
		echo "Working directory created\n" >> $LOG
		cd $CWD
		
		$HG init
		$HG pull $repository
		$HG update
	
		if [ $? -ne 0 ] 
			then	
				echo -e "Unable to pull from $REPOSITORY" >> $LOG
				exit
		fi
		echo "Pull successfull from $REPOSITORY\n" >> $LOG		
fi


## Building for msys

BUILDDIR=$CWD/build/msys

cd $BUILDDIR
mkdir enabled
cd enabled

echo  "SET(ENABLE_TESTS ON CACHE BOOL \"Enable Unit Tests\" FORCE )" >> enablecache.txt
echo  "SET(HIGH_BIT_DEPTH ON CACHE BOOL \"Use 16bit pixels internally\" FORCE )" >> enablecache.txt
echo  "SET(ENABLE_PRIMITIVES ON CACHE BOOL \"Enable use of optimized encoder primitives\" FORCE ) " >> enablecache.txt

cmake -G "MSYS Makefiles" -C enablecache.txt ../../../source
make all
if [ $? -ne 0 ] 
	then	
		echo -e "Unable to build for MSYS\n" >> $LOG
fi
echo -e "Build successfull for MSYS\n" >> $LOG
echo -e "Running the encoder\n" >> $LOG
./x265-cli.exe -c $CWD/cfg/encoder_all_I.cfg -c $CWD/cfg/per-sequence/BasketballDrive.cfg -f 3 -i $WD/$VIDEO >> ExecLog.txt
./test/TestBench.exe >> test/TestBenchLog.txt

cd $BUILDDIR
mkdir disabled
cd disabled

echo "SET(ENABLE_TESTS ON CACHE BOOL \"Enable Unit Tests\" FORCE )" >> disablecache.txt
echo "SET(ENABLE_PRIMITIVES OFF CACHE BOOL \"Use 16bit pixels internally\" FORCE )" >> disablecache.txt
echo "SET(ENABLE_PRIMITIVES_VEC OFF CACHE BOOL \"Enable use of optimized encoder primitives\" FORCE )" >> disablecache.txt

cmake -G "MSYS Makefiles" -C disablecache.txt ../../../source
make all
if [ $? -ne 0 ] 
	then	
		echo -e "Unable to build for MSYS, primitives disabled\n" >> $LOG
fi
echo -e "Build successfull for MSYS, primitives disabled\n" >> $LOG
echo -e "Running the encoder\n" >> $LOG
./x265-cli.exe -c $CWD/cfg/encoder_all_I.cfg -c $CWD/cfg/per-sequence/BasketballDrive.cfg -f 3 -i $WD/$VIDEO >> ExecLog.txt
./test/TestBench.exe >> test/TestBenchLog.txt


## Building for linux

BUILDDIR=$CWD/build/linux

cd $BUILDDIR
mkdir enabled
cd enabled

echo "SET(ENABLE_TESTS ON CACHE BOOL \"Enable Unit Tests\" FORCE )" >> enablecache.txt
echo "SET(HIGH_BIT_DEPTH ON CACHE BOOL \"Use 16bit pixels internally\" FORCE )" >> enablecache.txt
echo "SET(ENABLE_PRIMITIVES ON CACHE BOOL \"Enable use of optimized encoder primitives\" FORCE )" >> enablecache.txt

cmake -G "Unix Makefiles" -C enablecache.txt ../../../source
make all
if [ $? -ne 0 ] 
	then	
		echo -e "Unable to build for Unix\n" >> $LOG
fi
echo -e "Build successfull for Unix\n" >> $LOG
echo -e "Running the encoder\n" >> $LOG
./x265-cli.exe -c $CWD/cfg/encoder_all_I.cfg -c $CWD/cfg/per-sequence/BasketballDrive.cfg -f 3 -i $WD/$VIDEO >> ExecLog.txt
./test/TestBench.exe >> test/TestBenchLog.txt


cd $BUILDDIR
mkdir disabled
cd disabled

echo "SET(ENABLE_TESTS ON CACHE BOOL \"Enable Unit Tests\" FORCE )" >> disablecache.txt
echo "SET(ENABLE_PRIMITIVES OFF CACHE BOOL \"Use 16bit pixels internally\" FORCE )" >> disablecache.txt
echo "SET(ENABLE_PRIMITIVES_VEC OFF CACHE BOOL \"Enable use of optimized encoder primitives\" FORCE )" >> disablecache.txt

cmake -G "Unix Makefiles" -C disablecache.txt ../../../source
make all
if [ $? -ne 0 ] 
	then	
		echo -e "Unable to build for Unix, primitives disabled\n" >> $LOG
fi
echo -e "Build successfull for Unix, primitives disabled\n" >> $LOG
echo -e "Running the encoder\n" >> $LOG
./x265-cli.exe -c $CWD/cfg/encoder_all_I.cfg -c $CWD/cfg/per-sequence/BasketballDrive.cfg -f 3 -i $WD/$VIDEO >> ExecLog.txt
./test/TestBench.exe >> test/TestBenchLog.txt


