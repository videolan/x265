*********************
Command Line Options
*********************

.. _string-options-ref:

Note that unless an option is listed as **CLI ONLY** the option is also
supported by x265_param_parse(). The CLI uses getopt to parse the
command line options so the short or long versions may be used and the
long options may be truncated to the shortest unambiguous abbreviation.
Users of the API must pass x265_param_parse() the full option name.

Preset and tune have special implications. The API user must call
x265_param_default_preset() with the preset and tune parameters they
wish to use, prior to calling x265_param_parse() to set any additional
fields. The CLI does this for the user implicitly, so all CLI options
are applied after the user's preset and tune choices, regardless of the
order of the arguments on the command line.

If there is an extra command line argument (not an option or an option
value) the CLI will treat it as the input filename.  This effectively
makes the :option:`--input` specifier optional for the input file. If
there are two extra arguments, the second is treated as the output
bitstream filename, making :option:`--output` also optional if the input
filename was implied. This makes :command:`x265 in.y4m out.hevc` a valid
command line. If there are more than two extra arguments, the CLI will
consider this an error and abort.

Generally, when an option expects a string value from a list of strings
the user may specify the integer ordinal of the value they desire. ie:
:option:`--log-level` 3 is equivalent to :option:`--log-level` debug.

Executable Options
==================

.. option:: --help, -h

	Display help text

	**CLI ONLY**

.. option:: --version, -V

	Display version details

	**CLI ONLY**

Command line executable return codes::

	0. encode successful
	1. unable to parse command line
	2. unable to open encoder
	3. unable to generate stream headers
	4. encoder abort
	5. unable to open csv file

Logging/Statistic Options
=========================

.. option:: --log-level <integer|string>

	Logging level. Debug level enables per-frame QP, metric, and bitrate
	logging. If a CSV file is being generated, frame level makes the log
	be per-frame rather than per-encode. Full level enables hash and
	weight logging. -1 disables all logging, except certain fatal
	errors, and can be specified by the string "none".

	0. error
	1. warning
	2. info **(default)**
	3. debug
	4. full

.. option:: --no-progress

	Disable periodic progress reports from the CLI

	**CLI ONLY**

.. option:: --csv <filename>

	Writes encoding results to a comma separated value log file. Creates
	the file if it doesnt already exist. If :option:`--csv-log-level` is 0, 
	it adds one line per run. If :option:`--csv-log-level` is greater than
	0, it writes one line per frame. Default none

	Several frame performance statistics are available when 
	:option:`--csv-log-level` is greater than or equal to 2:

	**DecideWait ms** number of milliseconds the frame encoder had to
	wait, since the previous frame was retrieved by the API thread,
	before a new frame has been given to it. This is the latency
	introduced by slicetype decisions (lookahead).
	
	**Row0Wait ms** number of milliseconds since the frame encoder
	received a frame to encode before its first row of CTUs is allowed
	to begin compression. This is the latency introduced by reference
	frames making reconstructed and filtered rows available.
	
	**Wall time ms** number of milliseconds between the first CTU
	being ready to be compressed and the entire frame being compressed
	and the output NALs being completed.
	
	**Ref Wait Wall ms** number of milliseconds between the first
	reference row being available and the last reference row becoming
	available.
	
	**Total CTU time ms** the total time (measured in milliseconds)
	spent by worker threads compressing and filtering CTUs for this
	frame.
	
	**Stall Time ms** the number of milliseconds of the reported wall
	time that were spent with zero worker threads, aka all compression
	was completely stalled.

	**Avg WPP** the average number of worker threads working on this
	frame, at any given time. This value is sampled at the completion of
	each CTU. This shows the effectiveness of Wavefront Parallel
	Processing.

	**Row Blocks** the number of times a worker thread had to abandon
	the row of CTUs it was encoding because the row above it was not far
	enough ahead for the necessary reference data to be available. This
	is more of a problem for P frames where some blocks are much more
	expensive than others.
	
	**CLI ONLY**

.. option:: --csv-log-level <integer>

        CSV logging level. Default 0
        0. summary
        1. frame level logging
        2. frame level logging with performance statistics

        **CLI ONLY**

.. option:: --ssim, --no-ssim

	Calculate and report Structural Similarity values. It is
	recommended to use :option:`--tune` ssim if you are measuring ssim,
	else the results should not be used for comparison purposes.
	Default disabled

.. option:: --psnr, --no-psnr

	Calculate and report Peak Signal to Noise Ratio.  It is recommended
	to use :option:`--tune` psnr if you are measuring PSNR, else the
	results should not be used for comparison purposes.  Default
	disabled

Performance Options
===================

.. option:: --asm <integer:false:string>, --no-asm

	x265 will use all detected CPU SIMD architectures by default. You can
	disable all assembly by using :option:`--no-asm` or you can specify
	a comma separated list of SIMD architectures to use, matching these
	strings: MMX2, SSE, SSE2, SSE3, SSSE3, SSE4, SSE4.1, SSE4.2, AVX, XOP, FMA4, AVX2, FMA3

	Some higher architectures imply lower ones being present, this is
	handled implicitly.

	One may also directly supply the CPU capability bitmap as an integer.
	
	Note that by specifying this option you are overriding x265's CPU
	detection and it is possible to do this wrong. You can cause encoder
	crashes by specifying SIMD architectures which are not supported on
	your CPU.

	Default: auto-detected SIMD architectures

.. option:: --frame-threads, -F <integer>

	Number of concurrently encoded frames. Using a single frame thread
	gives a slight improvement in compression, since the entire reference
	frames are always available for motion compensation, but it has
	severe performance implications. Default is an autodetected count
	based on the number of CPU cores and whether WPP is enabled or not.

	Over-allocation of frame threads will not improve performance, it
	will generally just increase memory use.

	**Values:** any value between 0 and 16. Default is 0, auto-detect

