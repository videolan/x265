/***************************  decimal.h   *************************************
| Author:        Agner Fog
| Date created:  2012-07-08
| Last modified: 2012-07-08
| Version:       1.01 Beta
| Project:       vector classes
| Description:
| Functions for conversion between binary number vectors and Binary Coded 
| Decimal (BCD), decimal ASCII, hexadecimal ASCII, etc.
|
| This file should only be included if it is needed - it takes a long time to compile
|
| (c) Copyright 2012 GNU General Public License http://www.gnu.org/licenses
\*****************************************************************************/


#include "vectorclass.h"

/*****************************************************************************
*
*               Conversion from binary to BCD
*
*****************************************************************************/

// convert from binary to BCD, for values 0 - 99
// no check for overflow
static inline Vec16uc bin2bcd (Vec16uc const & a) {
    // masks used in double-dabble algorithm
    Vec16uc mask3(0x30), mask8(0x80);    
    Vec16uc x = a;
    for (int p = 4; p > 0; p--) {
        // find nibbles that are bigger than 4
        // (adding 3 will not overflow into next nibble because BCD nibbles are < 10)
        Vec16uc bigger4 = (x + mask3) & mask8;
        // generate value 3 for nibbles that are > 4
        Vec16uc add3 = (bigger4 >> 2) + (bigger4 >> 3);
        // add 3 to generate carry when shifting left
        x += add3;
        // shift masks right instead of shifting x left
        mask3 = mask3 >> 1;  mask8 = mask8 >> 1;
    }
    return x;
}

// convert from binary to BCD, for values 0 - 9999
// no check for overflow
static inline Vec8us bin2bcd (Vec8us const & a) {
    // split into halves by dividing by 100
    Vec8us thi = a / const_uint(100);  // a / 100
    Vec8us tlo = a - thi * 100;        // a % 100
    Vec8us split = (thi << 8) | tlo;
    // convert each half to BCD
    return Vec8us(bin2bcd(Vec16uc(split)));
}
/*
// same function using double-dabble algorithm. May be useful on other platforms
static inline Vec8us bin2bcd (Vec8us const & a) {
    Vec8us x = a;
    Vec8us mask3(0x3333), mask8(0x8888);
    for (int p = 10; p > 0; p--) {
        // find nibbles that are bigger than 4
        // (adding 3 will not overflow into next nibble because BCD nibbles are < 10)
        Vec8us bigger4 = (x + (mask3<<p)) & (mask8<<p);
        // generate value 3 for nibbles that are > 4
        Vec8us add3 = (bigger4 >> 2) + (bigger4 >> 3);
        // add 3 to generate carry when shifting left
        x += add3;
    }
    return x;
} */

// convert from binary to BCD, for values 0 - 99999999
// no check for overflow
static inline Vec4ui bin2bcd (Vec4ui const & a) {
    // split into halves by dividing by 10000
    Vec4ui thi = a / const_uint(10000);  // a / 10000
    Vec4ui tlo = a - thi * 10000;        // a % 10000
    Vec4ui split = (thi << 16) | tlo;
    // convert each half to BCD
    return Vec4ui(bin2bcd(Vec8us(split)));
}

// convert from binary to BCD, for values 0 - 9999999999999999
// no check for overflow
static inline Vec2uq bin2bcd (Vec2uq const & a) {
    // split into halves by dividing by 100000000
    uint64_t t[2];
    a.store(t);
    Vec4ui split(uint32_t(t[0]%100000000u), uint32_t(t[0]/100000000u), 
                 uint32_t(t[1]%100000000u), uint32_t(t[1]/100000000u));
    // convert each half to BCD
    return Vec2uq(bin2bcd(split));
}

// convert from binary to BCD, for values 0 - 99
// no check for overflow
static inline Vec32uc bin2bcd (Vec32uc const & a) {
    // masks used in double-dabble algorithm
    Vec32uc x = a;
    Vec32uc mask3(0x30), mask8(0x80);
    for (int p = 4; p > 0; p--) {
        // find nibbles that are bigger than 4
        // (adding 3 will not overflow into next nibble because BCD nibbles are < 10)
        Vec32uc bigger4 = (x + mask3) & mask8;
        // generate value 3 for nibbles that are > 4
        Vec32uc add3 = (bigger4 >> 2) + (bigger4 >> 3);
        // add 3 to generate carry when shifting left
        x += add3;
        // shift masks right instead of shifting x left
        mask3 = mask3 >> 1;  mask8 = mask8 >> 1;
    }
    return x;
}

// convert from binary to BCD, for values 0 - 9999
// no check for overflow
static inline Vec16us bin2bcd (Vec16us const & a) {
    // split into halves by dividing by 100
    Vec16us thi = a / const_uint(100);  // a / 100
    Vec16us tlo = a - thi * 100;        // a % 100
    Vec16us split = (thi << 8) | tlo;
    // convert each half to BCD
    return Vec16us(bin2bcd(Vec32uc(split)));
}

// convert from binary to BCD, for values 0 - 99999999
// no check for overflow
static inline Vec8ui bin2bcd (Vec8ui const & a) {
    // split into halves by dividing by 10000
    Vec8ui thi = a / const_uint(10000);  // a / 10000
    Vec8ui tlo = a - thi * 10000;        // a % 10000
    Vec8ui split = (thi << 16) | tlo;
    // convert each half to BCD
    return Vec8ui(bin2bcd(Vec16us(split)));
}

// convert from binary to BCD, for values 0 - 9999999999999999
// no check for overflow
static inline Vec4uq bin2bcd (Vec4uq const & a) {
    // split into halves by dividing by 100000000
    uint64_t t[4];
    a.store(t);
    Vec8ui split(
        uint32_t(t[0]%100000000u), uint32_t(t[0]/100000000u), 
        uint32_t(t[1]%100000000u), uint32_t(t[1]/100000000u),
        uint32_t(t[2]%100000000u), uint32_t(t[2]/100000000u),
        uint32_t(t[3]%100000000u), uint32_t(t[3]/100000000u));
    // convert each half to BCD
    return Vec4uq(bin2bcd(split));
}


/*****************************************************************************
*
*               Conversion from binary to decimal ASCII string
*
*****************************************************************************/

