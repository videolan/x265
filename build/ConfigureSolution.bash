#!/bin/bash

if test x"$1" = x"-h" -o x"$1" = x"--help" ; then
cat <<EOF
Usage: ./ConfigureSolution.bash [options]

  Help:
  -h, --help                  print this message

  ./ConfigureSolution.bash AA   -- Build the Solution for All the below Make Files
  
  Example: ./ConfigureSolution.bash AA
  
  ./ConfigureSolution.bash SS <<Make Files>> <<Build Dir Name>>  Build any Specific Make Files
  
  Example: ./ConfigureSolution.bash SS "MSYS Makefiles" "MSYS"
  
  The following generators are available on this platform:
  
  MSYS Makefiles           	= Generates MSYS makefiles.
  MinGW Makefiles             	= Generates a make file for use with
  Unix Makefiles              	= Generates standard UNIX makefiles.
 

EOF
exit 1

fi

if [ "$1" == "ALL" ] ; then
echo "Building Solution For MSYS ..... "
mkdir MSYS 
cd MSYS
cmake -G "MSYS Makefiles" ../../source
if [ $? = 0 ] ; then
 echo " ********** BUILDED THE SOLUTION ****************************** "
 else
 echo " ********* SOLUTION NOT BUILDED CHECK COMP *******************"
fi
cd ../  #Go back into Build (Working Directory


#Build Solution for MS Visual Studio 11 Win64

echo "Building Solution For MinGW ....... "
mkdir MinGW
cd MinGW
cmake -G "MinGW Makefiles" ../../source
if [ $? = 0 ] ; then
  echo " ********** BUILDED THE SOLUTION ****************************** "
 else
  echo " ********* SOLUTION NOT BUILDED CHECK COMP *******************"
fi
cd ../ #Go back into Build (Working Directory 

#Building the Solution for Unix Makefiles

echo "Building Solution For Unix Makefiles ......."
mkdir Unix
cd Unix
cmake -G "Unix Makefiles" ../../source
if [ $? = 0 ] ; then
   echo " ********** BUILDED THE SOLUTION ****************************** "
 else
   echo " ********* SOLUTION NOT BUILDED CHECK COMP *******************"
fi
cd ../ #Go back into Build (Working Directory)
fi
#----------------  END FOR All SOLUTION BUILDING --------------------

# ---------------  BUILD THE SOLUTION FOR ANY SPECIFIC COMPILER -----

if [ "$1" == "SS"  -a "$2" -a "$3" ] ; then
echo "Building  the Solution For $2 ....... "
mkdir "$3"
cd "$3"
cmake -G "$2" ../../source

if [ $? = 0 ] ; then
 echo " ********** BUILDED THE SOLUTION ****************************** "
 else
 echo " ********* SOLUTION NOT BUILDED CHECK COMP *******************"
fi
cd ../ # Go Back into the BUILD (Working Directory ) ...
exit
fi

echo " FOR HELP AND USAGE : -   ./ConfigureSolution.bash -h "
exit



