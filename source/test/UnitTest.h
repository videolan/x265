#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <fstream>

//Error Codes
#define WRONG_SIZE    -1
#define WRONG_BUFFER  -2
#define NOT_MATCHED   1
#define MATCHED     0
#define FILE_OPEN_ERROR -3
#define FILE_READ_ERROR -4
#define FILEWRITE_ERROR -5

using namespace std;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

//UnitTest Class
class UnitTest
{
private:

    fstream   fhandle;                                      ///< file handle
    int fSize;

public:

    UnitTest()   {}

    ~UnitTest()   {}

    static int CompareBuffer(unsigned char *Buff_one, unsigned char *Buff_two);
    static int CompareFiles(char *file1, char *file2);
    static int DumpBuffer(unsigned char *Buffer, char *Filename);
    static int CompareYUVOutputFile(char *file1, char *file2);
    static int CompareYUVBuffer(unsigned char *Buff_old, unsigned char *Buff_new);
};