// Convert binary numbers to decimal ASCII string.
// The numbers will be written to the string as decimal numbers in human-readable format.
// Each number will be right-justified with leading spaces in a field of the specified length.
static int bin2ascii (Vec4i const & a, char * string, int fieldlen = 8, int numdat = 4, bool signd = true, char ovfl = '*', char separator = ',', bool term = true) {
    // Parameters:
    // a         vector of 4 numbers to convert
    // string    buffer to receive ASCII string
    // fieldlen  string length of each converted number
    // numdat    number of data elements to output
    // signd     each number will be interpreted as signed if signd is true, unsigned if false. 
    //           Negative numbers will be indicated by a preceding '-'
    // ovfl      Output string will be filled with this character if the number is too big to fit in the field.
    //           The size of a field will be extended in case of overflow if ovfl = 0.
    // separator This character is inserted between data fields, but not after the last field.
    //           The separator character is not included in fieldlen. Separator = 0 for no separator.
    // term      The output string will have a terminating zero ('\0') if term is true.
    // Return value: The length of the written string is returned. The terminating zero is not included in the count.

    if (fieldlen <= 0 || (uint32_t)(numdat-1) > 4-1) {
        // write nothing but terminator
        if (term) *string = 0;
        return 0;
    }
    Vec4ui aa;               // a or abs(a)
    Vec4i  signa;            // sign of a
    Vec4q  signe;            // sign of a, extended
    Vec4i  ovfla;            // overflow of a
    Vec4q  ovfle;            // overflow of a, extended
    Vec16c space16(' ');     // 16 spaces
    int    numwrit;          // number of bytes written to string, not including terminating zero

    // limits depending on fieldlength
    static const int  limits[9] = {0,9,99,999,9999,99999,999999,9999999,99999999};
    int flen = fieldlen < 8 ? fieldlen : 8;
    if (signd) {                                                // signed
        aa    = abs(a);                                         // abs a
        signa = a >> 31;                                        // sign
        signe = Vec4q(extend_low(signa), extend_high(signa));   // sign, extended
        ovfla = (a > limits[flen]) | (a < -limits[flen-1]);     // overflow
    }
    else {                                                      // unsigned
        aa = a;
        ovfla = aa > limits[flen];                              // overflow
        signa = 0;
    }
    if (!(horizontal_or(ovfla) && (fieldlen > 8 || ovfl == 0))) {
        // normal case
        ovfle = Vec4q(extend_low(ovfla), extend_high(ovfla));   // overflow, extended
        Vec16uc bcd        = Vec16uc(bin2bcd(aa));              // converted to BCD
        Vec16uc bcd0246    = bcd & 0x0F;                        // low  nibbles of BCD code
        Vec16uc bcd1357    = bcd >> 4;                          // high nibbles of BCD code
        // interleave nibbles and reverse byte order
        Vec16c  declo      = blend16c<19, 3,18, 2,17, 1,16, 0, 23, 7,22, 6,21, 5,20, 4>(bcd0246, bcd1357);
        Vec16c  dechi      = blend16c<27,11,26,10,25, 9,24, 8, 31,15,30,14,29,13,28,12>(bcd0246, bcd1357);
        Vec32c  dec        = Vec32c(declo,dechi);               // all digits, big endian digit order
        Vec32c  ascii      = dec + 0x30;                        // add '0' to get ascii digits
        // find most significant nonzero digit, or digit 0 if all zero
        Vec32c decnz = (dec != 0) | Vec32c(0,0,0,0,0,0,0,-1,0,0,0,0,0,0,0,-1,0,0,0,0,0,0,0,-1,0,0,0,0,0,0,0,-1);
        Vec4q  scan  = Vec4q(decnz);
        scan |= scan << 8; scan |= scan << 16; scan |= scan << 32;
        // insert spaces to the left of most significant nonzero digit
        ascii = select(Vec32c(scan), ascii, Vec32c(' '));
        if (signd) {
            Vec32c minuspos = Vec32c(andnot(scan >> 8, scan)) & Vec32c(signe);  // position of minus sign
            ascii  = select(minuspos, Vec32c('-'), ascii);      // insert minus sign
        }
        // insert overflow indicator
        ascii = select(Vec32c(ovfle), Vec32c(ovfl), ascii);
        const int d = -256;  // means don't care in permute functions
        if (separator) {
            numwrit = (fieldlen + 1) * numdat - 1;
            // write output fields with separator
            Vec32c sep(separator);
            if (fieldlen <= 7) {
                switch (fieldlen) {
                case 1:
                    ascii = blend32c<7,32,15,32,23,32,31,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d>(ascii, sep);
                    break;
                case 2:
                    ascii = blend32c<6,7,32,14,15,32,22,23,32,30,31,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d>(ascii, sep);
                    break;
                case 3:
                    ascii = blend32c<5,6,7,32,13,14,15,32,21,22,23,32,29,30,31,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d>(ascii, sep);
                    break;
                case 4:
                    ascii = blend32c<4,5,6,7,32,12,13,14,15,32,20,21,22,23,32,28,29,30,31,d,d,d,d,d,d,d,d,d,d,d,d,d>(ascii, sep);
                    break;
                case 5:
                    ascii = blend32c<3,4,5,6,7,32,11,12,13,14,15,32,19,20,21,22,23,32,27,28,29,30,31,d,d,d,d,d,d,d,d,d>(ascii, sep);
                    break;
                case 6:
                    ascii = blend32c<2,3,4,5,6,7,32,10,11,12,13,14,15,32,18,19,20,21,22,23,32,26,27,28,29,30,31,d,d,d,d,d>(ascii, sep);
                    break;
                case 7:
                    ascii = blend32c<1,2,3,4,5,6,7,32,9,10,11,12,13,14,15,32,17,18,19,20,21,22,23,32,25,26,27,28,29,30,31,d>(ascii, sep);
                    break;
                }
                // store to string
                ascii.store_partial(numwrit, string);
                if (term) string[numwrit] = 0;
            }
            else {
                // fieldlen > 7
                int f;  // field counter
                int c;  // space counter
                // loop for each field
                for (f = 0; f < numdat; f++) {
                    // loop for multiples of 16 spaces
                    for (c = fieldlen - 8; c >= 16; c -= 16) {
                        space16.store(string);  string += 16;
                    }
                    // remaining < 16 spaces
                    space16.store_partial(c, string);  string += c;
                    // insert number (8 digits)
                    ascii.store(string);  string += 8;
                    if (f < numdat-1) {
                        // not last field. insert separator
                        *(string++) = separator;
                        // get next number into position 0
                        ascii = Vec32c(permute4q<1,2,3,d>(Vec4q(ascii)));
                    }
                }
                if (term) *string = 0;
            }
        }
        else {
            // write output fields without separator
            numwrit = fieldlen * numdat;
            if (fieldlen <= 8) {
                switch (fieldlen) {
                case 1:
                    ascii = permute32c<7,15,23,31,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d>(ascii);
                    break;
                case 2:
                    ascii = permute32c<6,7,14,15,22,23,30,31,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d>(ascii);
                    break;
                case 3:
                    ascii = permute32c<5,6,7,13,14,15,21,22,23,29,30,31,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d>(ascii);
                    break;
                case 4:
                    ascii = permute32c<4,5,6,7,12,13,14,15,20,21,22,23,28,29,30,31,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d>(ascii);
                    break;
                case 5:
                    ascii = permute32c<3,4,5,6,7,11,12,13,14,15,19,20,21,22,23,27,28,29,30,31,d,d,d,d,d,d,d,d,d,d,d,d>(ascii);
                    break;
                case 6:
                    ascii = permute32c<2,3,4,5,6,7,10,11,12,13,14,15,18,19,20,21,22,23,26,27,28,29,30,31,d,d,d,d,d,d,d,d>(ascii);
                    break;
                case 7:
                    ascii = permute32c<1,2,3,4,5,6,7,9,10,11,12,13,14,15,17,18,19,20,21,22,23,25,26,27,28,29,30,31,d,d,d,d>(ascii);
                    break;
                }
                // store to string
                ascii.store_partial(numwrit, string);
                if (term) string[numwrit] = 0;
            }
            else {
                // fieldlen > 8
                int f;                    // field counter
                int c;                    // space counter
                Vec16c space16(' ');      // 16 spaces
                // loop for each field
                for (f = 0; f < numdat; f++) {
                    // loop for multiples of 16 spaces
                    for (c = fieldlen - 8; c >= 16; c -= 16) {
                        space16.store(string);  string += 16;
                    }
                    // remaining < 16 spaces
                    space16.store_partial(c, string);  string += c;
                    // insert number (8 digits)
                    ascii.store_partial(8, string);  string += 8;
                    // get next number into position 0
                    ascii = Vec32c(permute4q<1,2,3,d>(Vec4q(ascii)));
                }
                if (term) *string = 0;
            }
        }
    }
    else {
        // two special cases are handled here by making one number at a time:
        // (1) more than 8 characters needed
        // (2) one or more fields need to be extended to handle overflow
        uint32_t aalist[4];  
        int32_t signlist[4];
        aa.store(aalist); signa.store(signlist);
        Vec4i x;
        int i;
        numwrit = 0;
        // loop for numdat data
        for (i = 0; i < numdat; i++) {
            x = permute4i<0,-1,-1,-1>(aa);                      // zero-extend abs(a)
            if (aalist[i] > 99999999u) {
                // extend into next field
                x = Vec4i(uint32_t(aalist[i])%100000000u, uint32_t(aalist[i])/100000000u, 0, 0);
            }
            Vec4ui bcd = bin2bcd(Vec4ui(x));                    // converted to BCD
            Vec16uc bcd0246    = Vec16uc(bcd) & 0x0F;           // low  nibbles of BCD code
            Vec16uc bcd1357    = Vec16uc(bcd) >> 4;             // high nibbles of BCD code
            // interleave nibbles and reverse byte order
            Vec16uc dec        = blend16uc<-1,-1,-1,-1,-1,-1,20,4,19,3,18,2,17,1,16,0 >(bcd0246, bcd1357);
            Vec16c  ascii      = dec + 0x30;                    // add '0' to get ascii digits
            // find most significant nonzero digit, or digit 0 if all zero
            Vec16c decnz = (dec != 0) | Vec16c(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1);
            Vec2q  scan  = Vec2q(decnz);
            scan |= scan << 8; scan |= scan << 16; scan |= scan << 32;
            // insert spaces to the left of most significant nonzero digit
            ascii = select(Vec16c(scan), ascii, Vec16c(' '));
            // count digits
            int charmask = _mm_movemask_epi8(scan) ^ 0xFFFF;
            int numchars = 15 - bit_scan_reverse(charmask);
            if (signlist[i]) {
                Vec16c minuspos = Vec16c(andnot(scan >> 8, scan)); // position of minus sign
                ascii  = select(minuspos, Vec16c('-'), ascii);     // insert minus sign
                numchars++;
            }
            int flen2 = fieldlen;
            if (numchars > flen2) {
                // number too big for field
                if (ovfl) {
                    ascii = Vec16c(ovfl);        // fill whole field with ovfl character
                }
                else {
                    flen2 = numchars;            // make field longer
                }
            }
            numwrit += flen2;
            // write field
            int c = flen2 - 16;                  // extra spaces needed
            if (c > 0) {                  
                flen2 -= c;
                // loop for multiples of 16 spaces
                for (; c >= 16; c -= 16) {
                    space16.store(string);  string += 16;
                }
                // remaining < 16 spaces
                space16.store_partial(c, string);  string += c;
            }
            if (flen2 < 16) {
                ascii = shift_bytes_down(ascii, 16 - flen2);
            }
            // insert number
            ascii.store_partial(flen2, string);  string += flen2;
            if (i < numdat-1) {
                // not last field. insert separator
                if (separator) {
                    *(string++) = separator;
                    numwrit++;
                }
                // get next number into position 0
                aa = permute4i<1,2,3,-256>(aa);  // shift down next value
            }
        }
        if (term) *string = 0;
    }
    return numwrit;
}


