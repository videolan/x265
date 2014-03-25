*********************
Command line Options
*********************

Note that unless an option is listed as **CLI ONLY** the option is also
supported by x265_param_parse(). The CLI uses getopt to parse the
command line options so the short or long versions may be used and the
long options may be truncated to the shortest unambiguous abbreviation.
Users of the API must pass x265_param_parse() the full option name.

Preset and tune have special implications. The API user must call
x265_param_default_preset() with the preset and tune parameters they
wish to use, prior to calling x265_param_parse() to set any additional
fields. The CLI does this for the user implicitly, so all CLI options
are applied after the user's preset and tune choices, regarless of the
order of the options on the command line.

Standalone Executable Options
=============================

.. option:: --help, -h

	Display help text

	**CLI ONLY**

.. option:: --version, -V

	Display version details

	**CLI ONLY**

.. option:: --asm <integer:false:string>, --no-asm

	x265 will use all detected CPU SIMD architectures by default. You can
	disable all assembly by using :option:`--no-asm` or you can specify
	a comma separated list of SIMD architectures to use, matching these
	strings: MMX2, SSE, SSE2, SSE3, SSSE3, SSE4, SSE4.1, SSE4.2, AVX, XOP, FMA4, AVX2, FMA3

	Some higher architectures imply lower ones being present, this is
	handled implicitly.

	One may also directly supply the CPU capability bitmap as an integer.

.. option:: --threads <integer>

	Number of threads for thread pool; 0: detect CPU core count **(Default)**

.. option:: --preset, -p <integer|string>

	Sets parameters to preselected values, trading off compression efficiency against 
	encoding speed. These parameters are applied before all other input parameters are 
	applied, and so you can override any parameters that these values control. Default medium

	**Values:** ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow, placebo

.. option:: --tune, -t <string>

	Tune the settings for a particular type of source or situation. The changes will
	be applied after --preset but before all other parameters. Default none

	**Values:** psnr, ssim, zero-latency, fast-decode.

.. option:: --frame-threads, -F <integer>

	Number of concurrently encoded frames. Using a single frame thred
	gives a slight improvement in compression, since the entire reference
	frames are always available for motion compensation, but it has
	severe performance implications. Default is an autodetected count
	based on the number of CPU cores and whether WPP is enabled or not.

.. option:: --log-level <int|string>

	Logging level. Debug level enables per-frame QP, metric, and bitrate
	logging. Full level enables hash and weight logging. Default: 2 (info)

	**Values:** none(-1) error(0) warning info debug full

.. option:: --output, -o <filename>

	Bitstream output file name

	**CLI ONLY**

.. option:: --no-progress

	Disable CLI periodic progress reports

	**CLI ONLY**

.. option:: --csv <filename>

	Writes encoding results to a comma separated value log file Creates
	the file if it doesnt already exist, else adds one line per run.  if
	:option:`--log-level` level is debug or above, it writes one line per
	frame. Default none

.. option:: --y4m

	Parse input stream as YUV4MPEG2 regardless of file extension,
	primarily intended for use with stdin
	(ie: :option:`--input` - :option:`--y4m`)

	**CLI ONLY**

Input Options
=============

.. option:: --input <filename>

	Input filename, only raw YUV or Y4M supported. Use single dash for
	stdin.

	**CLI ONLY**

.. option:: --input-depth <integer>

	Bit-depth of input file or stream (YUV only).

	**Values:** any value between 8 and 16. Default is internal depth.

	**CLI ONLY**

.. option:: --input-res <wxh>

	Source picture size [w x h], auto-detected if Y4M

	**CLI ONLY**

.. option:: --input-csp <integer|string>

	Source color space parameter, auto detected if Y4M

	**Values:** 1:"i420" **(Default)**, or 3:"i444"

.. option:: --fps <integer|float|numerator/denominator>

	Source frame rate; auto-detected if Y4M

	**Range of values:** positive int or float, or num/denom

.. option:: --seek <integer>

	Number of frames to skip at start of input file. Default 0

	**Range of values:** 0 to the number of frames in the video

	**CLI ONLY**

.. option:: --frames, -f <integer>

	Number of frames to be encoded. Default 0 (all)

	**CLI ONLY**


Quad-Tree analysis
==================

.. option:: --wpp, --no-wpp

	Enable Wavefront Parallel Processing. Default: Enabled

.. option:: --ctu, -s <64|32|16>

	Maximum CU size (width and height). Default: 64

.. option:: --tu-intra-depth <1..4>

	Max TU recursive depth for intra CUs. Default: 1

.. option:: --tu-inter-depth <1..4>

	Max TU recursive depth for inter CUs. Default: 1


Temporal / motion search options
================================

.. option:: --me <integer|string>

	Motion search method. Default: hex

	**Values** dia, hex, umh, star, full

.. option:: --subme, -m <0..7>

	Amount of subpel refinement to perform. Default 2

.. option:: --merange <integer>

	Motion search range. Default 57

	**Range of values:** an integer from 0 to 32768

