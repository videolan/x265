*********************
Command line Options
*********************

Standalone Executable Options
=============================

.. option:: --help/-h

   Display help text

.. option:: --version/-V

   Display version details

.. option:: --cpuid
   
   Limit SIMD capability bitmap
   
   **Values:**  0:auto **(Default)**; 1:none
   
.. option:: --threads
   
   Number of threads for thread pool; 0: detect CPU core count **(Default)**
   
.. option:: --preset/-p
   
   Sets parameters to preselected values, trading off compression efficiency against 
   encoding speed. These parameters are applied before all other input parameters are 
   applied, and so you can override any parameters that these values control.   
   
   **Values:**  ultrafast, superfast, veryfast, faster, fast, medium **(Default)**, slow, slower, veryslow, placebo

.. option:: --tune/-t

   Tune the settings for a particular type of source or situation. The changes will
   be applied after --preset but before all other parameters.
   
   **Values:** psnr, ssim **(Default)**, zero-latency.

.. option:: --frame-threads/-F
    
   Number of concurrently encoded frames
  
   **Range of values:** >=0.  **Default** = auto-determined from a formula based on the number of CPU cores

.. option::--log
    
   Logging level

   **Values:**  0:ERROR; 1:WARNING; 2:INFO **(Default)**; 3:DEBUG; 4:FULL -1:NONE

.. option:: --log 3

   produces a log file that records results per frame

.. option:: --output/-o

   Bitstream output file name

.. option:: --no-progress

   Disable per-frame encoding progress reporting

.. option:: --csv <filename>

   Writes encoding results to a comma separated value log file
   Creates the file if it doesnt already exist, else adds one line per run

.. option:: --y4m
    
   Parse input stream as YUV4MPEG2 regardless of file extension

------------------------------

Input Options
=============

.. option:: --input

    Raw YUV or Y4M input file name

.. option:: --input-depth

    Bit-depth of input file (YUV only).

    **Values:** any value between 8 and 16. Default is internal depth.

.. option:: --input-res

    Source picture size [w x h], auto-detected if Y4M

.. option:: --input-csp

    Source color space parameter, auto detected if Y4M;

    **Values:** 1:"i420" **(Default)**, or 3:"i444"

.. option:: --fps

    Source frame rate; auto-detected if Y4M;

    **Range of values:** positive int or float, or num/denom

.. option:: --seek
    
    Number of frames to skip at start of input file

    **Range of values:** 0 to the number of frames in the video
    **Default**: 0

.. option:: --frames/-f

    Number of frames to be encoded; 0 implies all **(Default)**

    **Range of values:** 0 to the number of frames in the video

------------------------------

Reconstructed video options (debugging)
=======================================

.. option:: --recon/-r

    Re-constructed image YUV or Y4M output file name

.. option:: --recon-depth

    Bit-depth of output file 

    **Default:** same as input bit depth

Quad-Tree analysis
==================

.. option:: --no-wpp

    Disable Wavefront Parallel Processing

.. option:: --wpp

    Enable Wavefront Parallel Processing **(Default)**

.. option:: --ctu/-s

    Maximum CU size (width and height)
   
    **Values:** 16, 32, 64 **(Default)**

.. option:: --tu-intra-depth

    Max TU recursive depth for intra CUs
   
    **Values:** 1 **(Default)**, 2, 3, 4

.. option:: --tu-inter-depth

    Max TU recursive depth for inter CUs 
   
    **Values:** 1 **(Default)**, 2, 3, 4

------------------------------   

Temporal / motion search options
================================

.. option:: --me

    Motion search method 0: dia; 1: hex **(Default)**; 2: umh; 3: star; 4: full

.. option:: --subme/-m

    Amount of subpel refinement to perform

    **Range of values:** an integer from 0 to 7 (0: least..7: most)
    **Default: 2**

.. option:: --merange

    Motion search range

    **Range of values:** an integer from 0 to 32768
    **Default: 57**

.. option:: --no-rect

    Disable rectangular motion partitions Nx2N and 2NxN

.. option:: --rect

    Enable rectangular motion partitions Nx2N and 2NxN **(Default)**

.. option:: --no-amp

    Disable asymmetric motion partitions

.. option:: --amp

    Enable asymmetric motion partitions, requires rect **(Default)**

.. option:: --max-merge

   Maximum number of merge candidates
    
   **Range of values:** 1 to 5  **Default: 2**

.. option:: --early-skip

    Enable early SKIP detection

.. option:: --no-early-skip

    Disable early SKIP detection **(Default)**

.. option:: --fast-cbf

    Enable Cbf fast mode

.. option:: --no-fast-cbf

    Disable Cbf fast mode **(Default)**
 