static int bin2ascii (Vec8i const & a, char * string, int fieldlen = 8, int numdat = 8, bool signd = true, char ovfl = '*', char separator = ',', bool term = true) {
    int numwrit;
    numwrit = bin2ascii (a.get_low(), string, fieldlen, numdat < 4 ? numdat : 4, signd, ovfl, separator, term);
    if (numdat > 4) {
        string += numwrit;
        if (separator) {
            *(string++) = separator;  numwrit++;
        }
        numwrit += bin2ascii (a.get_high(), string, fieldlen, numdat - 4, signd, ovfl, separator, term);
    }
    return numwrit;
}


// Convert binary numbers to decimal ASCII string.
// The numbers will be written to the string as decimal numbers in human-readable format.
// Each number will be right-justified with leading spaces in a field of the specified length.
static int bin2ascii (Vec8s const & a, char * string, int fieldlen = 4, int numdat = 8, bool signd = true, char ovfl = '*', char separator = ',', bool term = true) {
    // Parameters:
    // a         vector of 8 numbers to convert
    // string    buffer to receive ASCII string
    // fieldlen  string length of each converted number
    // numdat    number of data elements to output
    // signd     each number will be interpreted as signed if signd is true, unsigned if false. 
    //           Negative numbers will be indicated by a preceding '-'
    // ovfl      Output string will be filled with this character if the number is too big to fit in the field.
    //           The size of a field will be extended in case of overflow if ovfl = 0.
    // separator This character is inserted between data fields, but not after the last field.
    //           The separator character is not included in fieldlen. Separator = 0 for no separator.
    // term      The output string will have a terminating zero ('\0') if term is true.
    // Return value: The length of the written string is returned. The terminating zero is not included in the count.

    if (fieldlen <= 0 || (uint32_t)(numdat-1) > 8-1) {
        // write nothing but terminator
        if (term) *string = 0;
        return 0;
    }

    Vec8us aa;               // a or abs(a)
    Vec8s  signa;            // sign of a
    Vec8i  signe;            // sign of a, extended
    Vec8s  ovfla;            // overflow of a
    Vec8i  ovfle;            // overflow of a, extended
    Vec16c space16(' ');     // 16 spaces
    int    numwrit;          // number of bytes written, not including terminating zero

    // limits depending on fieldlength
    static const int  limits[5] = {0,9,99,999,9999};
    int flen = fieldlen < 4 ? fieldlen : 4;
    if (signd) {                                                // signed
        aa    = abs(a);                                         // abs a
        signa = a >> 15;                                        // sign
        signe = Vec8i(extend_low(signa), extend_high(signa));   // sign, extended
        ovfla = a > limits[flen] || a < -limits[flen-1];        // overflow
    }
    else {                                                      // unsigned
        aa = a;
        ovfla = aa > limits[flen];                              // overflow
        signa = 0;
    }
    if (!(horizontal_or(ovfla) && (fieldlen > 4 || ovfl == 0))) {
        // normal case
        ovfle = Vec8i(extend_low(ovfla), extend_high(ovfla));   // overflow, extended
        Vec16uc bcd        = Vec16uc(bin2bcd(aa));              // converted to BCD
        Vec16uc bcd0246    = bcd & 0x0F;                        // low  nibbles of BCD code
        Vec16uc bcd1357    = bcd >> 4;                          // high nibbles of BCD code
        // interleave nibbles and reverse byte order
        Vec16c  declo      = blend16c<17,1,16,0, 19,3,18,2,   21,5,20,4,   23,7,22,6>  (bcd0246, bcd1357);
        Vec16c  dechi      = blend16c<25,9,24,8, 27,11,26,10, 29,13,28,12, 31,15,30,14>(bcd0246, bcd1357);
        Vec32c  dec        = Vec32c(declo,dechi);               // all digits, big endian digit order
        Vec32c  ascii      = dec + 0x30;                        // add '0' to get ascii digits
        // find most significant nonzero digit, or digit 0 if all zero
        Vec32c decnz = (dec != 0) | Vec32c(0,0,0,-1,0,0,0,-1,0,0,0,-1,0,0,0,-1,0,0,0,-1,0,0,0,-1,0,0,0,-1,0,0,0,-1);
        Vec8i  scan  = Vec8i(decnz);
        scan |= scan << 8; scan |= scan << 16;
        // insert spaces to the left of most significant nonzero digit
        ascii = select(Vec32c(scan), ascii, Vec32c(' '));
        if (signd) {
            Vec32c minuspos = Vec32c(andnot(scan >> 8, scan)) & Vec32c(signe);  // position of minus sign
            ascii  = select(minuspos, Vec32c('-'), ascii);      // insert minus sign
        }
        // insert overflow indicator
        ascii = select(Vec32c(ovfle), Vec32c(ovfl), ascii);
        const int d = -256;  // means don't care in permute functions
        if (separator) {
            // write output fields with separator
            numwrit = (fieldlen+1) * numdat - 1;
            Vec32c sep(separator);
            if (fieldlen < 4) {
                switch (fieldlen) {
                case 1:
                    ascii = blend32c<3,32,7,32,11,32,15,32,19,32,23,32,27,32,31,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d>(ascii, sep);
                    break;
                case 2:
                    ascii = blend32c<2,3,32,6,7,32,10,11,32,14,15,32,18,19,32,22,23,32,26,27,32,30,31,d,d,d,d,d,d,d,d,d>(ascii, sep);
                    break;
                case 3:
                    ascii = blend32c<1,2,3,32,5,6,7,32,9,10,11,32,13,14,15,32,17,18,19,32,21,22,23,32,25,26,27,32,29,30,31,d>(ascii, sep);
                    break;
                }
                // store to string
                ascii.store_partial(numwrit, string);
                if (term) string[numwrit] = 0;
            }
            else if (fieldlen == 4) {
                Vec32c ascii1 = blend32c<0,1,2,3,32,4,5,6,7,32,8,9,10,11,32,12,13,14,15,32,16,17,18,19,32,20,21,22,23,32,d,d>(ascii, sep);
                int nn = numwrit;  if (nn > 30) nn = 30;
                ascii1.store_partial(nn, string);  string += nn;
                if (numdat > 6) {
                    Vec16c ascii2 = blend16c<8,9,10,11,16,12,13,14,15,d,d,d,d,d,d,d>(ascii.get_high(), sep.get_low());
                    nn = (numdat-6)*5-1;
                    ascii2.store_partial(nn, string);  string += nn;
                }
                if (term) *string = 0;
            }
            else {
                // fieldlen > 4
                int f;  // field counter
                int c;  // space counter
                // loop for each field
                for (f = 0; f < numdat; f++) {
                    // loop for multiples of 16 spaces
                    for (c = fieldlen - 4; c >= 16; c -= 16) {
                        space16.store(string);  string += 16;
                    }
                    // remaining < 16 spaces
                    space16.store_partial(c, string);  string += c;
                    // insert number (4 digits)
                    ascii.store_partial(4, string);  string += 4;
                    if (f < numdat-1) {
                        // not last field. insert separator
                        *(string++) = separator;
                        // get next number into position 0
                        ascii = Vec32c(permute8i<1,2,3,4,5,6,7,d>(Vec8i(ascii)));
                    }
                }
                if (term) *string = 0;
            }
        }
        else {
            // write output fields without separator
            numwrit = fieldlen * numdat;
            if (fieldlen <= 4) {
                switch (fieldlen) {
                case 1:
                    ascii = permute32c<3,7,11,15,19,23,27,31,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d>(ascii);
                    break;
                case 2:
                    ascii = permute32c<2,3,6,7,10,11,14,15,18,19,22,23,26,27,30,31,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d>(ascii);
                    break;
                case 3:
                    ascii = permute32c<1,2,3,5,6,7,9,10,11,13,14,15,17,18,19,21,22,23,25,26,27,29,30,31,d,d,d,d,d,d,d,d>(ascii);
                    break;
                }
                // store to string
                ascii.store_partial(numwrit, string);
                if (term) string[numwrit] = 0;
            }
            else {
                // fieldlen > 4
                int f;  // field counter
                int c;  // space counter
                Vec16c space16(' ');       // 16 spaces
                // loop for each field
                for (f = 0; f < numdat; f++) {
                    // loop for multiples of 16 spaces
                    for (c = fieldlen - 4; c >= 16; c -= 16) {
                        space16.store(string);  string += 16;
                    }
                    // remaining < 16 spaces
                    space16.store_partial(c, string);  string += c;
                    // insert number (4 digits)
                    ascii.store_partial(4, string);  string += 4;
                    // get next number into position 0
                    ascii = Vec32c(permute8i<1,2,3,4,5,6,7,d>(Vec8i(ascii)));
                }
                if (term) *string = 0;
            }
        }
    }
    else {
        // two special cases are handled here
        // (1) more than 4 characters needed
        // (2) one or more fields need to be extended to handle overflow
        Vec4i a0, a1;
        if (signd) {
            a0 = extend_low(a);  a1 = extend_high(a);
        }
        else {
            a0 = extend_low(Vec8us(a));  a1 = extend_high(Vec8us(a));
        }
        numwrit = bin2ascii (a0, string, fieldlen, numdat < 4 ? numdat : 4, signd, ovfl, separator, term);
        if (numdat > 4) {
            string += numwrit;
            if (separator) {
                *(string++) = separator;  numwrit++;
            }
            numwrit += bin2ascii (a1, string, fieldlen, numdat - 4, signd, ovfl, separator, term);
        }
    }
    return numwrit;
}