.. option:: --pools <string>, --numa-pools <string>

	Comma seperated list of threads per NUMA node. If "none", then no worker
	pools are created and only frame parallelism is possible. If NULL or ""
	(default) x265 will use all available threads on each NUMA node::

	'+'  is a special value indicating all cores detected on the node
	'*'  is a special value indicating all cores detected on the node and all remaining nodes
	'-'  is a special value indicating no cores on the node, same as '0'

	example strings for a 4-node system::

	""        - default, unspecified, all numa nodes are used for thread pools
	"*"       - same as default
	"none"    - no thread pools are created, only frame parallelism possible
	"-"       - same as "none"
	"10"      - allocate one pool, using up to 10 cores on node 0
	"-,+"     - allocate one pool, using all cores on node 1
	"+,-,+"   - allocate one pool, using only cores on nodes 0 and 2
	"+,-,+,-" - allocate one pool, using only cores on nodes 0 and 2
	"-,*"     - allocate one pool, using all cores on nodes 1, 2 and 3
	"8,8,8,8" - allocate four pools with up to 8 threads in each pool
	"8,+,+,+" - allocate two pools, the first with 8 threads on node 0, and the second with all cores on node 1,2,3

	A thread pool dedicated to a given NUMA node is enabled only when the
	number of threads to be created on that NUMA node is explicitly mentioned
	in that corresponding position with the --pools option. Else, all threads
	are spawned from a single pool. The total number of threads will be
	determined by the number of threads assigned to the enabled NUMA nodes for
	that pool. The worker threads are be given affinity to all the enabled
	NUMA nodes for that pool and may migrate between them, unless explicitly
	specified as described above.

	In the case that any threadpool has more than 64 threads, the threadpool
	may be broken down into multiple pools of 64 threads each; on 32-bit
	machines, this number is 32. All pools are given affinity to the NUMA
	nodes on which the original pool had affinity. For performance reasons,
	the last thread pool is spawned only if it has more than 32 threads for
	64-bit machines, or 16 for 32-bit machines. If the total number of threads
	in the system doesn't obey this constraint, we may spawn fewer threads
	than cores which has been emperically shown to be better for performance. 

	If the four pool features: :option:`--wpp`, :option:`--pmode`,
	:option:`--pme` and :option:`--lookahead-slices` are all disabled,
	then :option:`--pools` is ignored and no thread pools are created.

	If "none" is specified, then all four of the thread pool features are
	implicitly disabled.

	Frame encoders are distributed between the available thread pools,
	and the encoder will never generate more thread pools than
	:option:`--frame-threads`.  The pools are used for WPP and for
	distributed analysis and motion search.

	On Windows, the native APIs offer sufficient functionality to
	discover the NUMA topology and enforce the thread affinity that
	libx265 needs (so long as you have not chosen to target XP or
	Vista), but on POSIX systems it relies on libnuma for this
	functionality. If your target POSIX system is single socket, then
	building without libnuma is a perfectly reasonable option, as it
	will have no effect on the runtime behavior. On a multiple-socket
	system, a POSIX build of libx265 without libnuma will be less work
	efficient. See :ref:`thread pools <pools>` for more detail.

	Default "", one pool is created across all available NUMA nodes, with
	one thread allocated per detected hardware thread
	(logical CPU cores). In the case that the total number of threads is more
	than the maximum size that ATOMIC operations can handle (32 for 32-bit
	compiles, and 64 for 64-bit compiles), multiple thread pools may be
	spawned subject to the performance constraint described above.

	Note that the string value will need to be escaped or quoted to
	protect against shell expansion on many platforms

.. option:: --wpp, --no-wpp

	Enable Wavefront Parallel Processing. The encoder may begin encoding
	a row as soon as the row above it is at least two CTUs ahead in the
	encode process. This gives a 3-5x gain in parallelism for about 1%
	overhead in compression efficiency.

	This feature is implicitly disabled when no thread pool is present.

	Default: Enabled

.. option:: --pmode, --no-pmode

	Parallel mode decision, or distributed mode analysis. When enabled
	the encoder will distribute the analysis work of each CU (merge,
	inter, intra) across multiple worker threads. Only recommended if
	x265 is not already saturating the CPU cores. In RD levels 3 and 4
	it will be most effective if --rect is enabled. At RD levels 5 and
	6 there is generally always enough work to distribute to warrant the
	overhead, assuming your CPUs are not already saturated.
	
	--pmode will increase utilization without reducing compression
	efficiency. In fact, since the modes are all measured in parallel it
	makes certain early-outs impractical and thus you usually get
	slightly better compression when it is enabled (at the expense of
	not skipping improbable modes). This bypassing of early-outs can
	cause pmode to slow down encodes, especially at faster presets.

	This feature is implicitly disabled when no thread pool is present.

	Default disabled

.. option:: --pme, --no-pme

	Parallel motion estimation. When enabled the encoder will distribute
	motion estimation across multiple worker threads when more than two
	references require motion searches for a given CU. Only recommended
	if x265 is not already saturating CPU cores. :option:`--pmode` is
	much more effective than this option, since the amount of work it
	distributes is substantially higher. With --pme it is not unusual
	for the overhead of distributing the work to outweigh the
	parallelism benefits.
	
	This feature is implicitly disabled when no thread pool is present.

	--pme will increase utilization on many core systems with no effect
	on the output bitstream.
	
	Default disabled

.. option:: --preset, -p <integer|string>

	Sets parameters to preselected values, trading off compression efficiency against 
	encoding speed. These parameters are applied before all other input parameters are 
	applied, and so you can override any parameters that these values control.  See
	:ref:`presets <presets>` for more detail.

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
	be applied after :option:`--preset` but before all other parameters. Default none.
	See :ref:`tunings <tunings>` for more detail.

	**Values:** psnr, ssim, grain, zero-latency, fast-decode.

Input/Output File Options
=========================

These options all describe the input video sequence or, in the case of
:option:`--dither`, operations that are performed on the sequence prior
to encode. All options dealing with files (names, formats, offsets or
frame counts) are only applicable to the CLI application.

.. option:: --input <filename>

	Input filename, only raw YUV or Y4M supported. Use single dash for
	stdin. This option name will be implied for the first "extra"
	command line argument.

	**CLI ONLY**

.. option:: --y4m

	Parse input stream as YUV4MPEG2 regardless of file extension,
	primarily intended for use with stdin (ie: :option:`--input` -
	:option:`--y4m`).  This option is implied if the input filename has
	a ".y4m" extension

	**CLI ONLY**

.. option:: --input-depth <integer>

	YUV only: Bit-depth of input file or stream

	**Values:** any value between 8 and 16. Default is internal depth.

	**CLI ONLY**

.. option:: --total-frames <integer>

	The number of frames intended to be encoded.  It may be left
	unspecified, but when it is specified rate control can make use of
	this information. It is also used to determine if an encode is
	actually a stillpicture profile encode (single frame)

.. option:: --dither

	Enable high quality downscaling. Dithering is based on the diffusion
	of errors from one row of pixels to the next row of pixels in a
	picture. Only applicable when the input bit depth is larger than
	8bits and internal bit depth is 8bits. Default disabled

	**CLI ONLY**

.. option:: --input-res <wxh>

	YUV only: Source picture size [w x h]

	**CLI ONLY**

.. option:: --input-csp <integer|string>

	YUV only: Source color space. Only i420, i422, and i444 are
	supported at this time. The internal color space is always the
	same as the source color space (libx265 does not support any color
	space conversions).

	0. i400
	1. i420 **(default)**
	2. i422
	3. i444
	4. nv12
	5. nv16

.. option:: --fps <integer|float|numerator/denominator>

	YUV only: Source frame rate

	**Range of values:** positive int or float, or num/denom

.. option:: --interlace <false|tff|bff>, --no-interlace

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

	Number of frames of input sequence to be encoded. Default 0 (all)

	**CLI ONLY**

.. option:: --output, -o <filename>

	Bitstream output file name. If there are two extra CLI options, the
	first is implicitly the input filename and the second is the output
	filename, making the :option:`--output` option optional.

	The output file will always contain a raw HEVC bitstream, the CLI
	does not support any container file formats.

	**CLI ONLY**

.. option:: --output-depth, -D 8|10|12

	Bitdepth of output HEVC bitstream, which is also the internal bit
	depth of the encoder. If the requested bit depth is not the bit
	depth of the linked libx265, it will attempt to bind libx265_main
	for an 8bit encoder, libx265_main10 for a 10bit encoder, or
	libx265_main12 for a 12bit encoder, with the same API version as the
	linked libx265.

	If the output depth is not specified but :option:`--profile` is
	specified, the output depth will be derived from the profile name.

	**CLI ONLY**

Profile, Level, Tier
====================

