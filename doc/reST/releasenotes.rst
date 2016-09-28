*************
Release Notes
*************

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

Release date - 10th August, 2016

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