static int bin2ascii (Vec16s const & a, char * string, int fieldlen = 4, int numdat = 16, bool signd = true, char ovfl = '*', char separator = ',', bool term = true) {
    int numwrit;
    numwrit = bin2ascii (a.get_low(), string, fieldlen, numdat < 8 ? numdat : 8, signd, ovfl, separator, term);
    if (numdat > 8) {
        string += numwrit;
        if (separator) {
            *(string++) = separator;  numwrit++;
        }
        numwrit += bin2ascii (a.get_high(), string, fieldlen, numdat - 8, signd, ovfl, separator, term);
    }
    return numwrit;
}


// Convert binary numbers to decimal ASCII string.
// The numbers will be written to the string as decimal numbers in human-readable format.
// Each number will be right-justified with leading spaces in a field of the specified length.
static int bin2ascii (Vec16c const & a, char * string, int fieldlen = 2, int numdat = 16, bool signd = true, char ovfl = '*', char separator = ',', bool term = true) {
    // Parameters:
    // a         vector of 16 numbers to convert
    // string    buffer to receive ASCII string
    // fieldlen  string length of each converted number
    // numdat    number of data elements to output
    // signd     each number will be interpreted as signed if signd is true, unsigned if false. 
    //           Negative numbers will be indicated by a preceding '-'
    // ovfl      Output string will be filled with this character if the number is too big to fit in the field.
    //           The size of a field will be extended in case of overflow if ovfl = 0.
    // separator This character is inserted between data fields, but not after the last field.
    //           The separator character is not included in fieldlen. Separator = 0 for no separator.
    // term      The output string will have a terminating zero ('\0') if term is true.
    // Return value: The length of the written string is returned. The terminating zero is not included in the count.

    if (fieldlen <= 0 || (uint32_t)(numdat-1) > 16-1) {
        // write nothing but terminator
        if (term) *string = 0;
        return 0;
    }

    Vec16uc aa;              // a or abs(a)
    Vec16c  signa;           // sign of a
    Vec16s  signe;           // sign of a, extended
    Vec16c  ovfla;           // overflow of a
    Vec16s  ovfle;           // overflow of a, extended
    Vec16c  space16(' ');    // 16 spaces
    int     numwrit;         // number of bytes written, not including terminating zero

    // limits depending on fieldlength
    static const int  limits[3] = {0,9,99};
    int flen = fieldlen < 2 ? fieldlen : 2;
    if (signd) {             // signed
        aa    = abs(a);      // abs a
        signa = a >> 7;      // sign
        signe = Vec16s(extend_low(signa), extend_high(signa));  // sign, extended
        ovfla = a > limits[flen] || a < -limits[flen-1];        // overflow
    }
    else {                   // unsigned
        aa = a;
        ovfla = aa > limits[flen];                               // overflow
        signa = 0;
    }
    if (!(horizontal_or(ovfla) && (fieldlen > 2 || ovfl == 0))) {
        // normal case
        ovfle = Vec16s(extend_low(ovfla), extend_high(ovfla));  // overflow, extended
        Vec16uc bcd        = bin2bcd(aa);                       // converted to BCD
        Vec16uc bcd0246    = bcd & 0x0F;                        // low  nibbles of BCD code
        Vec16uc bcd1357    = bcd >> 4;                          // high nibbles of BCD code
        // interleave nibbles and reverse byte order
        Vec16c  declo      = blend16c<16,0, 17,1, 18,2,  19,3,  20,4,  21,5,  22,6,  23,7 > (bcd0246, bcd1357);
        Vec16c  dechi      = blend16c<24,8, 25,9, 26,10, 27,11, 28,12, 29,13, 30,14, 31,15> (bcd0246, bcd1357);
        Vec32c  dec        = Vec32c(declo,dechi);               // all digits, big endian digit order
        Vec32c  ascii      = dec + 0x30;                        // add '0' to get ascii digits
        // find most significant nonzero digit, or digit 0 if all zero
        Vec32c  decnz = (dec != 0) | Vec32c(0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1);
        Vec16s  scan  = Vec16s(decnz);
        scan |= scan << 8;
        // insert spaces to the left of most significant nonzero digit
        ascii = select(Vec32c(scan), ascii, Vec32c(' '));
        if (signd) {
            Vec32c minuspos = Vec32c(Vec16us(signe) >> 8);      // position of minus sign
            ascii  = select(minuspos, Vec32c('-'), ascii);      // insert minus sign
        }
        // insert overflow indicator
        ascii = select(Vec32c(ovfle), Vec32c(ovfl), ascii);
        const int d = -256;  // means don't care in permute functions
        if (separator) {
            // write output fields with separator
            numwrit = (fieldlen+1) * numdat - 1;
            Vec32c sep(separator);
            if (fieldlen == 1) {
                ascii = blend32c<1,32,3,32,5,32,7,32,9,32,11,32,13,32,15,32,17,32,19,32,21,32,23,32,25,32,27,32,29,32,31,d>(ascii, sep);
                // store to string
                ascii.store_partial(numwrit, string);
                if (term) string[numwrit] = 0;
            }
            else if (fieldlen == 2) {
                Vec32c ascii1 = blend32c<0,1,32,2,3,32,4,5,32,6,7,32,8,9,32,10,11,32,12,13,32,14,15,32,16,17,32,18,19,32,20,21>(ascii, sep);
                int nn = numwrit;  if (nn > 32) nn = 32;
                ascii1.store_partial(nn, string);  string += nn;
                if (numdat > 11) {
                    Vec16c ascii2 = blend16c<16,6,7,16,8,9,16,10,11,16,12,13,16,14,15,d>(ascii.get_high(), sep.get_low());
                    nn = (numdat-11)*3;
                    ascii2.store_partial(nn, string);  string += nn;
                }
                if (term) *string = 0;
            }
            else {
                // fieldlen > 2
                int f;  // field counter
                int c;  // space counter
                // loop for each field
                for (f = 0; f < numdat; f++) {
                    // loop for multiples of 16 spaces
                    for (c = fieldlen - 2; c >= 16; c -= 16) {
                        space16.store(string);  string += 16;
                    }
                    // remaining < 16 spaces
                    space16.store_partial(c, string);  string += c;
                    // insert number (2 digits)
                    ascii.store_partial(2, string);  string += 2;
                    if (f < numdat-1) {
                        // not last field. insert separator
                        *(string++) = separator;
                        // get next number into position 0
                        ascii = Vec32c(permute16s<1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,d>(Vec16s(ascii)));
                    }
                }
                if (term) *string = 0;
            }
        }
        else {
            // write output fields without separator
            numwrit = fieldlen * numdat;
            if (fieldlen == 1) {
                ascii = permute32c<1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d,d>(ascii);
                // store to string
                ascii.store_partial(numwrit, string);
                if (term) string[numwrit] = 0;
            }
            else if (fieldlen == 2) {
                ascii.store_partial(numwrit, string);
                if (term) string[numwrit] = 0;
            }
            else {
                // fieldlen > 2
                int f;                 // field counter
                int c;                 // space counter
                Vec16c space16(' ');   // 16 spaces
                // loop for each field
                for (f = 0; f < numdat; f++) {
                    // loop for multiples of 16 spaces
                    for (c = fieldlen - 2; c >= 16; c -= 16) {
                        space16.store(string);  string += 16;
                    }
                    // remaining < 16 spaces
                    space16.store_partial(c, string);  string += c;
                    // insert number (2 digits)
                    ascii.store_partial(2, string);  string += 2;
                    // get next number into position 0
                    ascii = Vec32c(permute16s<1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,d>(Vec16s(ascii)));
                }
                if (term) *string = 0;
            }
        }
    }
    else {
        // two special cases are handled here
        // (1) more than 2 characters needed
        // (2) one or more fields need to be extended to handle overflow
        Vec8s a0, a1;
        if (signd) {
            a0 = extend_low(a);  a1 = extend_high(a);
        }
        else {
            a0 = extend_low(Vec16uc(a));  a1 = extend_high(Vec16uc(a));
        }
        numwrit = bin2ascii (a0, string, fieldlen, numdat < 8 ? numdat : 8, signd, ovfl, separator, term);
        if (numdat > 8) {
            string += numwrit;
            if (separator) {
                *(string++) = separator;  numwrit++;
            }
            numwrit += bin2ascii (a1, string, fieldlen, numdat - 8, signd, ovfl, separator, term);
        }
    }
    return numwrit;
}