.. option:: --profile, -P <string>

	Enforce the requirements of the specified profile, ensuring the
	output stream will be decodable by a decoder which supports that
	profile.  May abort the encode if the specified profile is
	impossible to be supported by the compile options chosen for the
	encoder (a high bit depth encoder will be unable to output
	bitstreams compliant with Main or MainStillPicture).

	The first version of the HEVC specification only described Main,
	Main10, and MainStillPicture. All other profiles were added by the
	Range Extensions additions in HEVC version two.

	8bit profiles::

	main, main-intra, mainstillpicture (or msp for short)
	main444-8 main444-intra main444-stillpicture

	10bit profiles::

	main10, main10-intra
	main422-10, main422-10-intra
	main444-10, main444-10-intra

	12bit profiles::

	main12, main12-intra
	main422-12, main422-12-intra
	main444-12, main444-12-intra

	16bit profiles::

	main444-16-intra main444-16-stillpicture

	**CLI ONLY**

	API users must call x265_param_apply_profile() after configuring
	their param structure. Any changes made to the param structure after
	this call might make the encode non-compliant.

	The CLI application will derive the output bit depth from the
	profile name if :option:`--output-depth` is not specified.

.. option:: --level-idc <integer|float>

	Minimum decoder requirement level. Defaults to 0, which implies
	auto-detection by the encoder. If specified, the encoder will
	attempt to bring the encode specifications within that specified
	level. If the encoder is unable to reach the level it issues a
	warning and aborts the encode. If the requested requirement level is
	higher than the actual level, the actual requirement level is
	signaled.

	Beware, specifying a decoder level will force the encoder to enable
	VBV for constant rate factor encodes, which may introduce
	non-determinism.

	The value is specified as a float or as an integer with the level
	times 10, for example level **5.1** is specified as "5.1" or "51",
	and level **5.0** is specified as "5.0" or "50".

	Annex A levels: 1, 2, 2.1, 3, 3.1, 4, 4.1, 5, 5.1, 5.2, 6, 6.1, 6.2, 8.5

.. option:: --high-tier, --no-high-tier

	If :option:`--level-idc` has been specified, the option adds the
	intention to support the High tier of that level. If your specified
	level does not support a High tier, a warning is issued and this
	modifier flag is ignored. If :option:`--level-idc` has been specified,
	but not --high-tier, then the encoder will attempt to encode at the 
	specified level, main tier first, turning on high tier only if 
	necessary and available at that level.

	If :option:`--level-idc` has not been specified, this argument is
	ignored.

.. option:: --ref <1..16>

	Max number of L0 references to be allowed. This number has a linear
	multiplier effect on the amount of work performed in motion search,
	but will generally have a beneficial affect on compression and
	distortion.
	
	Note that x265 allows up to 16 L0 references but the HEVC
	specification only allows a maximum of 8 total reference frames. So
	if you have B frames enabled only 7 L0 refs are valid and if you
	have :option:`--b-pyramid` enabled (which is enabled by default in
	all presets), then only 6 L0 refs are the maximum allowed by the
	HEVC specification.  If x265 detects that the total reference count
	is greater than 8, it will issue a warning that the resulting stream
	is non-compliant and it signals the stream as profile NONE and level
	NONE and will abort the encode unless
	:option:`--allow-non-conformance` it specified.  Compliant HEVC
	decoders may refuse to decode such streams.
	
	Default 3

.. option:: --allow-non-conformance, --no-allow-non-conformance

	Allow libx265 to generate a bitstream with profile and level NONE.
	By default it will abort any encode which does not meet strict level
	compliance. The two most likely causes for non-conformance are
	:option:`--ctu` being too small, :option:`--ref` being too high,
	or the bitrate or resolution being out of specification.

	Default: disabled

.. note::

	:option:`--profile`, :option:`--level-idc`, and
	:option:`--high-tier` are only intended for use when you are
	targeting a particular decoder (or decoders) with fixed resource
	limitations and must constrain the bitstream within those limits.
	Specifying a profile or level may lower the encode quality
	parameters to meet those requirements but it will never raise
	them. It may enable VBV constraints on a CRF encode.

	Also note that x265 determines the decoder requirement profile and
	level in three steps.  First, the user configures an x265_param
	structure with their suggested encoder options and then optionally
	calls x265_param_apply_profile() to enforce a specific profile
	(main, main10, etc). Second, an encoder is created from this
	x265_param instance and the :option:`--level-idc` and
	:option:`--high-tier` parameters are used to reduce bitrate or other
	features in order to enforce the target level. Finally, the encoder
	re-examines the final set of parameters and detects the actual
	minimum decoder requirement level and this is what is signaled in
	the bitstream headers. The detected decoder level will only use High
	tier if the user specified a High tier level.

	The signaled profile will be determined by the encoder's internal
	bitdepth and input color space. If :option:`--keyint` is 0 or 1,
	then an intra variant of the profile will be signaled.

	If :option:`--total-frames` is 1, then a stillpicture variant will
	be signaled, but this parameter is not always set by applications,
	particularly not when the CLI uses stdin streaming or when libx265
	is used by third-party applications.


Mode decision / Analysis
========================

.. option:: --rd <0..6>

	Level of RDO in mode decision. The higher the value, the more
	exhaustive the analysis and the more rate distortion optimization is
	used. The lower the value the faster the encode, the higher the
	value the smaller the bitstream (in general). Default 3

	Note that this table aims for accuracy, but is not necessarily our
	final target behavior for each mode.

	+-------+---------------------------------------------------------------+
	| Level | Description                                                   |
	+=======+===============================================================+
	| 0     | sa8d mode and split decisions, intra w/ source pixels         |
	+-------+---------------------------------------------------------------+
	| 1     | recon generated (better intra), RDO merge/skip selection      |
	+-------+---------------------------------------------------------------+
	| 2     | RDO splits and merge/skip selection                           |
	+-------+---------------------------------------------------------------+
	| 3     | RDO mode and split decisions, chroma residual used for sa8d   |
	+-------+---------------------------------------------------------------+
	| 4     | Currently same as 3                                           |
	+-------+---------------------------------------------------------------+
	| 5     | Adds RDO prediction decisions                                 |
	+-------+---------------------------------------------------------------+
	| 6     | Currently same as 5                                           |
	+-------+---------------------------------------------------------------+

	**Range of values:** 0: least .. 6: full RDO analysis

Options which affect the coding unit quad-tree, sometimes referred to as
the prediction quad-tree.

.. option:: --ctu, -s <64|32|16>

	Maximum CU size (width and height). The larger the maximum CU size,
	the more efficiently x265 can encode flat areas of the picture,
	giving large reductions in bitrate. However this comes at a loss of
	parallelism with fewer rows of CUs that can be encoded in parallel,
	and less frame parallelism as well. Because of this the faster
	presets use a CU size of 32. Default: 64

.. option:: --min-cu-size <64|32|16|8>

	Minimum CU size (width and height). By using 16 or 32 the encoder
	will not analyze the cost of CUs below that minimum threshold,
	saving considerable amounts of compute with a predictable increase
	in bitrate. This setting has a large effect on performance on the
	faster presets.

	Default: 8 (minimum 8x8 CU for HEVC, best compression efficiency)

.. note::

	All encoders within a single process must use the same settings for
	the CU size range. :option:`--ctu` and :option:`--min-cu-size` must
	be consistent for all of them since the encoder configures several
	key global data structures based on this range.