.. option:: --rect, --no-rect

	Enable rectangular motion partitions Nx2N and 2NxN. Default enabled

.. option:: --amp, --no-amp

	Enable asymmetric motion partitions, requires rect. Default enabled

.. option:: --max-merge <1..5>

	Maximum number of merge candidates. Default 2

.. option:: --early-skip, --no-early-skip

	Enable early SKIP detection, Default disabled

.. option:: --fast-cbf, --no-fast-cbf

	Enable Cbf fast mode. Default disabled


Spatial/intra options
=====================

.. option:: --rdpenalty <0..2>

	Penalty for 32x32 intra TU in non-I slices.  Default 0

	**Values:** 0:disabled 1:RD-penalty 2:maximum

.. option:: --tskip, --no-tskip

	Enable intra transform skipping. Default disabled

.. option:: --tskip-fast, --no-tskip-fast

	Enable fast intra transform skip decisions. Default disabled

.. option:: --strong-intra-smoothing, --no-strong-intra-smoothing

	Enable strong intra smoothing for 32x32 blocks. Default enabled

.. option:: --constrained-intra, --no-constrained-intra

	Constrained intra prediction (use only intra coded reference pixels)
	Default disabled


Slice decision options
======================

.. option:: --open-gop, --no-open-gop

	Enable open GOP, allow I-slices to be non-IDR. Default enabled

.. option:: --keyint, -I <integer>

	Max intra period in frames. A special case of infinite-gop (single
	keyframe at the beginning of the stream) can be triggered with
	argument -1. Use 1 to force all-intra. Default 250

.. option:: --min-keyint, -i <integer>

	Minimum GOP size. Scenecuts closer together than this are coded as I or P, not IDR. 

	**Range of values:** >=0 (0: auto)

.. option:: --scenecut <integer>, --no-scenecut

	How aggressively I-frames need to be inserted. The higher the
	threshold value, the more aggressive the I-frame placement.
	:option:`--scenecut`=0 or :option:`--no-scenecut` disables adaptive
	I frame placement. Default 40

.. option:: --rc-lookahead <integer>

	Number of frames for frame-type lookahead (determines encoder latency). Default 20

	**Range of values:** an integer less than or equal to 250 and greater than maximum consecutive bframe count (:option:`--bframes`)

.. option:: --b-adapt <integer>

	Adaptive B frame scheduling. Default 2

	**Values:** 0:none; 1:fast; 2:full(trellis)

.. option:: --bframes, -b <integer>

	Maximum number of consecutive b-frames. Use :option:`--bframes` 0 to
	force all P/I low-latency encodes. Default 4

	**Range of values:** 0 to 16

.. option:: --bframe-bias <integer>

	Bias towards B frames in slicetype decision. The higher the bias the
	more likely x265 is to use B frames. Default 0

	**Range of values:** usually >=0 (increase the value for referring more B Frames e.g. 40-50)

.. option:: --b-pyramid <0|1>

	Use B-frames as references 0: Disabled, 1: Enabled **(Default)**

.. option:: --ref <integer>

	Max number of L0 references to be allowed. Default 3

	**Range of values:** 1 to 16

.. option:: --weightp, -w, --no-weightp

	Enable weighted prediction in P slices. Default enabled


Quality, rate control and rate distortion options
=================================================

.. option:: --bitrate <integer>

	Enables ABR rate control. Specify the target bitrate in kbps.  
	Default is 0 (CRF, no ABR)

	**Range of values:** An integer greater than 0

.. option:: --crf <0..51>

	Quality-controlled VBR. Default rate factor is 28

.. option:: --vbv-bufsize <integer>

	Enables VBV in ABR mode. Sets the size of the VBV buffer (kbits).
	Default 0 (disabled)

.. option:: --vbv-maxrate <integer>

	Maximum local bitrate (kbits/sec). Will be used only if vbv-bufsize
	is also non-zero. Both vbv-bufsize and vbv-maxrate are required to
	enable VBV in CRF mode. Default 0 (disabled)

.. option:: --vbv-init <float>

	Initial VBV buffer occupancy. The portion of the VBV which must be
	full before the decoder will begin decoding.  Determines absolute
	maximum frame size. Default 0.9

	**Range of values:** 0 - 1.0

.. option:: --qp, -q <integer>

	Base Quantization Parameter for Constant QP mode. Using this option
	causes x265 to use Constant QP rate control. Default 0 (CRF)

	**Range of values:** an integer from 0 to 51

.. option:: --aq-mode <0|1|2>

	Mode for Adaptive Quantization. Default 1

	0. disabled
	1. AQ enabled
	2. AQ enabled with auto-variance

.. option:: --aq-strength <float>

	Reduces blocking and blurring in flat and textured areas. Default 1.0

	**Range of values:** 0.0 to 3.0

.. option:: --cbqpoffs <integer>

	Chroma Cb QP Offset. Default 0

	**Range of values:** -12 to 12

