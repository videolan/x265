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

If there is an extra command line argument (not an option or an option
value) the CLI will treat it as the input filename.  This effectively
makes the :option:`--input` specifier optional for the input file. If
there are two extra arguments, the second is treated as the output
bitstream filename, making :option:`--output` also optional if the input
filename was implied. This makes :command:`x265 in.yuv out.hevc` a valid
command line. If there are more than two extra arguments, the CLI will
consider this an error and abort.

Generally, when an option expects a string value from a list of strings
the user may specify the integer ordinal of the value they desire. ie:
:option:`--log-level` 3 is equivalent to :option:`--log-level` debug.

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

	Number of threads for thread pool. Default 0 (detected CPU core
	count)

.. option:: --preset, -p <integer|string>

	Sets parameters to preselected values, trading off compression efficiency against 
	encoding speed. These parameters are applied before all other input parameters are 
	applied, and so you can override any parameters that these values control.

	0. ultrafast
	1. superfast
	2. veryfast
	3. faster
	4. fast
	5. medium **(default)**
	6. slow
	7. slower
	8. veryslow
	9. placebo

.. option:: --tune, -t <string>

	Tune the settings for a particular type of source or situation. The changes will
	be applied after :option:`--preset` but before all other parameters. Default none

	**Values:** psnr, ssim, zero-latency, fast-decode.

.. option:: --frame-threads, -F <integer>

	Number of concurrently encoded frames. Using a single frame thread
	gives a slight improvement in compression, since the entire reference
	frames are always available for motion compensation, but it has
	severe performance implications. Default is an autodetected count
	based on the number of CPU cores and whether WPP is enabled or not.

.. option:: --log-level <int|string>

	Logging level. Debug level enables per-frame QP, metric, and bitrate
	logging. If a CSV file is being generated, debug level makes the log
	be per-frame rather than per-encode. Full level enables hash and
	weight logging. -1 disables all logging, except certain fatal
	errors, and can be specified by the string "none".

	0. error
	1. warning
	2. info **(default)**
	3. debug
	4. full

.. option:: --csv <filename>

	Writes encoding results to a comma separated value log file. Creates
	the file if it doesnt already exist, else adds one line per run.  if
	:option:`--log-level` is debug or above, it writes one line per
	frame. Default none

.. option:: --output, -o <filename>

	Bitstream output file name

	**CLI ONLY**

.. option:: --no-progress

	Disable CLI periodic progress reports

	**CLI ONLY**

Input Options
=============

.. option:: --input <filename>

	Input filename, only raw YUV or Y4M supported. Use single dash for
	stdin. This option name will be implied for the first "extra"
	command line argument.

	**CLI ONLY**

.. option:: --y4m

	Parse input stream as YUV4MPEG2 regardless of file extension,
	primarily intended for use with stdin. This option is implied if
	the input filename has a ".y4m" extension
	(ie: :option:`--input` - :option:`--y4m`)

	**CLI ONLY**

.. option:: --input-depth <integer>

	YUV only: Bit-depth of input file or stream

	**Values:** any value between 8 and 16. Default is internal depth.

	**CLI ONLY**

.. option:: --input-res <wxh>

	YUV only: Source picture size [w x h]

	**CLI ONLY**

.. option:: --input-csp <integer|string>

	YUV only: Source color space. Only i420 and i444 are supported at
	this time.

	0. i400
	1. i420 **(default)**
	2. i422
	3. i444
	4. nv12
	5. nv16

.. option:: --fps <integer|float|numerator/denominator>

	YUV only: Source frame rate

	**Range of values:** positive int or float, or num/denom

.. option:: --interlaceMode <false|tff|bff>, --no-interlaceMode

	**EXPERIMENTAL** Specify interlace type of source pictures. 
	
	0. progressive pictures **(default)**
	1. top field first 
	2. bottom field first

	HEVC encodes interlaced content as fields. Fields must be provided to
	the encoder in the correct temporal order. The source dimensions
	must be field dimensions and the FPS must be in units of fields per
	second. The decoder must re-combine the fields in their correct
	orientation for display.

.. option:: --seek <integer>

	Number of frames to skip at start of input file. Default 0

	**CLI ONLY**

.. option:: --frames, -f <integer>

	Number of frames to be encoded. Default 0 (all)

	**CLI ONLY**


Quad-Tree analysis
==================

.. option:: --wpp, --no-wpp

	Enable Wavefront Parallel Processing. The encoder may begin encoding
	a row as soon as the row above it is at least two LCUs ahead in the
	encode process. This gives a 3-5x gain in parallelism for about 1%
	overhead in compression efficiency. Default: Enabled

.. option:: --ctu, -s <64|32|16>

	Maximum CU size (width and height). The larger the maximum CU size,
	the more efficiently x265 can encode flat areas of the picture,
	giving large reductions in bitrate. However this comes at a loss of
	parallelism with fewer rows of CUs that can be encoded in parallel,
	and less frame parallelism as well. Because of this the faster
	presets use a CU size of 32. Default: 64

.. option:: --tu-intra-depth <1..4>

	Max TU recursive depth for intra CUs. Default: 1

.. option:: --tu-inter-depth <1..4>

	Max TU recursive depth for inter CUs. Default: 1


Temporal / motion search options
================================

.. option:: --me <integer|string>

	Motion search method. Generally, the higher the number the harder
	the ME method will try to find an optimal match.

	0. dia
	1. hex **(default)**
	2. umh
	3. star
	4. full

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

	Maximum number of neighbor (spatial and temporal) candidate blocks
	that the encoder may consider for merging motion predictions. If a
	merge candidate results in no residual, it is immediately selected
	as a "skip".  Otherwise the merge candidates are tested as part of
	motion estimation when searching for the least cost inter option.
	The max candidate number is encoded in the SPS and determines the
	bit cost of signaling merge CUs.  Default 2