.. option:: --limit-refs <0|1|2|3>

	When set to X265_REF_LIMIT_DEPTH (1) x265 will limit the references
	analyzed at the current depth based on the references used to code
	the 4 sub-blocks at the next depth.  For example, a 16x16 CU will
	only use the references used to code its four 8x8 CUs.

	When set to X265_REF_LIMIT_CU (2), the rectangular and asymmetrical
	partitions will only use references selected by the 2Nx2N motion
	search (including at the lowest depth which is otherwise unaffected
	by the depth limit).

	When set to 3 (X265_REF_LIMIT_DEPTH && X265_REF_LIMIT_CU), the 2Nx2N 
	motion search at each depth will only use references from the split 
	CUs and the rect/amp motion searches at that depth will only use the 
	reference(s) selected by 2Nx2N. 

	For all non-zero values of limit-refs, the current depth will evaluate
	intra mode (in inter slices), only if intra mode was chosen as the best
	mode for atleast one of the 4 sub-blocks.

	You can often increase the number of references you are using
	(within your decoder level limits) if you enable one or
	both of these flags.

	This feature is EXPERIMENTAL and functional at all RD levels.

.. option:: --rect, --no-rect

	Enable analysis of rectangular motion partitions Nx2N and 2NxN
	(50/50 splits, two directions). Default disabled

.. option:: --amp, --no-amp

	Enable analysis of asymmetric motion partitions (75/25 splits, four
	directions). At RD levels 0 through 4, AMP partitions are only
	considered at CU sizes 32x32 and below. At RD levels 5 and 6, it
	will only consider AMP partitions as merge candidates (no motion
	search) at 64x64, and as merge or inter candidates below 64x64.

	The AMP partitions which are searched are derived from the current
	best inter partition. If Nx2N (vertical rectangular) is the best
	current prediction, then left and right asymmetrical splits will be
	evaluated. If 2NxN (horizontal rectangular) is the best current
	prediction, then top and bottom asymmetrical splits will be
	evaluated, If 2Nx2N is the best prediction, and the block is not a
	merge/skip, then all four AMP partitions are evaluated.

	This setting has no effect if rectangular partitions are disabled.
	Default disabled

.. option:: --early-skip, --no-early-skip

	Measure full CU size (2Nx2N) merge candidates first; if no residual
	is found the analysis is short circuited. Default disabled

.. option:: --fast-intra, --no-fast-intra

	Perform an initial scan of every fifth intra angular mode, then
	check modes +/- 2 distance from the best mode, then +/- 1 distance
	from the best mode, effectively performing a gradient descent. When
	enabled 10 modes in total are checked. When disabled all 33 angular
	modes are checked.  Only applicable for :option:`--rd` levels 4 and
	below (medium preset and faster).

.. option:: --b-intra, --no-b-intra

	Enables the evaluation of intra modes in B slices. Default disabled.

.. option:: --cu-lossless, --no-cu-lossless

	For each CU, evaluate lossless (transform and quant bypass) encode
	of the best non-lossless mode option as a potential rate distortion
	optimization. If the global option :option:`--lossless` has been
	specified, all CUs will be encoded as lossless unconditionally
	regardless of whether this option was enabled. Default disabled.

	Only effective at RD levels 3 and above, which perform RDO mode
	decisions.

.. option:: --tskip-fast, --no-tskip-fast

	Only evaluate transform skip for NxN intra predictions (4x4 blocks).
	Only applicable if transform skip is enabled. For chroma, only
	evaluate if luma used tskip. Inter block tskip analysis is
	unmodified. Default disabled

Analysis re-use options, to improve performance when encoding the same
sequence multiple times (presumably at varying bitrates). The encoder
will not reuse analysis if the resolution and slice type parameters do
not match.

.. option:: --analysis-mode <string|int>

	Specify whether analysis information of each frame is output by encoder
	or input for reuse. By reading the analysis data writen by an
	earlier encode of the same sequence, substantial redundant work may
	be avoided.

	The following data may be stored and reused:
	I frames   - split decisions and luma intra directions of all CUs.
	P/B frames - motion vectors are dumped at each depth for all CUs.

	**Values:** off(0), save(1): dump analysis data, load(2): read analysis data

.. option:: --analysis-file <filename>

	Specify a filename for analysis data (see :option:`--analysis-mode`)
	If no filename is specified, x265_analysis.dat is used.

Options which affect the transform unit quad-tree, sometimes referred to
as the residual quad-tree (RQT).

.. option:: --rdoq-level <0|1|2>, --no-rdoq-level

	Specify the amount of rate-distortion analysis to use within
	quantization::

	At level 0 rate-distortion cost is not considered in quant
	
	At level 1 rate-distortion cost is used to find optimal rounding
	values for each level (and allows psy-rdoq to be effective). It
	trades-off the signaling cost of the coefficient vs its post-inverse
	quant distortion from the pre-quant coefficient. When
	:option:`--psy-rdoq` is enabled, this formula is biased in favor of
	more energy in the residual (larger coefficient absolute levels)
	
	At level 2 rate-distortion cost is used to make decimate decisions
	on each 4x4 coding group, including the cost of signaling the group
	within the group bitmap. If the total distortion of not signaling
	the entire coding group is less than the rate cost, the block is
	decimated. Next, it applies rate-distortion cost analysis to the
	last non-zero coefficient, which can result in many (or all) of the
	coding groups being decimated. Psy-rdoq is less effective at
	preserving energy when RDOQ is at level 2, since it only has
	influence over the level distortion costs.

.. option:: --tu-intra-depth <1..4>

	The transform unit (residual) quad-tree begins with the same depth
	as the coding unit quad-tree, but the encoder may decide to further
	split the transform unit tree if it improves compression efficiency.
	This setting limits the number of extra recursion depth which can be
	attempted for intra coded units. Default: 1, which means the
	residual quad-tree is always at the same depth as the coded unit
	quad-tree
	
	Note that when the CU intra prediction is NxN (only possible with
	8x8 CUs), a TU split is implied, and thus the residual quad-tree
	begins at 4x4 and cannot split any futhrer.

.. option:: --tu-inter-depth <1..4>

	The transform unit (residual) quad-tree begins with the same depth
	as the coding unit quad-tree, but the encoder may decide to further
	split the transform unit tree if it improves compression efficiency.
	This setting limits the number of extra recursion depth which can be
	attempted for inter coded units. Default: 1. which means the
	residual quad-tree is always at the same depth as the coded unit
	quad-tree unless the CU was coded with rectangular or AMP
	partitions, in which case a TU split is implied and thus the
	residual quad-tree begins one layer below the CU quad-tree.

.. option:: --nr-intra <integer>, --nr-inter <integer>

	Noise reduction - an adaptive deadzone applied after DCT
	(subtracting from DCT coefficients), before quantization.  It does
	no pixel-level filtering, doesn't cross DCT block boundaries, has no
	overlap, The higher the strength value parameter, the more
	aggressively it will reduce noise.

	Enabling noise reduction will make outputs diverge between different
	numbers of frame threads. Outputs will be deterministic but the
	outputs of -F2 will no longer match the outputs of -F3, etc.

	**Values:** any value in range of 0 to 2000. Default 0 (disabled).

.. option:: --tskip, --no-tskip

	Enable evaluation of transform skip (bypass DCT but still use
	quantization) coding for 4x4 TU coded blocks.

	Only effective at RD levels 3 and above, which perform RDO mode
	decisions. Default disabled

.. option:: --rdpenalty <0..2>

	When set to 1, transform units of size 32x32 are given a 4x bit cost
	penalty compared to smaller transform units, in intra coded CUs in P
	or B slices.

	When set to 2, transform units of size 32x32 are not even attempted,
	unless otherwise required by the maximum recursion depth.  For this
	option to be effective with 32x32 intra CUs,
	:option:`--tu-intra-depth` must be at least 2.  For it to be
	effective with 64x64 intra CUs, :option:`--tu-intra-depth` must be
	at least 3.

	Note that in HEVC an intra transform unit (a block of the residual
	quad-tree) is also a prediction unit, meaning that the intra
	prediction signal is generated for each TU block, the residual
	subtracted and then coded. The coding unit simply provides the
	prediction modes that will be used when predicting all of the
	transform units within the CU. This means that when you prevent
	32x32 intra transform units, you are preventing 32x32 intra
	predictions.

	Default 0, disabled.

	**Values:** 0:disabled 1:4x cost penalty 2:force splits