static int bin2ascii (Vec32c const & a, char * string, int fieldlen = 2, int numdat = 32, bool signd = true, char ovfl = '*', char separator = ',', bool term = true) {
    int numwrit;
    numwrit = bin2ascii (a.get_low(), string, fieldlen, numdat < 16 ? numdat : 16, signd, ovfl, separator, term);
    if (numdat > 16) {
        string += numwrit;
        if (separator) {
            *(string++) = separator;  numwrit++;
        }
        numwrit += bin2ascii (a.get_high(), string, fieldlen, numdat - 16, signd, ovfl, separator, term);
    }
    return numwrit;
}



/*****************************************************************************
*
*               Conversion from binary to hexadecimal ASCII string
*
*****************************************************************************/

// Convert binary numbers to hexadecimal ASCII string.
// The numbers will be written to the string as unsigned hexadecimal numbers with 16 digits for each data element
// with big endian digit order
static int bin2hex_ascii (Vec2q const & a, char * string, int numdat = 2, char separator = ',', bool term = true) {
    // Parameters:
    // a         vector of 2 numbers to convert
    // string    buffer to receive ASCII string
    // numdat    number of data elements to output
    // separator This character is inserted between data fields, but not after the last field.
    //           The separator character is not included in fieldlen. Separator = 0 for no separator.
    // term      The output string will have a terminating zero ('\0') if term is true.
    // Return value: The length of the written string is returned. The terminating zero is not included in the count.

    if ((uint32_t)(numdat-1) > 2-1) {
        // write nothing but terminator
        if (term) *string = 0;
        return 0;
    }
    Vec16uc nibb0  = Vec16uc(a) & 0x0F;                         // low  nibbles
    Vec16uc nibb1  = Vec16uc(a) >> 4;                           // high nibbles
    // interleave nibbles and reverse byte order
    Vec16c  declo  = blend16c<23, 7,22, 6,21, 5,20, 4,19, 3,18, 2,17, 1,16, 0>(nibb0, nibb1);
    Vec16c  dechi  = blend16c<31,15,30,14,29,13,28,12,27,11,26,10,25, 9,24, 8>(nibb0, nibb1);
    Vec32c  hex    = Vec32c(declo,dechi);                       // all digits, big endian digit order
    Vec32c  ascii  = hex + 0x30;                                // add '0' to get ascii digits 0 - 9
            ascii += (ascii > '9') & 7;                         // fix A - F
    // store first number
    ascii.get_low().store(string);  string += 16;
    if (numdat > 1) {
        // store separator and second number
        if (separator) *(string++) = separator;
        ascii.get_high().store(string);  string += 16;
    }
    if (term) *string = 0;
    return numdat*16 + (numdat-1)*(separator != 0);
}