.. option:: --early-skip, --no-early-skip

	Enable early SKIP detection. Default disabled

.. option:: --fast-cbf, --no-fast-cbf

	Short circuit analysis if a prediction is found that does not set
	the coded block flag (aka: no residual was encoded).  It prevents
	the encoder from perhaps finding other predictions that also have no
	residual but perhaps require less signaling bits.  Default disabled

.. option:: --ref <1..16>

	Max number of L0 references to be allowed. Default 3

.. option:: --weightp, -w, --no-weightp

	Enable weighted prediction in P slices. Default enabled


Spatial/intra options
=====================

.. option:: --rdpenalty <0..2>

	Penalty for 32x32 intra TU in non-I slices. Default 0

	**Values:** 0:disabled 1:RD-penalty 2:maximum

.. option:: --tskip, --no-tskip

	Enable intra transform skipping (encode residual as coefficients)
	for intra coded TU. Default disabled

.. option:: --tskip-fast, --no-tskip-fast

	Enable fast intra transform skip decisions. Only applicable if
	transform skip is enabled. Default disabled

.. option:: --strong-intra-smoothing, --no-strong-intra-smoothing

	Enable strong intra smoothing for 32x32 intra blocks. Default enabled

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
	:option:`--scenecut` 0 or :option:`--no-scenecut` disables adaptive
	I frame placement. Default 40

.. option:: --rc-lookahead <integer>

	Number of frames for slice-type decision lookahead (a key
	determining factor for encoder latency). The longer the lookahead
	buffer the more accurate scenecut decisions will be, and the more
	effective cuTree will be at improving adaptive quant. Having a
	lookahead larger than the max keyframe interval is not helpful.
	Default 20

	**Range of values:** Between the maximum consecutive bframe count (:option:`--bframes`) and 250

.. option:: --b-adapt <integer>

	Adaptive B frame scheduling. Default 2

	**Values:** 0:none; 1:fast; 2:full(trellis)

.. option:: --bframes, -b <0..16>

	Maximum number of consecutive b-frames. Use :option:`--bframes` 0 to
	force all P/I low-latency encodes. Default 4. This parameter has a
	quadratic effect on the amount of memory allocated and the amount of
	work performed by the full trellis version of :option:`--b-adapt`
	lookahead.

.. option:: --bframe-bias <integer>

	Bias towards B frames in slicetype decision. The higher the bias the
	more likely x265 is to use B frames. Can be any value between -20
	and 100, but is typically between 10 and 30. Default 0

.. option:: --b-pyramid, --no-b-pyramid

	Use B-frames as references, when possible. Default enabled

Quality, rate control and rate distortion options
=================================================

.. option:: --bitrate <integer>

	Enables ABR rate control. Specify the target bitrate in kbps.  
	Default is 0 (CRF)

	**Range of values:** An integer greater than 0

.. option:: --crf <0..51>

	Quality-controlled VBR. Default rate factor is 28

.. option:: --max-crf <0..51>

	Specify an upper limit for which the adaptive rate factor algorithm
	can assign a rate factor for any given frame (ensuring a max QP).
	This is dangerous when CRF is used with VBV as it may result in
	buffer underruns. Default disabled

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

	Adaptive Quantization operating mode.

	0. disabled
	1. AQ enabled **(default)**
	2. AQ enabled with auto-variance

.. option:: --aq-strength <float>

	Reduces blocking and blurring in flat and textured areas. Default 1.0

	**Range of values:** 0.0 to 3.0

.. option:: --cutree, --no-cutree

	Enable the use of lookahead's lowres motion vector fields to
	determine the amount of reuse of each block to tune the quantization
	factors. CU blocks which are heavily reused as motion reference for
	later frames are given a lower QP (more bits) while CU blocks which
	are quickly changed and are not referenced are given less bits. This
	tends to improve detail in the backgrounds of video with less detail
	in areas of high motion. Default enabled

.. option:: --cbqpoffs <integer>

	Offset of Cb chroma QP from the luma QP selected by rate control.
	This is a general way to more or less bits to the chroma channel.
	Default 0

	**Range of values:** -12 to 12

.. option:: --crqpoffs <integer>

	Offset of Cr chroma QP from the luma QP selected by rate control.
	This is a general way to more or less bits to the chroma channel.
	Default 0

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

	How to handle depencency with deblocking filter

	0. right/bottom boundary areas skipped **(default)**
	1. non-deblocked pixels are used

.. option:: --sao-lcu-opt <0|1>

	Frame level or block level optimization

	0. SAO picture-based optimization (prevents frame parallelism,
	   effectively causes :option:`--frame-threads` 1)
	1. SAO LCU-based optimization **(default)**

Quality reporting metrics
=========================

.. option:: --ssim, --no-ssim

	Calculate and report Structural Similarity values. Default disabled
	It is recommended to use :option:`--tune` ssim if you are measuring
	ssim, else the results should not be used for comparison purposes.

.. option:: --psnr, --no-psnr

	Calculate and report Peak Signal to Noise Ratio. Default disabled
	It is recommended to use :option:`--tune` psnr if you are measuring
	PSNR, else the results should not be used for comparison purposes.

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

	Sample Aspect Ratio, the ratio of width to height of an individual
	sample (pixel). The user may supply the width and height explicitly
	or specify an integer from the predefined list of aspect ratios
	defined in the HEVC specification.  Default undefined

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

	Specify chroma sample location for 4:2:0 inputs. Default undefined

.. option:: --timinginfo, --no-timinginfo

	Add timing information (fps, timebase) to the VUI. Default disabled

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