.. option:: --max-tu-size <32|16|8|4>

	Maximum TU size (width and height). The residual can be more
	efficiently compressed by the DCT transform when the max TU size
	is larger, but at the expense of more computation. Transform unit
	quad-tree begins at the same depth of the coded tree unit, but if the
	maximum TU size is smaller than the CU size then transform QT begins 
	at the depth of the max-tu-size. Default: 32.

Temporal / motion search options
================================

.. option:: --max-merge <1..5>

	Maximum number of neighbor (spatial and temporal) candidate blocks
	that the encoder may consider for merging motion predictions. If a
	merge candidate results in no residual, it is immediately selected
	as a "skip".  Otherwise the merge candidates are tested as part of
	motion estimation when searching for the least cost inter option.
	The max candidate number is encoded in the SPS and determines the
	bit cost of signaling merge CUs. Default 2

.. option:: --me <integer|string>

	Motion search method. Generally, the higher the number the harder
	the ME method will try to find an optimal match. Diamond search is
	the simplest. Hexagon search is a little better. Uneven
	Multi-Hexegon is an adaption of the search method used by x264 for
	slower presets. Star is a three step search adapted from the HM
	encoder: a star-pattern search followed by an optional radix scan
	followed by an optional star-search refinement. Full is an
	exhaustive search; an order of magnitude slower than all other
	searches but not much better than umh or star.

	0. dia
	1. hex **(default)**
	2. umh
	3. star
	4. full

.. option:: --subme, -m <0..7>

	Amount of subpel refinement to perform. The higher the number the
	more subpel iterations and steps are performed. Default 2

	+----+------------+-----------+------------+-----------+-----------+
	| -m | HPEL iters | HPEL dirs | QPEL iters | QPEL dirs | HPEL SATD |
	+====+============+===========+============+===========+===========+
	|  0 | 1          | 4         | 0          | 4         | false     |
	+----+------------+-----------+------------+-----------+-----------+
	|  1 | 1          | 4         | 1          | 4         | false     |
	+----+------------+-----------+------------+-----------+-----------+
	|  2 | 1          | 4         | 1          | 4         | true      |
	+----+------------+-----------+------------+-----------+-----------+
	|  3 | 2          | 4         | 1          | 4         | true      |
	+----+------------+-----------+------------+-----------+-----------+
	|  4 | 2          | 4         | 2          | 4         | true      |
	+----+------------+-----------+------------+-----------+-----------+
	|  5 | 1          | 8         | 1          | 8         | true      |
	+----+------------+-----------+------------+-----------+-----------+
	|  6 | 2          | 8         | 1          | 8         | true      |
	+----+------------+-----------+------------+-----------+-----------+
	|  7 | 2          | 8         | 2          | 8         | true      |
	+----+------------+-----------+------------+-----------+-----------+

	At --subme values larger than 2, chroma residual cost is included
	in all subpel refinement steps and chroma residual is included in
	all motion estimation decisions (selecting the best reference
	picture in each list, and chosing between merge, uni-directional
	motion and bi-directional motion). The 'slow' preset is the first
	preset to enable the use of chroma residual.

.. option:: --merange <integer>

	Motion search range. Default 57

	The default is derived from the default CTU size (64) minus the luma
	interpolation half-length (4) minus maximum subpel distance (2)
	minus one extra pixel just in case the hex search method is used. If
	the search range were any larger than this, another CTU row of
	latency would be required for reference frames.

	**Range of values:** an integer from 0 to 32768

.. option:: --temporal-mvp, --no-temporal-mvp

	Enable temporal motion vector predictors in P and B slices.
	This enables the use of the motion vector from the collocated block
	in the previous frame to be used as a predictor. Default is enabled

.. option:: --weightp, -w, --no-weightp

	Enable weighted prediction in P slices. This enables weighting
	analysis in the lookahead, which influences slice decisions, and
	enables weighting analysis in the main encoder which allows P
	reference samples to have a weight function applied to them prior to
	using them for motion compensation.  In video which has lighting
	changes, it can give a large improvement in compression efficiency.
	Default is enabled

.. option:: --weightb, --no-weightb

	Enable weighted prediction in B slices. Default disabled

Spatial/intra options
=====================

.. option:: --strong-intra-smoothing, --no-strong-intra-smoothing

	Enable strong intra smoothing for 32x32 intra blocks. This flag 
	performs bi-linear interpolation of the corner reference samples 
	for a strong smoothing effect. The purpose is to prevent blocking 
	or banding artifacts in regions with few/zero AC coefficients. 
	Default enabled

.. option:: --constrained-intra, --no-constrained-intra

	Constrained intra prediction. When generating intra predictions for
	blocks in inter slices, only intra-coded reference pixels are used.
	Inter-coded reference pixels are replaced with intra-coded neighbor
	pixels or default values. The general idea is to block the
	propagation of reference errors that may have resulted from lossy
	signals. Default disabled

Psycho-visual options
=====================

Left to its own devices, the encoder will make mode decisions based on a
simple rate distortion formula, trading distortion for bitrate. This is
generally effective except for the manner in which this distortion is
measured. It tends to favor blurred reconstructed blocks over blocks
which have wrong motion. The human eye generally prefers the wrong
motion over the blur and thus x265 offers psycho-visual adjustments to
the rate distortion algorithm.

:option:`--psy-rd` will add an extra cost to reconstructed blocks which
do not match the visual energy of the source block. The higher the
strength of :option:`--psy-rd` the more strongly it will favor similar
energy over blur and the more aggressively it will ignore rate
distortion. If it is too high, it will introduce visal artifacts and
increase bitrate enough for rate control to increase quantization
globally, reducing overall quality. psy-rd will tend to reduce the use
of blurred prediction modes, like DC and planar intra and bi-directional
inter prediction.

:option:`--psy-rdoq` will adjust the distortion cost used in
rate-distortion optimized quantization (RDO quant), enabled by
:option:`--rdoq-level` 1 or 2, favoring the preservation of energy in the
reconstructed image.  :option:`--psy-rdoq` prevents RDOQ from blurring
all of the encoding options which psy-rd has to chose from.  At low
strength levels, psy-rdoq will influence the quantization level
decisions, favoring higher AC energy in the reconstructed image. As
psy-rdoq strength is increased, more non-zero coefficient levels are
added and fewer coefficients are zeroed by RDOQ's rate distortion
analysis. High levels of psy-rdoq can double the bitrate which can have
a drastic effect on rate control, forcing higher overall QP, and can
cause ringing artifacts. psy-rdoq is less accurate than psy-rd, it is
biasing towards energy in general while psy-rd biases towards the energy
of the source image. But very large psy-rdoq values can sometimes be
beneficial, preserving film grain for instance.

As a general rule, when both psycho-visual features are disabled, the
encoder will tend to blur blocks in areas of difficult motion. Turning
on small amounts of psy-rd and psy-rdoq will improve the perceived
visual quality. Increasing psycho-visual strength further will improve
quality and begin introducing artifacts and increase bitrate, which may
force rate control to increase global QP. Finding the optimal
psycho-visual parameters for a given video requires experimentation. Our
recommended defaults (1.0 for both) are generally on the low end of the
spectrum.