.. option:: --crqpoffs <integer>

	Chroma Cr QP Offset. Default 0

	**Range of values:**  -12 to 12

.. option:: --rd <0..6>

	Level of RDO in mode decision. Default 3

	**Range of values:** 0: least .. 6: full RDO analysis

.. option:: --signhide, --no-signhide

	Hide sign bit of one coeff per TU (rdo). Default enabled
 
Loop filter
===========

.. option:: --lft, --no-lft

	Toggle deblocking loop filter, default enabled

.. option:: --sao, --no-sao

	Toggle Sample Adaptive Offset loop filter, default enabled

.. option:: --sao-lcu-bounds <0|1>

	0: right/bottom boundary areas skipped **(Default)**; 1: non-deblocked pixels are used

.. option:: --sao-lcu-opt <0|1>

	0: SAO picture-based optimization (requires -F1); 1: SAO LCU-based optimization **(Default)**

Quality reporting metrics
=========================

.. option:: --ssim, --no-ssim

	Calculate and report Structural Similarity values. Default disabled

.. option:: --psnr, --no-psnr

	Calculate and report Peak Signal to Noise Ratio. Default disabled

------------------------------

VUI (Video Usability Information) options
=========================================

By default, no VUI will be emitted by x265. If you enable any of the VUI
parts (sar or color primitives) the VUI itself is also enabled.

.. option:: --vui, --no-vui

	Enable video usability Information with all fields in the SPS. It is
	generally unnecessary to enable the VUI itself. Enabling any of the
	VUI properties will enable the VUI. This is mostly a debugging
	feature and will likely be removed in a later release.  Default
	disabled

.. option:: --sar <integer|w:h>

	Sample Aspect Ratio <int:int|int>, the ratio of width to height of an
	individual sample (pixel). The user may supply the width and height
	explicitly or specify an integer for the predefined list of aspect
	ratios defined in the HEVC specification.  Default undefined

	1. 1:1 (square)
	2. 12:11
	3. 10:11
	4. 16:11
	5. 40:33
	6. 24:11
	7. 20:11
	8. 32:11
	9. 80:33
	10. 18:11
	11. 15:11
	12. 64:33
	13. 160:99
	14. 4:3
	15. 3:2
	16. 2:1

.. option:: --crop-rect <left,top,right,bottom>

	Region of image that does not contain information was added to achieve
	certain resolution or aspect ratio. Default undefined

.. option:: --overscan <show|crop>

	Specify whether it is appropriate for the decoder to display the
	overscan area

.. option:: --videoformat <integer|string>

	Specify format of original analog video prior to digitizing and
	encoding. Default undefined

	0. component
	1. pal
	2. ntsc
	3. secam
	4. mac
	5. undefined

.. option:: --range <full|limited>

	Specify output range of black level and range of luma and chroma
	signals. Default undefined

.. option:: --colorprim <integer|string>

	Specify color primitive to use when converting to RGB. Default
	undefined

	1. bt709
	2. undef
	3. **reserved**
	4. bt470m
	5. bt470bg
	6. smpte170m
	7. smpte240m
	8. film
	9. bt2020

.. option:: --transfer <integer|string>

	Specify transfer characteristics. Default undefined

	1. bt709
	2. undef
	3. **reserved**
	4. bt470m
	5. bt470bg
	6. smpte170m
	7. smpte240m
	8. linear
	9. log100
	10. log316
	11. iec61966-2-4
	12. bt1361e
	13. iec61966-2-1
	14. bt2020-10
	15. bt2020-12

.. option:: --colormatrix <integer|string>

	Specify color matrix setting i.e set the matrix coefficients used in
	deriving the luma and chroma. Default undefined

	0. GBR
	1. bt709
	2. undef 
	3. **reserved**
	4. fcc
	5. bt470bg
	6. smpte170m
	7. smpte240m
	8. YCgCo
	9. bt2020nc
	10. bt2020c

.. option:: --chromalocs <0..5>

	Specify chroma sample location, default undefined

.. option:: --timinginfo, --no-timinginfo

	Add timing information (fps, timebase) to the VUI

.. option:: --nal-hrd, --no-nal-hrd

	Add signal HRD information [NOT IMPLEMENTED]

.. option:: --bitstreamrestriction, --no-bitstreamrestriction

	Specifies whether that the bitstream restriction parameters for the
	CVS are present. [NOT IMPLEMENTED]

.. option:: --subpichrd, --no-subpichrd

	Add sub picture HRD parameters to the HRD. [NOT IMPLEMENTED]


Debugging options
=======================================

.. option:: --hash <integer>

	Emit decoded picture hash SEI, to validate encoder state. Default None

	1. MD5
	2. CRC
	3. Checksum

.. option:: --recon, -r <filename>

	Reconstructed image YUV or Y4M output file name

	**CLI ONLY**

.. option:: --recon-depth <integer>

	Bit-depth of output file. This value defaults to the internal bit
	depth and is not currently allowed to be modified.

	**CLI ONLY**

.. vim: noet
