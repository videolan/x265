#!/bin/bash

if test x"$1" = x"-h" -o x"$1" = x"--help" ; then
cat <<EOF
Usage: ./Build [options]

  Help:
  -h, --help                  print this message

  ./buildSolution AA   -- Build the Solution for All the below Make Files
  
  Example: ./buildSolution AA
  
  ./buildSolution SS <<Make Files>> <<Build Dir Name>>  Build any Specific Make Files
  
  Example: ./buildSolution SS "MSYS Makefiles" "MSYS"
  
  The following generators are available on this platform:
  
  MSYS Makefiles           	= Generates MSYS makefiles.
  MinGW Makefiles             	= Generates a make file for use with
  Unix Makefiles              	= Generates standard UNIX makefiles.
  VisualStudio 10            	= Generates Visual Studio 10 project files.
  VisualStudio 10 Win64      	= Generates Visual Studio 10 Win64 project files
  VisualStudio 11            	= Generates Visual Studio 11 project files.
  VisualStudio 11 Win64      	= Generates Visual Studio 11 Win64 project files
  VisualStudio 9 2008        	= Generates Visual Studio 9 2008 project files.
  VisualStudio 9 2008 Win64  	= Generates Visual Studio 9 2008 Win64 project

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
 rm -rf ../MSYS
fi
cd ../  #Go back into Build (Working Directory


#Build Solution for MS Visual Studio 11 Win64

echo "Building Solution For MS Visual Studio 11 ....... "

mkdir MSVisualStudio11Win64
cd MSVisualStudio11Win64
cmake -G "Visual Studio 11 Win64" ../../source

if [ $? = 0 ] ; then
  echo " ********** BUILDED THE SOLUTION ****************************** "
 else
   echo " ********* SOLUTION NOT BUILDED CHECK COMP *******************"
 rm -rf ../MSVisualStudio11Win64
fi
cd ../ #Go back into Build (Working Directory 

#Building the Solution for MS Visual Studio 11

echo "Building Solution For MS Visual Studio 11 ......."

mkdir MSVisualStudio11
cd MSVisualStudio11
cmake -G "Visual Studio 11" ../../source

if [ $? = 0 ] ; then
    echo " ********** BUILDED THE SOLUTION ****************************** "
 else
   echo " ********* SOLUTION NOT BUILDED CHECK COMP *******************"
 rm -rf ../MSVisualStudio11
fi
cd ../ #Go back into Build (Working Directory)


#Build Solution for MS Visual Studio 10

echo "Building the Solution for MS Visual Studion 10 Win 64.......... "

mkdir MSVisualStudio10Win64
cd MSVisualStudio10Win64
cmake -G "Visual Studio 10 Win64" ../../source

if [ $? = 0 ] ; then
    echo " ********** BUILDED THE SOLUTION ****************************"
 else
   echo " ********* SOLUTION NOT BUILDED CHECK COMP *******************"
 rm -rf ../MSVisualStudio10Win64
fi
cd ../ #Go back into Build (Working Directory


# Building Solution for MS Visual Studion 10

echo "Building the Solution for MS Visual Studio 10 ...... "
mkdir MSVisualStudio10
cd MSVisualStudio10
cmake -G "Visual Studio 10 " ../../source

if [ $? = 0 ] ; then
    echo " ********** BUILDED THE SOLUTION ****************************** "
 else
   echo " ********* SOLUTION NOT BUILDED CHECK COMP *******************"
 rm -rf ../MSVisualStudio10
fi
cd ../ #Go back into Build (Working Directory

#Build Solution for MSVisual Studio 9

echo "Building the Solution for MS Visual Studio 2008 ............."

mkdir MSVisualStudio92008
cd MSVisualStudio92008
cmake -G "Visual Studio 9 2008" ../../source

if [ $? = 0 ] ; then
    echo " ********** BUILDED THE SOLUTION ****************************** "
 else
   echo " ********* SOLUTION NOT BUILDED CHECK COMP *******************"
 rm -rf ../MSVisualStudio92008
fi
cd ../ #Go back into Build (Working Directory)


# Building Solution for MS Visual Studion 2008 win 64

echo "Building the Solution for Visual Studion Win 64 ........... "

mkdir MSVisualStudio92008Win64
cd MSVisualStudio92008Win64
cmake -G "Visual Studio 9 2008 Win64 " ../../source

if [ $? = 0 ] ; then
    echo " ********** BUILDED THE SOLUTION ****************************** "
 else
   echo " ********* SOLUTION NOT BUILDED CHECK COMP *******************"
 rm -rf ../MSVisualStudio92008Win64
fi

cd ../ #Go back into Build (Working Directory)
exit
fi

#----------------  END FOR All SOLUTION BUILDING --------------------

# ------ BUILD THE SOLUTION FOR ANY SPECIFIC COMPILER -----------

if [ "$1" == "SS"  -a "$2" -a "$3" ] ; then

echo "Building  the Solution For $2 ....... "
mkdir "$3"
cd "$3"
cmake -G "$2" ../../source

if [ $? = 0 ] ; then
 echo " ********** BUILDED THE SOLUTION ****************************** "
 else
  echo " ********* SOLUTION NOT BUILDED CHECK COMP *******************"
 rm -rf ../MSVisualStudio92008Win64
fi

cd ../ # Go Back into the BUILD (Working Directory ) ...

exit

fi

echo "                                                  "
echo " ..... Please Run ./buildSolution -h  for More Options and Usage..... "
echo "                                                  "

exit
