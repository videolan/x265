The ASM source here is directly pulled from the x264 project with two
changes:

1 - FENC_STRIDE must be increased to 64 in x86util.asm because of HEVC's
    larger CU sizes
2 - Because of #1, we must rebrand the functions with x265_ prefixes in
    x86inc.asm (private_prefix) and pixel-a.asm (mangle(x265_pixel_ssd))
3 - We have modified the MMX SSD primitives to use EMMS before returning
4 - We have added some new SATD block sizes for SSE3

Current assembly is based on x264 revision:
   configure: Support cygwin64
   Diogo Franco (Kovensky) <diogomfranco@gmail.com>
   2013-07-23 22:17:44 -0300