The lower the bitrate, the lower the optimal psycho-visual settings. If
the bitrate is too low for the psycho-visual settings, you will begin to
see temporal artifacts (motion judder). This is caused when the encoder
is forced to code skip blocks (no residual) in areas of difficult motion
because it is the best option psycho-visually (they have great amounts
of energy and no residual cost). One can lower psy-rd settings when
judder is happening, and allow the encoder to use some blur in these
areas of high motion.

.. option:: --psy-rd <float>

	Influence rate distortion optimizated mode decision to preserve the
	energy of the source image in the encoded image at the expense of
	compression efficiency. It only has effect on presets which use
	RDO-based mode decisions (:option:`--rd` 3 and above). 1.0 is a
	typical value. Default 0.3

	**Range of values:** 0 .. 2.0

.. option:: --psy-rdoq <float>

	Influence rate distortion optimized quantization by favoring higher
	energy in the reconstructed image. This generally improves perceived
	visual quality at the cost of lower quality metric scores.  It only
	has effect when :option:`--rdoq-level` is 1 or 2. High values can
	be beneficial in preserving high-frequency detail like film grain.
	Default: 1.0

	**Range of values:** 0 .. 50.0


Slice decision options
======================

.. option:: --open-gop, --no-open-gop

	Enable open GOP, allow I-slices to be non-IDR. Default enabled

.. option:: --keyint, -I <integer>

	Max intra period in frames. A special case of infinite-gop (single
	keyframe at the beginning of the stream) can be triggered with
	argument -1. Use 1 to force all-intra. Default 250

.. option:: --min-keyint, -i <integer>

	Minimum GOP size. Scenecuts closer together than this are coded as I
	or P, not IDR. Minimum keyint is clamped to be at least half of
	:option:`--keyint`. If you wish to force regular keyframe intervals
	and disable adaptive I frame placement, you must use
	:option:`--no-scenecut`.

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

.. option:: --lookahead-slices <0..16>

	Use multiple worker threads to measure the estimated cost of each
	frame within the lookahead. When :option:`--b-adapt` is 2, most
	frame cost estimates will be performed in batch mode, many cost
	estimates at the same time, and lookahead-slices is ignored for
	batched estimates. The effect on performance can be quite small.
	The higher this parameter, the less accurate the frame costs will be
	(since context is lost across slice boundaries) which will result in
	less accurate B-frame and scene-cut decisions.

	The encoder may internally lower the number of slices to ensure
	each slice codes at least 10 16x16 rows of lowres blocks. If slices
	are used in lookahead, they are logged in the list of tools as
	*lslices*.
	
	**Values:** 0 - disabled (default). 1 is the same as 0. Max 16

.. option:: --b-adapt <integer>

	Set the level of effort in determining B frame placement.

	With b-adapt 0, the GOP structure is fixed based on the values of
	:option:`--keyint` and :option:`--bframes`.
	
	With b-adapt 1 a light lookahead is used to choose B frame placement.

	With b-adapt 2 (trellis) a viterbi B path selection is performed

	**Values:** 0:none; 1:fast; 2:full(trellis) **default**

.. option:: --bframes, -b <0..16>

	Maximum number of consecutive b-frames. Use :option:`--bframes` 0 to
	force all P/I low-latency encodes. Default 4. This parameter has a
	quadratic effect on the amount of memory allocated and the amount of
	work performed by the full trellis version of :option:`--b-adapt`
	lookahead.

.. option:: --bframe-bias <integer>

	Bias towards B frames in slicetype decision. The higher the bias the
	more likely x265 is to use B frames. Can be any value between -90
	and 100 and is clipped to that range. Default 0

.. option:: --b-pyramid, --no-b-pyramid

	Use B-frames as references, when possible. Default enabled

Quality, rate control and rate distortion options
=================================================

.. option:: --bitrate <integer>

	Enables single-pass ABR rate control. Specify the target bitrate in
	kbps. Default is 0 (CRF)

	**Range of values:** An integer greater than 0

.. option:: --crf <0..51.0>

	Quality-controlled variable bitrate. CRF is the default rate control
	method; it does not try to reach any particular bitrate target,
	instead it tries to achieve a given uniform quality and the size of
	the bitstream is determined by the complexity of the source video.
	The higher the rate factor the higher the quantization and the lower
	the quality. Default rate factor is 28.0.

.. option:: --crf-max <0..51.0>

	Specify an upper limit to the rate factor which may be assigned to
	any given frame (ensuring a max QP).  This is dangerous when CRF is
	used in combination with VBV as it may result in buffer underruns.
	Default disabled
        
.. option:: --crf-min <0..51.0>

	Specify an lower limit to the rate factor which may be assigned to
	any given frame (ensuring a min compression factor).

.. option:: --vbv-bufsize <integer>

	Specify the size of the VBV buffer (kbits). Enables VBV in ABR
	mode.  In CRF mode, :option:`--vbv-maxrate` must also be specified.
	Default 0 (vbv disabled)

.. option:: --vbv-maxrate <integer>

	Maximum local bitrate (kbits/sec). Will be used only if vbv-bufsize
	is also non-zero. Both vbv-bufsize and vbv-maxrate are required to
	enable VBV in CRF mode. Default 0 (disabled)

	Note that when VBV is enabled (with a valid :option:`--vbv-bufsize`),
	VBV emergency denoising is turned on. This will turn on aggressive 
	denoising at the frame level when frame QP > QP_MAX_SPEC (51), drastically
	reducing bitrate and allowing ratecontrol to assign lower QPs for
	the following frames. The visual effect is blurring, but removes 
	significant blocking/displacement artifacts.

.. option:: --vbv-init <float>

	Initial buffer occupancy. The portion of the decode buffer which
	must be full before the decoder will begin decoding.  Determines
	absolute maximum frame size. May be specified as a fractional value
	between 0 and 1, or in kbits. In other words these two option pairs
	are equivalent::

	--vbv-bufsize 1000 --vbv-init 900
	--vbv-bufsize 1000 --vbv-init 0.9

	Default 0.9

	**Range of values:** fractional: 0 - 1.0, or kbits: 2 .. bufsize

.. option:: --qp, -q <integer>

	Specify base quantization parameter for Constant QP rate control.
	Using this option enables Constant QP rate control. The specified QP
	is assigned to P slices. I and B slices are given QPs relative to P
	slices using param->rc.ipFactor and param->rc.pbFactor unless QP 0
	is specified, in which case QP 0 is used for all slice types.  Note
	that QP 0 does not cause lossless encoding, it only disables
	quantization. Default disabled (CRF)

	**Range of values:** an integer from 0 to 51

.. option:: --lossless, --no-lossless

	Enables true lossless coding by bypassing scaling, transform,
	quantization and in-loop filter processes. This is used for
	ultra-high bitrates with zero loss of quality. Reconstructed output
	pictures are bit-exact to the input pictures. Lossless encodes
	implicitly have no rate control, all rate control options are
	ignored. Slower presets will generally achieve better compression
	efficiency (and generate smaller bitstreams). Default disabled.

.. option:: --aq-mode <0|1|2|3>

	Adaptive Quantization operating mode. Raise or lower per-block
	quantization based on complexity analysis of the source image. The
	more complex the block, the more quantization is used. This offsets
	the tendency of the encoder to spend too many bits on complex areas
	and not enough in flat areas.

	0. disabled
	1. AQ enabled **(default)**
	2. AQ enabled with auto-variance
	3. AQ enabled with auto-variance and bias to dark scenes

