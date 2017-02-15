*************
Release Notes
*************

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