static int bin2hex_ascii (Vec4q const & a, char * string, int numdat = 4, char separator = ',', bool term = true) {
    int numwrit;
    numwrit = bin2hex_ascii(a.get_low(), string, numdat < 2 ? numdat : 2, separator, term);
    if (numdat > 2) {
        string += numwrit;
        if (separator) {
            *(string++) = separator;  numwrit++;
        }
        numwrit += bin2hex_ascii(a.get_high(), string, numdat - 2, separator, term);
    }
    return numwrit;
}


// Convert binary numbers to hexadecimal ASCII string.
// The numbers will be written to the string as unsigned hexadecimal numbers with 8 digits for each data element
// with big endian digit order
static int bin2hex_ascii (Vec4i const & a, char * string, int numdat = 4, char separator = ',', bool term = true) {
    // parameters:
    // a         vector of 4 numbers to convert
    // string    buffer to receive ASCII string
    // numdat    number of data elements to output
    // separator This character is inserted between data fields, but not after the last field.
    //           The separator character is not included in fieldlen. Separator = 0 for no separator.
    // term      The output string will have a terminating zero ('\0') if term is true.
    // return value: The length of the written string is returned. The terminating zero is not included in the count.

    if ((uint32_t)(numdat-1) > 4-1) {
        // write nothing but terminator
        if (term) *string = 0;
        return 0;
    }
    int numwrit;                                                // number of characters written
    Vec16uc nibb0  = Vec16uc(a) & 0x0F;                         // low  nibbles
    Vec16uc nibb1  = Vec16uc(a) >> 4;                           // high nibbles
    // interleave nibbles and reverse byte order
    Vec16c  declo  = blend16c<19, 3,18, 2,17, 1,16, 0, 23, 7,22, 6,21, 5,20, 4>(nibb0, nibb1);
    Vec16c  dechi  = blend16c<27,11,26,10,25, 9,24, 8, 31,15,30,14,29,13,28,12>(nibb0, nibb1);
    Vec32c  hex    = Vec32c(declo,dechi);                       // all digits, big endian digit order
    Vec32c  ascii  = hex + 0x30;                                // add '0' to get ascii digits 0 - 9
            ascii += (ascii > '9') & 7;                         // fix A - F
    if (separator) {
        const int d = -256;                                     // don't care
        numwrit = 9 * numdat - 1;
        Vec32c sep(separator);
        Vec32c ascii1 = blend32c<0,1,2,3,4,5,6,7,32,8,9,10,11,12,13,14,15,32,16,17,18,19,20,21,22,23,32,d,d,d,d,d>(ascii, sep);
        int nn = numwrit < 27 ? numwrit : 27;
        ascii1.store_partial(nn, string);
        if (numdat > 3) {
            Vec16c ascii2 = Vec16c(permute2q<1,d>(Vec2q(ascii.get_high())));
            ascii2.store_partial(8, string + nn);
        }
    }
    else {
        numwrit = 8 * numdat;
        ascii.store_partial(numwrit, string);
    }
    if (term) {
        string[numwrit] = 0;
    }        
    return numwrit;
}