.. option:: --aq-strength <float>

	Adjust the strength of the adaptive quantization offsets. Setting
	:option:`--aq-strength` to 0 disables AQ. Default 1.0.

	**Range of values:** 0.0 to 3.0

.. option:: --qg-size <64|32|16>

	Enable adaptive quantization for sub-CTUs. This parameter specifies 
	the minimum CU size at which QP can be adjusted, ie. Quantization Group
	size. Allowed range of values are 64, 32, 16 provided this falls within 
	the inclusive range [maxCUSize, minCUSize]. Experimental.
	Default: same as maxCUSize

.. option:: --cutree, --no-cutree

	Enable the use of lookahead's lowres motion vector fields to
	determine the amount of reuse of each block to tune adaptive
	quantization factors. CU blocks which are heavily reused as motion
	reference for later frames are given a lower QP (more bits) while CU
	blocks which are quickly changed and are not referenced are given
	less bits. This tends to improve detail in the backgrounds of video
	with less detail in areas of high motion. Default enabled

.. option:: --pass <integer>

	Enable multi-pass rate control mode. Input is encoded multiple times,
	storing the encoded information of each pass in a stats file from which
	the consecutive pass tunes the qp of each frame to improve the quality
	of the output. Default disabled

	1. First pass, creates stats file
	2. Last pass, does not overwrite stats file
	3. Nth pass, overwrites stats file

	**Range of values:** 1 to 3

.. option:: --stats <filename>

	Specify file name of of the multi-pass stats file. If unspecified
	the encoder will use x265_2pass.log

.. option:: --slow-firstpass, --no-slow-firstpass

	Enable a slow and more detailed first pass encode in multi-pass rate
	control mode.  Speed of the first pass encode is slightly lesser and
	quality midly improved when compared to the default settings in a
	multi-pass encode. Default disabled (turbo mode enabled)

	When **turbo** first pass is not disabled, these options are
	set on the first pass to improve performance:
	
	* :option:`--fast-intra`
	* :option:`--no-rect`
	* :option:`--no-amp`
	* :option:`--early-skip`
	* :option:`--ref` = 1
	* :option:`--max-merge` = 1
	* :option:`--me` = DIA
	* :option:`--subme` = MIN(2, :option:`--subme`)
	* :option:`--rd` = MIN(2, :option:`--rd`)

.. option:: --strict-cbr, --no-strict-cbr
	
	Enables stricter conditions to control bitrate deviance from the 
	target bitrate in ABR mode. Bit rate adherence is prioritised
	over quality. Rate tolerance is reduced to 50%. Default disabled.
	
	This option is for use-cases which require the final average bitrate 
	to be within very strict limits of the target; preventing overshoots, 
	while keeping the bit rate within 5% of the target setting, 
	especially in short segment encodes. Typically, the encoder stays 
	conservative, waiting until there is enough feedback in terms of 
	encoded frames to control QP. strict-cbr allows the encoder to be 
	more aggressive in hitting the target bitrate even for short segment 
	videos. Experimental.
	
.. option:: --cbqpoffs <integer>

	Offset of Cb chroma QP from the luma QP selected by rate control.
	This is a general way to spend more or less bits on the chroma
	channel.  Default 0

	**Range of values:** -12 to 12

.. option:: --crqpoffs <integer>

	Offset of Cr chroma QP from the luma QP selected by rate control.
	This is a general way to spend more or less bits on the chroma
	channel.  Default 0

	**Range of values:**  -12 to 12

.. option:: --ipratio <float>

	QP ratio factor between I and P slices. This ratio is used in all of
	the rate control modes. Some :option:`--tune` options may change the
	default value. It is not typically manually specified. Default 1.4

.. option:: --pbratio <float>

	QP ratio factor between P and B slices. This ratio is used in all of
	the rate control modes. Some :option:`--tune` options may change the
	default value. It is not typically manually specified. Default 1.3

.. option:: --qcomp <float>

	qComp sets the quantizer curve compression factor. It weights the
	frame quantizer based on the complexity of residual (measured by
	lookahead).  Default value is 0.6. Increasing it to 1 will
	effectively generate CQP

.. option:: --qpstep <integer>

	The maximum single adjustment in QP allowed to rate control. Default
	4

.. option:: --qblur <float>

	Temporally blur quants. Default 0.5

.. option:: --cplxblur <float>

	temporally blur complexity. default 20

.. option:: --zones <zone0>/<zone1>/...

	Tweak the bitrate of regions of the video. Each zone takes the form:

	<start frame>,<end frame>,<option> where <option> is either q=<integer>
	(force QP) or b=<float> (bitrate multiplier).

	If zones overlap, whichever comes later in the list takes precedence.
	Default none

Quantization Options
====================

Note that rate-distortion optimized quantization (RDOQ) is enabled
implicitly at :option:`--rd` 4, 5, and 6 and disabled implicitly at all
other levels.
 
.. option:: --signhide, --no-signhide

	Hide sign bit of one coeff per TU (rdo). The last sign is implied.
	This requires analyzing all the coefficients to determine if a sign
	must be toggled, and then to determine which one can be toggled with
	the least amount of distortion. Default enabled

.. option:: --qpfile <filename>

	Specify a text file which contains frametypes and QPs for some or
	all frames. The format of each line is:

	framenumber frametype QP

	Frametype can be one of [I,i,P,B,b]. **B** is a referenced B frame,
	**b** is an unreferenced B frame.  **I** is a keyframe (random
	access point) while **i** is a I frame that is not a keyframe
	(references are not broken).

	Specifying QP (integer) is optional, and if specified they are
	clamped within the encoder to qpmin/qpmax.

.. option:: --scaling-list <filename>

	Quantization scaling lists. HEVC supports 6 quantization scaling
	lists to be defined; one each for Y, Cb, Cr for intra prediction and
	one each for inter prediction.

	x265 does not use scaling lists by default, but this can also be
	made explicit by :option:`--scaling-list` *off*.

	HEVC specifies a default set of scaling lists which may be enabled
	without requiring them to be signaled in the SPS. Those scaling
	lists can be enabled via :option:`--scaling-list` *default*.
    
	All other strings indicate a filename containing custom scaling
	lists in the HM format. The encode will abort if the file is not
	parsed correctly. Custom lists must be signaled in the SPS