------------------------------

Spatial/intra options
=====================

.. option:: --rdpenalty

    Penalty for 32x32 intra TU in non-I slices. 

    **Range of values:** 0:disabled **(Default)**; 1:RD-penalty; 2:maximum

.. option:: --no-tskip

    Disable intra transform skipping **(Default)**

.. option:: --tskip

    Enable intra transform skipping 

.. option:: --no-tskip-fast

    Disable fast intra transform skipping **(Default)**

.. option:: --tskip-fast

    Enable fast intra transform skipping

.. option:: --no-strong-intra-smoothing

    Disable strong intra smoothing for 32x32 blocks

.. option:: --strong-intra-smoothing

   Enable strong intra smoothing for 32x32 blocks **(Default)**
   
.. option:: --constrained-intra

    Constrained intra prediction (use only intra coded reference pixels)

.. option:: --no-constrained-intra

    Disable constrained intra prediction (use only intra coded reference pixels **(Default)**

------------------------------

Slice decision options
======================

.. option:: --open-gop

    Enable open GOP, allow I-slices to be non-IDR

.. option:: --no-open-gop

    Disable open GOP. All I-slices are IDR.

.. option:: --keyint/-I

    Max intra period in frames. A special case of infinite-gop (single keyframe at the beginning of the stream)
    can be triggered with argument -1.

    **Range of values:** >= -1 (-1: infinite-gop, 0: auto; 1: intra only) **Default: 250**

.. option:: --min-keyint/-i

    Minimum GOP size. Scenecuts closer together than this are coded as I, not IDR. 

    **Range of values:** >=0 (0: auto)

.. option:: --scenecut

    How aggressively I-frames need to be inserted. The lower the threshold value, the more aggressive the I-frame placement. 

    **Range of values:** >=0  **Default: 40**

.. option:: --no-scenecut

    Disable adaptive I-frame placement

.. option:: --rc-lookahead

    Number of frames for frame-type lookahead (determines encoder latency) 

    **Range of values:** an integer less than or equal to 250 and greater than maximum consecutive bframe count (--bframes)
    **Default: 20**

.. option:: --b-adapt

    Adaptive B frame scheduling

    **Values:** 0:none; 1:fast; 2:full(trellis) **(Default)**

.. option:: --bframes/-b

    Maximum number of consecutive b-frames 

    **Range of values:** 0 to 16  **Default: 4**

.. option:: --bframe-bias

    Bias towards B frame decisions

    **Range of values:** usually >=0 (increase the value for referring more B Frames e.g. 40-50) **Default: 0**

.. option:: --b-pyramid

    Use B-frames as references 0: Disabled, 1: Enabled **(Default)**

.. option:: --ref
    
    Max number of L0 references to be allowed

    **Range of values:** 1 to 16  **Default: 3**

.. option:: --weightp/-w

    Enable weighted prediction in P slices**(Default)**

.. option:: --no-weightp

    Disable weighted prediction in P slices 

------------------------------

Quality, rate control and rate distortion options
=================================================
.. option:: --bitrate

   Enables ABR rate control.  Specify the target bitrate in kbps.  

   **Range of values:** An integer greater than 0

.. option:: --crf

   Quality-controlled VBR

   **Range of values:** an integer from 0 to 51 **Default: 28**

.. option:: --vbv-bufsize

   Enables VBV in ABR mode. Sets the size of the VBV buffer (kbits)  **Default: 0**

.. option:: --vbv-maxrate

   Maximum local bitrate (kbits/sec). Will be used only if vbv-bufsize is also non-zero. Both vbv-bufsize and 
   vbv-maxrate are required to enable VBV in CRF mode. **Default: 0**

.. option:: --vbv-init

   Initial VBV buffer occupancy. 

   **Range of values:** 0-1 **Default: 0.9**

.. option:: --qp/-q

   Base Quantization Parameter for Constant QP mode. Using this option causes x265 to use Constant QP rate control **(Default)**

   **Range of values:** an integer from 0 to 51  **Default: 32**

.. option:: --aq-mode

   Mode for Adaptive Quantization

   **Range of values:** 0: no Aq; 1: aqVariance 2: aqAutoVariance  **Default: 1**

.. option:: --aq-strength

   Reduces blocking and blurring in flat and textured areas

   **Range of values:** 0.0  to 3.0 (double) **Default: 1.0**

.. option:: --cbqpoffs

   Chroma Cb QP Offset

   **Range of values:**  -12 to 12  **Default: 0**

.. option:: --crqpoffs

   Chroma Cr QP Offset 

   **Range of values:**  -12 to 12   **Default: 0**

.. option:: --rd

   Level of RD in mode decision

   **Range of values:** 0: Least - 6: Full RDO Analysis  **Default: 3**

.. option:: --signhide

   Hide sign bit of one coeff per TU (rdo) **(Default)**

.. option:: --no-signhide

   Disable hide sign bit of one coeff per TU (rdo)

------------------------------
 
Loop filter
===========

.. option:: --no-lft

   Disable Loop Filter

.. option:: --lft

   Enable Loop Filter **(Default)**

------------------------------

Sample Adaptive Offset loop filter
==================================

.. option:: --no-sao

   Disable Sample Adaptive Offset

.. option:: --sao

   Enable Sample Adaptive Offset **(Default)**

.. option:: --sao-lcu-bounds

   0: right/bottom boundary areas skipped **(Default)**; 1: non-deblocked pixels are used

.. option:: --sao-lcu-opt

   0: SAO picture-based optimization (requires -F1); 1: SAO LCU-based optimization **(Default)**

------------------------------

Quality reporting metrics
=========================

.. option:: --ssim

   Calculate and report Structural Similarity values

.. option:: --no-ssim

   Disable SSIM calculation and reporting **(Default)**

.. option:: --psnr

   Calculate and report Peak Signal to Noise Ratio

.. option:: --no-psnr

   Disable PSNR calculation and reporting **(Default)**

------------------------------

SEI options
===========

.. option:: --hash

   Decoded picture hash 0: disabled **(Default)**, 1: MD5, 2: CRC, 3: Checksum

------------------------------

VUI options
===========

.. option:: --vui

   Enable video usability Information with all fields in the SPS

   **Range of values:** 0: disabled **(Default)**; 1: Enabled

.. option:: --sar

   Sample Aspect Ratio <int:int|int>, the ratio of width to height of an individual pixel.   

   **values:** 0- undef **(Default)**, 1- 1:1(square), 2- 12:11, 3- 10:11, 4- 16:11, 5- 40:33, 6- 24:11, 7- 20:11, 
   8- 32:11, 9- 80:33, 10- 18:11, 11- 15:11, 12- 64:33, 13- 160:99, 14- 4:3, 15- 3:2, 16- 2:1 or
   custom ratio of <int:int> 

.. option:: --overscan

   Region of image that does not contain information is added to achieve certain resolution or aspect ratio

   **values:** undef **(Default)**, show, crop.

.. option:: --videoformat

   Specify video format, Explains what type analog video was before digitizing/encoding

   **values:** 0: undef, 1: component, 2: pal, 3: ntsc, 4: secam, 5: mac **(Default)**

.. option:: --range

   Specify output range of black level and range of luma and chroma signals

   **values:** full, limited **(Default)**

.. option:: --colorprim

   Set what color primitives for converting to RGB

   **values:** bt709, bt470m, bt470bg, smpte170m smpte240m, film, bt2020, undef **(Default)**

.. option:: --transfer

   Specify transfer characteristics

   **values:** bt709bt709, bt470m, bt470bg, smpte170m, smpte240m, linear, log100, log316, iec61966-2-4, bt1361e, iec61966-2-1,
   bt2020-10, bt2020-12, undef **(Default)**

.. option:: --colormatrix

   Specify color matrix setting i.e set the matrix coefficients used in deriving the luma and chroma

   **values:** bt709, fcc, bt470bg, smpte170m, smpte240m, GBR, YCgCo, bt2020nc, bt2020c, undef **(Default)**

.. option:: --chromalocs

   Specify chroma sample location

   **Range of values:** 0 **(Default)**  5 

.. option:: --no-fieldseq

   Disable pictures are fields and an SEI timing message will be added to every access unit

.. option:: --fieldseq

   Enable pictures are fields and an SEI timing message will be added to every access unit  [NOT IMPLEMENTED]

.. option:: --no-framefieldinfo

   A pic-struct will not be added to the SEI timing message **(Default)**

.. option:: --framefieldinfo

   A pic-struct will be added to the SEI timing message

.. option:: --crop-rect

   Add bitstream-level cropping rectangle.

   **values:** left, top, right, bottom

.. option:: --timinginfo

   Add timing information to the VUI

   **values:** 0 **(Default)** or 1

.. option:: --nal-hrd

   Add signal HRD information [NOT IMPLEMENTED]

   **values:** 0 **(Default)** or 1

.. option:: --bitstreamrestriction

   specifies whether that the bitstream restriction parameters for the CVS are present

   **values:** 0 **(Default)** or 1 [NOT IMPLEMENTED]

.. option:: --subpichrd

   Add sub picture HRD parameters to the HRD

   **values:** 0 **(Default)** or 1 [NOT IMPLEMENTED]
