*************
Release Notes
*************

Version 2.9
===========

Release date - 05/10/2018

New features
-------------
1. Support for chunked encoding

   :option:`--chunk-start and --chunk-end` 
   Frames preceding first frame of chunk in display order will be encoded, however, they will be discarded in the bitstream.
   Frames following last frame of the chunk in display order will be used in taking lookahead decisions, but, they will not be encoded. 
   This feature can be enabled only in closed GOP structures. Default disabled.

2. Support for HDR10+ version 1 SEI messages.

Encoder enhancements
--------------------
1. Create API function for allocating and freeing x265_analysis_data.
2. CEA 608/708 support: Read SEI messages from text file and encode it using userSEI message.

Bug fixes
---------
1. Disable noise reduction when vbv is enabled.
2. Support minLuma and maxLuma values changed by the commandline.

Version 2.8
===========

Release date - 21/05/2018

New features
-------------
1. :option:`--asm avx512` used to enable AVX-512 in x265. Default disabled.	
    For 4K main10 high-quality encoding, we are seeing good gains; for other resolutions and presets, we don't recommend using this setting for now.

2. :option:`--dynamic-refine` dynamically switches between different inter refine levels. Default disabled.
    It is recommended to use :option:`--refine-intra 4' with dynamic refinement for a better trade-off between encode efficiency and performance than using static refinement.

3. :option:`--single-sei`
    Encode SEI messages in a single NAL unit instead of multiple NAL units. Default disabled. 

4. :option:`--max-ausize-factor` controls the maximum AU size defined in HEVC specification.
    It represents the percentage of maximum AU size used. Default is 1. 
	  
5. VMAF (Video Multi-Method Assessment Fusion)
   Added VMAF support for objective quality measurement of a video sequence. 
   Enable cmake option ENABLE_LIBVMAF to report per frame and aggregate VMAF score. The frame level VMAF score does not include temporal scores.
   This is supported only on linux for now.
 
Encoder enhancements
--------------------
1. Introduced refine-intra level 4 to improve quality. 
2. Support for HLG-graded content and pic_struct in SEI message.

Bug Fixes
---------
1. Fix 32 bit build error (using CMAKE GUI) in Linux.
2. Fix 32 bit build error for asm primitives.
3. Fix build error on mac OS.
4. Fix VBV Lookahead in analysis load to achieve target bitrate.


Version 2.7
===========

Release date - 21st Feb, 2018.

New features
------------
1. :option:`--gop-lookahead` can be used to extend the gop boundary(set by `--keyint`). The GOP will be extended, if a scene-cut frame is found within this many number of frames. 
2. Support for RADL pictures added in x265.
   :option:`--radl` can be used to decide number of RADL pictures preceding the IDR picture.

Encoder enhancements
--------------------
1. Moved from YASM to NASM assembler. Supports NASM assembler version 2.13 and greater.
2. Enable analysis save and load in a single run. Introduces two new cli options `--analysis-save <filename>` and `--analysis-load <filename>`.
3. Comply to HDR10+ LLC specification.
4. Reduced x265 build time by more than 50% by re-factoring ipfilter.asm.  

Bug fixes
---------
1. Fixed inconsistent output issue in deblock filter and --const-vbv.
2. Fixed Mac OS build warnings.
3. Fixed inconsistency in pass-2 when weightp and cutree are enabled.
4. Fixed deadlock issue due to dropping of BREF frames, while forcing slice types through qp file.


Version 2.6
===========

Release date - 29th November, 2017.

New features
------------
1. x265 can now refine analysis from a previous HEVC encode (using options :option:`--refine-inter`, and :option:`--refine-intra`), or a previous AVC encode (using option :option:`--refine-mv-type`). The previous encode's information can be packaged using the *x265_analysis_data_t*  data field available in the *x265_picture* object.
2. Basic support for segmented (or chunked) encoding added with :option:`--vbv-end` that can specify the status of CPB at the end of a segment. String this together with :option:`--vbv-init` to encode a title as chunks while maintaining VBV compliance!
3. :option:`--force-flush` can be used to trigger a premature flush of the encoder. This option is beneficial when input is known to be bursty, and may be at a rate slower than the encoder.
4. Experimental feature :option:`--lowpass-dct` that uses truncated DCT for transformation.

Encoder enhancements
--------------------
1. Slice-parallel mode gets a significant boost in performance, particularly in low-latency mode.
2. x265 now officially supported on VS2017.
3. x265 now supports all depths from mono0 to mono16 for Y4M format.

API changes
-----------
1. Options that modified PPS dynamically (:option:`--opt-qp-pps` and :option:`--opt-ref-list-length-pps`) are now disabled by default to enable users to save bits by not sending headers. If these options are enabled, headers have to be repeated for every GOP.
2. Rate-control and analysis parameters can dynamically be reconfigured simultaneously via the *x265_encoder_reconfig* API.
3. New API functions to extract intermediate information such as slice-type, scenecut information, reference frames, etc. are now available. This information may be beneficial to integrating applications that are attempting to perform content-adaptive encoding. Refer to documentation on *x265_get_slicetype_poc_and_scenecut*, and *x265_get_ref_frame_list* for more details and suggested usage.
4. A new API to pass supplemental CTU information to x265 to influence analysis decisions has been added. Refer to documentation on *x265_encoder_ctu_info* for more details.

Bug fixes
---------
1. Bug fixes when :option:`--slices` is used with VBV settings.
2. Minor memory leak fixed for HDR10+ builds, and default x265 when pools option is specified.
3. HDR10+ bug fix to remove dependence on poc counter to select meta-data information.

Version 2.5
===========

Release date - 13th July, 2017.

Encoder enhancements
--------------------
1. Improved grain handling with :option:`--tune` grain option by throttling VBV operations to limit QP jumps.
2. Frame threads are now decided based on number of threads specified in the :option:`--pools`, as opposed to the number of hardware threads available. The mapping was also adjusted to improve quality of the encodes with minimal impact to performance.
3. CSV logging feature (enabled by :option:`--csv`) is now part of the library; it was previously part of the x265 application. Applications that integrate libx265 can now extract frame level statistics for their encodes by exercising this option in the library.
4.  Globals that track min and max CU sizes, number of slices, and other parameters have now been moved into instance-specific variables. Consequently, applications that invoke multiple instances of x265 library are no longer restricted to use the same settings for these parameter options across the multiple instances.
5. x265 can now generate a seprate library that exports the HDR10+ parsing API. Other libraries that wish to use this API may do so by linking against this library. Enable ENABLE_HDR10_PLUS in CMake options and build to generate this library.
6. SEA motion search receives a 10% performance boost from AVX2 optimization of its kernels.
7. The CSV log is now more elaborate with additional fields such as PU statistics, average-min-max luma and chroma values, etc. Refer to documentation of :option:`--csv` for details of all fields.
8. x86inc.asm cleaned-up for improved instruction handling.

API changes
-----------
1. New API x265_encoder_ctu_info() introduced to specify suggested partition sizes for various CTUs in a frame. To be used in conjunction with :option:`--ctu-info` to react to the specified partitions appropriately.
2. Rate-control statistics passed through the x265_picture object for an incoming frame are now used by the encoder.
3. Options to scale, reuse, and refine analysis for incoming analysis shared through the x265_analysis_data field in x265_picture for runs that use :option:`--analysis-reuse-mode` load; use options :option:`--scale`, :option:`--refine-mv`, :option:`--refine-inter`, and :option:`--refine-intra` to explore. 
4. VBV now has a deterministic mode. Use :option:`--const-vbv` to exercise.

Bug fixes
---------
1. Several fixes for HDR10+ parsing code including incompatibility with user-specific SEI, removal of warnings, linking issues in linux, etc.
2. SEI messages for HDR10 repeated every keyint when HDR options (:option:`--hdr-opt`, :option:`--master-display`) specified.

Version 2.4
===========

Release date - 22nd April, 2017.

Encoder enhancements
--------------------
1. HDR10+ supported. Dynamic metadata may be either supplied as a bitstream via the userSEI field of x265_picture, or as a json jile that can be parsed by x265 and inserted into the bitstream; use :option:`--dhdr10-info` to specify json file name, and :option:`--dhdr10-opt` to enable optimization of inserting tone-map information only at IDR frames, or when the tone map information changes.
2. Lambda tables for 8, 10, and 12-bit encoding revised, resulting in significant enhancement to subjective  visual quality.
3. Enhanced HDR10 encoding with HDR-specific QP optimzations for chroma, and luma planes of WCG content enabled; use :option:`--hdr-opt` to activate.
4. Ability to accept analysis information from other previous encodes (that may or may not be x265), and selectively reuse and refine analysis for encoding subsequent passes enabled with the :option:`--refine-level` option. 
5. Slow and veryslow presets receive a 20% speed boost at iso-quality by enabling the :option:`--limit-tu` option.
6. The bitrate target for x265 can now be dynamically reconfigured via the reconfigure API.
7. Performance optimized SAO algorithm introduced via the :option:`--limit-sao` option; seeing 10% speed benefits at faster presets.

API changes
-----------
1. x265_reconfigure API now also accepts rate-control parameters for dynamic reconfiguration.
2. Several additions to data fields in x265_analysis to support :option:`--refine-level`: see x265.h for more details.

Bug fixes
---------
1. Avoid negative offsets in x265 lambda2 table with SAO enabled.
2. Fix mingw32 build error.
3. Seek now enabled for pipe input, in addition to file-based input
4. Fix issue of statically linking core-utils not working in linux.
5. Fix visual artifacts with :option:`--multi-pass-opt-distortion` with VBV.
6. Fix bufferFill stats reported in csv.

Version 2.3
===========

Release date - 15th February, 2017.

Encoder enhancements
--------------------
1. New SSIM-based RD-cost computation for improved visual quality, and efficiency; use :option:`--ssim-rd` to exercise.
2. Multi-pass encoding can now share analysis information from prior passes (in addition to rate-control information) to improve performance and quality of subsequent passes; to your multi-pass command-lines that use the :option:`--pass` option, add :option:`--multi-pass-opt-distortion` to share distortion information, and :option:`--multi-pass-opt-analysis` to share other analysis information.
3. A dedicated thread pool for lookahead can now be specified with :option:`--lookahead-threads`.
4. option:`--dynamic-rd` dynamically increase analysis in areas where the bitrate is being capped by VBV; works for both CRF and ABR encodes with VBV settings.
5. The number of bits used to signal the delta-QP can be optimized with the :option:`--opt-cu-delta-qp` option; found to be useful in some scenarios for lower bitrate targets.
6. Experimental feature option:`--aq-motion` adds new QP offsets based on relative motion of a block with respect to the movement of the frame.

API changes
-----------
1. Reconfigure API now supports signalling new scaling lists.
2. x265 application's csv functionality now reports time (in milliseconds) taken to encode each frame.
3. :option:`--strict-cbr` enables stricter bitrate adherence by adding filler bits when achieved bitrate is lower than the target; earlier, it was only reacting when the achieved rate was higher.
4. :option:`--hdr` can be used to ensure that max-cll and max-fall values are always signaled (even if 0,0).

Bug fixes
---------
1. Fixed incorrect HW thread counting on MacOS platform.
2. Fixed scaling lists support for 4:4:4 videos.
3. Inconsistent output fix for :option:`--opt-qp-pss` by removing last slice's QP from cost calculation.
4. VTune profiling (enabled using ENABLE_VTUNE CMake option) now also works with 2017 VTune builds.

Version 2.2
===========

Release date - 26th December, 2016.

Encoder enhancements
--------------------
1. Enhancements to TU selection algorithm with early-outs for improved speed; use :option:`--limit-tu` to exercise.
2. New motion search method SEA (Successive Elimination Algorithm) supported now as :option: `--me` 4
3. Bit-stream optimizations to improve fields in PPS and SPS for bit-rate savings through :option:`--opt-qp-pps`, :option:`--opt-ref-list-length-pps`, and :option:`--multi-pass-opt-rps`.
4. Enabled using VBV constraints when encoding without WPP.
5. All param options dumped in SEI packet in bitstream when info selected.
6. x265 now supports POWERPC-based systems. Several key functions also have optimized ALTIVEC kernels.

API changes
-----------
1. Options to disable SEI and optional-VUI messages from bitstream made more descriptive.
2. New option :option:`--scenecut-bias` to enable controlling bias to mark scene-cuts via cli.
3. Support mono and mono16 color spaces for y4m input.
4. :option:`--min-cu-size` of 64 no-longer supported for reasons of visual quality (was crashing earlier anyways.)
5. API for CSV now expects version string for better integration of x265 into other applications.

Bug fixes
---------
1. Several fixes to slice-based encoding.
2. :option:`--log2-max-poc-lsb`'s range limited according to HEVC spec.
3. Restrict MVs to within legal boundaries when encoding.

Version 2.1
===========

Release date - 27th September, 2016

Encoder enhancements
--------------------
1. Support for qg-size of 8
2. Support for inserting non-IDR I-frames at scenecuts and when running with settings for fixed-GOP (min-keyint = max-keyint)
3. Experimental support for slice-parallelism.

API changes
-----------
1. Encode user-define SEI messages passed in through x265_picture object.
2. Disable SEI and VUI messages from the bitstream
3. Specify qpmin and qpmax
4. Control number of bits to encode POC.

Bug fixes
---------
1. QP fluctuation fix for first B-frame in mini-GOP for 2-pass encoding with tune-grain.
2. Assembly fix for crashes in 32-bit from dct_sse4.
3. Threadpool creation fix in windows platform.

Version 2.0
===========

Release date - 13th July, 2016

New Features
------------

1. uhd-bd: Enable Ultra-HD Bluray support
2. rskip: Enables skipping recursion to analyze lower CU sizes using heuristics at different rd-levels. Provides good visual quality gains at the highest quality presets. 
3. rc-grain: Enables a new ratecontrol mode specifically for grainy content. Strictly prevents QP oscillations within and between frames to avoid grain fluctuations.
4. tune grain: A fully refactored and improved option to encode film grain content including QP control as well as analysis options.
5. asm: ARM assembly is now enabled by default, native or cross compiled builds supported on armv6 and later systems.

API and Key Behaviour Changes
-----------------------------

1. x265_rc_stats added to x265_picture, containing all RC decision points for that frame
2. PTL: high tier is now allowed by default, chosen only if necessary
3. multi-pass: First pass now uses slow-firstpass by default, enabling better RC decisions in future passes 
4. pools: fix behaviour on multi-socketed Windows systems, provide more flexibility in determining thread and pool counts
5. ABR: improve bits allocation in the first few frames, abr reset, vbv and cutree improved

Misc
----
1. An SSIM calculation bug was corrected

Version 1.9
===========

Release date - 29th January, 2016

New Features
------------

1. Quant offsets: This feature allows block level quantization offsets to be specified for every frame. An API-only feature.
2. --intra-refresh: Keyframes can be replaced by a moving column of intra blocks in non-keyframes.
3. --limit-modes: Intelligently restricts mode analysis. 
4. --max-luma and --min-luma for luma clipping, optional for HDR use-cases
5. Emergency denoising is now enabled by default in very low bitrate, VBV encodes

API Changes
-----------

1. x265_frame_stats returns many additional fields: maxCLL, maxFALL, residual energy, scenecut  and latency logging
2. --qpfile now supports frametype 'K"
3. x265 now allows CRF ratecontrol in pass N (N greater than or equal to 2)
4. Chroma subsampling format YUV 4:0:0 is now fully supported and tested

Presets and Performance
-----------------------

1. Recently added features lookahead-slices, limit-modes, limit-refs have been enabled by default for applicable presets.
2. The default psy-rd strength has been increased to 2.0
3. Multi-socket machines now use a single pool of threads that can work cross-socket.

Version 1.8
===========

Release date - 10th August, 2015

API Changes
-----------
1. Experimental support for Main12 is now enabled. Partial assembly support exists. 
2. Main12 and Intra/Still picture profiles are now supported. Still picture profile is detected based on x265_param::totalFrames.
3. Three classes of encoding statistics are now available through the API. 
a) x265_stats - contains encoding statistics, available through x265_encoder_get_stats()
b) x265_frame_stats and x265_cu_stats - contains frame encoding statistics, available through recon x265_picture
4. --csv
a) x265_encoder_log() is now deprecated
b) x265_param::csvfn is also deprecated
5. --log-level now controls only console logging, frame level console logging has been removed.
6. Support added for new color transfer characteristic ARIB STD-B67

New Features
------------
1. limit-refs: This feature limits the references analysed for individual CUS. Provides a nice tradeoff between efficiency and performance.
2. aq-mode 3: A new aq-mode that provides additional biasing for low-light conditions.
3. An improved scene cut detection logic that allows ratecontrol to manage visual quality at fade-ins and fade-outs better.

Preset and Tune Options
-----------------------

1. tune grain: Increases psyRdoq strength to 10.0, and rdoq-level to 2.
2. qg-size: Default value changed to 32.