static int bin2hex_ascii (Vec8i const & a, char * string, int numdat = 8, char separator = ',', bool term = true) {
    int numwrit;
    numwrit = bin2hex_ascii(a.get_low(), string, numdat < 4 ? numdat : 4, separator, term);
    if (numdat > 4) {
        string += numwrit;
        if (separator) {
            *(string++) = separator;  numwrit++;
        }
        numwrit += bin2hex_ascii(a.get_high(), string, numdat - 4, separator, term);
    }
    return numwrit;
}


// Convert binary numbers to hexadecimal ASCII string.
// The numbers will be written to the string as unsigned hexadecimal numbers with 4 digits for each data element
// with big endian digit order
static int bin2hex_ascii (Vec8s const & a, char * string, int numdat = 8, char separator = ',', bool term = true) {
    // Parameters:
    // a         vector of 8 numbers to convert
    // string    buffer to receive ASCII string
    // numdat    number of data elements to output
    // separator This character is inserted between data fields, but not after the last field.
    //           The separator character is not included in fieldlen. Separator = 0 for no separator.
    // term      The output string will have a terminating zero ('\0') if term is true.
    // Return value: The length of the written string is returned. The terminating zero is not included in the count.

    if ((uint32_t)(numdat-1) > 8-1) {
        // write nothing but terminator
        if (term) *string = 0;
        return 0;
    }
    int numwrit;                                                // number of characters written
    Vec16uc nibb0  = Vec16uc(a) & 0x0F;                         // low  nibbles
    Vec16uc nibb1  = Vec16uc(a) >> 4;                           // high nibbles
    // interleave nibbles and reverse byte order
    Vec16c  declo  = blend16c<17,1,16,0,  19,3,18,2,   21,5,20,4,   23,7,22,6  >(nibb0, nibb1);
    Vec16c  dechi  = blend16c<25,9,24,8,  27,11,26,10, 29,13,28,12, 31,15,30,14>(nibb0, nibb1);
    Vec32c  hex    = Vec32c(declo,dechi);                       // all digits, big endian digit order
    Vec32c  ascii  = hex + 0x30;                                // add '0' to get ascii digits 0 - 9
            ascii += (ascii > '9') & 7;                         // fix A - F
    if (separator) {
        const int d = -256;                                     // don't care
        numwrit = 5 * numdat - 1;
        Vec32c sep(separator);
        Vec32c ascii1 = blend32c<0,1,2,3,32,4,5,6,7,32,8,9,10,11,32,12,13,14,15,32,16,17,18,19,32,20,21,22,23,32,d,d>(ascii, sep);
        int nn = numwrit < 30 ? numwrit : 30;
        ascii1.store_partial(nn, string);
        if (numdat > 6) {
            Vec16c ascii2 = blend16c<8,9,10,11,16,12,13,14,15,d,d,d,d,d,d,d>(ascii.get_high(),sep.get_low());
            ascii2.store_partial((numdat-6)*5-1, string + nn);
        }
    }
    else {
        numwrit = 4 * numdat;
        ascii.store_partial(numwrit, string);
    }
    if (term) {
        string[numwrit] = 0;
    }        
    return numwrit;
}

static int bin2hex_ascii (Vec16s const & a, char * string, int numdat = 16, char separator = ',', bool term = true) {
    int numwrit;
    numwrit = bin2hex_ascii(a.get_low(), string, numdat < 8 ? numdat : 8, separator, term);
    if (numdat > 8) {
        string += numwrit;
        if (separator) {
            *(string++) = separator;  numwrit++;
        }
        numwrit += bin2hex_ascii(a.get_high(), string, numdat - 8, separator, term);
    }
    return numwrit;
}