.. option:: --lambda-file <filename>

	Specify a text file containing values for x265_lambda_tab and
	x265_lambda2_tab. Each table requires MAX_MAX_QP+1 (70) float
	values.
	
	The text file syntax is simple. Comma is considered to be
	white-space. All white-space is ignored. Lines must be less than 2k
	bytes in length. Content following hash (#) characters are ignored.
	The values read from the file are logged at :option:`--log-level`
	debug.

	Note that the lambda tables are process-global and so the new values
	affect all encoders running in the same process. 
	
	Lambda values affect encoder mode decisions, the lower the lambda
	the more bits it will try to spend on signaling information (motion
	vectors and splits) and less on residual. This feature is intended
	for experimentation.

Loop filters
============

.. option:: --deblock=<int>:<int>, --no-deblock

	Toggle deblocking loop filter, optionally specify deblocking
	strength offsets.

	<int>:<int> - parsed as tC offset and Beta offset
	<int>,<int> - parsed as tC offset and Beta offset
	<int>       - both tC and Beta offsets assigned the same value

	If unspecified, the offsets default to 0. The offsets must be in a
	range of -6 (lowest strength) to 6 (highest strength).

	To disable the deblocking filter entirely, use --no-deblock or
	--deblock=false. Default enabled, with both offsets defaulting to 0

	If deblocking is disabled, or the offsets are non-zero, these
	changes from the default configuration are signaled in the PPS.

.. option:: --sao, --no-sao

	Toggle Sample Adaptive Offset loop filter, default enabled

.. option:: --sao-non-deblock, --no-sao-non-deblock

	Specify how to handle depencency between SAO and deblocking filter.
	When enabled, non-deblocked pixels are used for SAO analysis. When
	disabled, SAO analysis skips the right/bottom boundary areas.
	Default disabled

VUI (Video Usability Information) options
=========================================

x265 emits a VUI with only the timing info by default. If the SAR is
specified (or read from a Y4M header) it is also included.  All other
VUI fields must be manually specified.

.. option:: --sar <integer|w:h>

	Sample Aspect Ratio, the ratio of width to height of an individual
	sample (pixel). The user may supply the width and height explicitly
	or specify an integer from the predefined list of aspect ratios
	defined in the HEVC specification.  Default undefined (not signaled)

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

.. option:: --display-window <left,top,right,bottom>

	Define the (overscan) region of the image that does not contain
	information because it was added to achieve certain resolution or
	aspect ratio (the areas are typically black bars). The decoder may
	be directed to crop away this region before displaying the images
	via the :option:`--overscan` option.  Default undefined (not
	signaled).

	Note that this has nothing to do with padding added internally by
	the encoder to ensure the pictures size is a multiple of the minimum
	coding unit (4x4). That padding is signaled in a separate
	"conformance window" and is not user-configurable.

.. option:: --overscan <show|crop>

	Specify whether it is appropriate for the decoder to display or crop
	the overscan area. Default unspecified (not signaled)

.. option:: --videoformat <integer|string>

	Specify the source format of the original analog video prior to
	digitizing and encoding. Default undefined (not signaled)

	0. component
	1. pal
	2. ntsc
	3. secam
	4. mac
	5. undefined

.. option:: --range <full|limited>

	Specify output range of black level and range of luma and chroma
	signals. Default undefined (not signaled)

.. option:: --colorprim <integer|string>

	Specify color primitive to use when converting to RGB. Default
	undefined (not signaled)

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

	Specify transfer characteristics. Default undefined (not signaled)

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
	16. smpte-st-2084
	17. smpte-st-428
	18. arib-std-b67

.. option:: --colormatrix <integer|string>

	Specify color matrix setting i.e set the matrix coefficients used in
	deriving the luma and chroma. Default undefined (not signaled)

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

.. option:: --chromaloc <0..5>

	Specify chroma sample location for 4:2:0 inputs. Consult the HEVC
	specification for a description of these values. Default undefined
	(not signaled)

.. option:: --master-display <string>

	SMPTE ST 2086 mastering display color volume SEI info, specified as
	a string which is parsed when the stream header SEI are emitted. The
	string format is "G(%hu,%hu)B(%hu,%hu)R(%hu,%hu)WP(%hu,%hu)L(%u,%u)"
	where %hu are unsigned 16bit integers and %u are unsigned 32bit
	integers. The SEI includes X,Y display primaries for RGB channels,
	white point X,Y and max,min luminance values. (HDR)

	Example for D65P3 1000-nits:

		G(13200,34500)B(7500,3000)R(34000,16000)WP(15635,16450)L(10000000,1)

	Note that this string value will need to be escaped or quoted to
	protect against shell expansion on many platforms. No default.

.. option:: --max-cll <string>

	Maximum content light level and maximum frame average light level as
	required by the Consumer Electronics Association 861.3 specification.

	Specified as a string which is parsed when the stream header SEI are
	emitted. The string format is "%hu,%hu" where %hu are unsigned 16bit
	integers. The first value is the max content light level (or 0 if no
	maximum is indicated), the second value is the maximum picture
	average light level (or 0). (HDR)

	Note that this string value will need to be escaped or quoted to
	protect against shell expansion on many platforms. No default.

.. option:: --min-luma <integer>

	Minimum luma value allowed for input pictures. Any values below min-luma
	are clipped. Experimental. No default.

.. option:: --max-luma <integer>

	Maximum luma value allowed for input pictures. Any values above max-luma
	are clipped. Experimental. No default.

Bitstream options
=================

.. option:: --annexb, --no-annexb

	If enabled, x265 will produce Annex B bitstream format, which places
	start codes before NAL. If disabled, x265 will produce file format,
	which places length before NAL. x265 CLI will choose the right option
	based on output format. Default enabled

	**API ONLY**

.. option:: --repeat-headers, --no-repeat-headers

	If enabled, x265 will emit VPS, SPS, and PPS headers with every
	keyframe. This is intended for use when you do not have a container
	to keep the stream headers for you and you want keyframes to be
	random access points. Default disabled

.. option:: --aud, --no-aud

	Emit an access unit delimiter NAL at the start of each slice access
	unit. If :option:`--repeat-headers` is not enabled (indicating the
	user will be writing headers manually at the start of the stream)
	the very first AUD will be skipped since it cannot be placed at the
	start of the access unit, where it belongs. Default disabled

.. option:: --hrd, --no-hrd

	Enable the signalling of HRD parameters to the decoder. The HRD
	parameters are carried by the Buffering Period SEI messages and
	Picture Timing SEI messages providing timing information to the
	decoder. Default disabled

.. option:: --info, --no-info

	Emit an informational SEI with the stream headers which describes
	the encoder version, build info, and encode parameters. This is very
	helpful for debugging purposes but encoding version numbers and
	build info could make your bitstreams diverge and interfere with
	regression testing. Default enabled

.. option:: --hash <integer>

	Emit decoded picture hash SEI, so the decoder may validate the
	reconstructed pictures and detect data loss. Also useful as a
	debug feature to validate the encoder state. Default None

	1. MD5
	2. CRC
	3. Checksum

.. option:: --temporal-layers,--no-temporal-layers

	Enable a temporal sub layer. All referenced I/P/B frames are in the
	base layer and all unreferenced B frames are placed in a temporal
	enhancement layer. A decoder may chose to drop the enhancement layer 
	and only decode and display the base layer slices.
	
	If used with a fixed GOP (:option:`b-adapt` 0) and :option:`bframes`
	3 then the two layers evenly split the frame rate, with a cadence of
	PbBbP. You probably also want :option:`--no-scenecut` and a keyframe
	interval that is a multiple of 4.

Debugging options
=================

.. option:: --recon, -r <filename>

	Output file containing reconstructed images in display order. If the
	file extension is ".y4m" the file will contain a YUV4MPEG2 stream
	header and frame headers. Otherwise it will be a raw YUV file in the
	encoder's internal bit depth.

	**CLI ONLY**

.. option:: --recon-depth <integer>

	Bit-depth of output file. This value defaults to the internal bit
	depth and currently cannot to be modified.

	**CLI ONLY**

.. option:: --recon-y4m-exec <string>

	If you have an application which can play a Y4MPEG stream received
	on stdin, the x265 CLI can feed it reconstructed pictures in display
	order.  The pictures will have no timing info, obviously, so the
	picture timing will be determined primarily by encoding elapsed time
	and latencies, but it can be useful to preview the pictures being
	output by the encoder to validate input settings and rate control
	parameters.

	Example command for ffplay (assuming it is in your PATH):

	--recon-y4m-exec "ffplay -i pipe:0 -autoexit"

	**CLI ONLY**

.. vim: noet