// Convert binary numbers to hexadecimal ASCII string.
// The numbers will be written to the string as unsigned hexadecimal numbers with 2 digits for each data element
// with big endian digit order
static int bin2hex_ascii (Vec16c const & a, char * string, int numdat = 16, char separator = ',', bool term = true) {
    // Parameters:
    // a         vector of 16 numbers to convert
    // string    buffer to receive ASCII string
    // numdat    number of data elements to output
    // separator This character is inserted between data fields, but not after the last field.
    //           The separator character is not included in fieldlen. Separator = 0 for no separator.
    // term      The output string will have a terminating zero ('\0') if term is true.
    // Return value: The length of the written string is returned. The terminating zero is not included in the count.

    if ((uint32_t)(numdat-1) > 16-1) {
        // write nothing but terminator
        if (term) *string = 0;
        return 0;
    }
    int numwrit;                                                // number of characters written
    Vec16uc nibb0  = Vec16uc(a) & 0x0F;                         // low  nibbles
    Vec16uc nibb1  = Vec16uc(a) >> 4;                           // high nibbles
    // interleave nibbles and reverse byte order
    Vec16c  declo  = blend16c<16,0, 17,1, 18,2,  19,3,  20,4,  21,5,  22,6,  23,7 >(nibb0, nibb1);
    Vec16c  dechi  = blend16c<24,8, 25,9, 26,10, 27,11, 28,12, 29,13, 30,14, 31,15>(nibb0, nibb1);
    Vec32c  hex    = Vec32c(declo,dechi);                       // all digits, big endian digit order
    Vec32c  ascii  = hex + 0x30;                                // add '0' to get ascii digits 0 - 9
            ascii += (ascii > '9') & 7;                         // fix A - F
    if (separator) {
        const int d = -256;                                     // don't care
        numwrit = 3 * numdat - 1;
        Vec32c sep(separator);
        Vec32c ascii1 = blend32c<0,1,32,2,3,32,4,5,32,6,7,32,8,9,32,10,11,32,12,13,32,14,15,32,16,17,32,18,19,32,20,21>(ascii, sep);
        int nn = numwrit < 32 ? numwrit : 32;
        ascii1.store_partial(nn, string);
        if (numdat > 11) {
            Vec16c ascii2 = blend16c<16,6,7,16,8,9,16,10,11,16,12,13,16,14,15,d>(ascii.get_high(),sep.get_low());
            ascii2.store_partial((numdat-11)*3, string + nn);
        }
    }
    else {
        numwrit = 2 * numdat;
        ascii.store_partial(numwrit, string);
    }
    if (term) {
        string[numwrit] = 0;
    }        
    return numwrit;
}

static int bin2hex_ascii (Vec32c const & a, char * string, int numdat = 32, char separator = ',', bool term = true) {
    int numwrit;
    numwrit = bin2hex_ascii(a.get_low(), string, numdat < 16 ? numdat : 16, separator, term);
    if (numdat > 16) {
        string += numwrit;
        if (separator) {
            *(string++) = separator;  numwrit++;
        }
        numwrit += bin2hex_ascii(a.get_high(), string, numdat - 16, separator, term);
    }
    return numwrit;
}


/*****************************************************************************
*
*               Conversion from decimal ASCII string to binary numbers
*
*****************************************************************************/

// Converts numbers in a decimal ASCII string to a vector of signed binary integers.
// Each number must be right-justified in a field of exactly 8 characters, 
// with no separator or terminator in the string.
// The string can contain no other characters than '0'-'9', spaces and a minus sign.
// Each number must be in the range that can be contained in an 8-character field, 
// i.e. -9999999 to 99999999.
// A syntax error in a field will give the value 0x80000000. There is no performance 
// penalty for syntax errors. This can be useful if only part of the string contains
// valid numbers.
// Parameters:
// string:  A 32-byte vector containing an ASCII string with four decimal numbers
//          right-justified in each of four 8-character fields.
// Return value: A vector of four signed integers

static Vec4i ascii2bin(Vec32c const & string) {
    // Find minus signs
    Vec32c minussigns = string == '-';
    // Convert '-' to ' '
    Vec32uc string1 = Vec32uc(string - (minussigns & ('-' - ' ')));
    // Find digits '0' - '9'
    string1 -= '0';
    Vec32c digitpos = string1 <= 9;
    // Check that we have nothing to the right of digits
    Vec4q  digitpos2 = Vec4q(digitpos);
    digitpos2 |= digitpos2 << 8;  digitpos2 |= digitpos2 << 16;  digitpos2 |= digitpos2 << 32;
    Vec4q  syntaxerr = digitpos2 != Vec4q(digitpos);
    // Check that we have at least one digit
    syntaxerr |= Vec4q(digitpos) == 0;
    // Check that we have only spaces to the left of digits
    syntaxerr |= Vec4q((string1 == ' ' - '0') | digitpos) != -1L;
#if 1
    // Check that we have no more than one minus sign
    // (this error is so rare that you may remove this check)
    Vec32uc minuscheck = Vec32uc(minussigns) >> 7;
    syntaxerr |= (Vec4q(minuscheck) & (Vec4q(minuscheck) - 1)) != 0;
#endif
    // Replace spaces by zeroes
    string1 &= digitpos;
    // Convert to binary by multiplying. Note that byte order is big endian within each 8-byte field
    // Multiply even-numbered digits by 10
    Vec32uc string2 = (string1 << 1) + (string1 << 3);  // there is no efficient 8-bit multiply. shift and add instead
    // Add to odd-numbered digits to even-numbered digits * 10
    Vec16us string3 = (Vec16us(string1) >> 8) + Vec16us(Vec16us(string2) & Vec16us(0x00FF));
    // Now we have 4x4 blocks of 00-99. Multiply every second number by 100
    Vec16us string4 = string3 * Vec16us(100,1,100,1,100,1,100,1,100,1,100,1,100,1,100,1);
    // Add
    string4 += Vec16us(Vec8ui(string4) >> 16);
    // Sort out blocks, and zero-extend
    Vec4i a0 = Vec4i(blend8s<2,-1,6,-1,10,-1,14,-1>(string4.get_low(), string4.get_high()));
    Vec4i a1 = Vec4i(blend8s<0,-1,4,-1, 8,-1,12,-1>(string4.get_low(), string4.get_high()));
    // Multiply
    Vec4i b  = a0 + a1 * 10000;
    // Get signs
    Vec4q signse = Vec4q(minussigns) != 0;
    // Compress signs
    Vec4i signs = compress(signse.get_low(), signse.get_high());
    // Apply signs
    Vec4i c  = (b ^ signs) - signs;
    // Compress syntaxerr
    Vec4i err = compress(syntaxerr.get_low(), syntaxerr.get_high());
    // Insert 0x80000000 for syntax error
    Vec4i d  = select(err, Vec4i(0x80000000), c);
    // Return
    return d;
}
