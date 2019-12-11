SVT-HEVC
--------

.. _SvtHevc:

x265 has support for open source HEVC encoder `SVT-HEVC <https://01.org/svt>`_ 
and can generate SVT-HEVC compliant bitstreams. SVT-HEVC encoder can be enabled at run time 
using :option:`--svt`. Since SVT-HEVC params/CLI are not exposed outside, it has to be 
configured only via x265 CLI options. The API's of SVT-HEVC are accessed through x265's API 
so even library users of x265 can avail this feature, under the condition that x265_param_parse() 
API should be used for all param assignment. Params assigned outside of x265_param_parse() API 
wont't be mapped to SVT-HEVC. This document describes the steps needed to compile x265 
with SVT-HEVC and CLI options mapping between x265 and SVT-HEVC.

Supported Version
=================
Version - 1.4.1

Build Steps
===========
This section describes the build steps to be followed to link SVT-HEVC with x265.

**SVT-HEVC**

1. Clone `SVT-HEVC <https://github.com/intel/SVT-HEVC>`_ (say at path "/home/app/") and build it (follow the build steps in its README file)
2. Once build is successful, binaries can be found inside the *Bin* folder at its root directory ("/home/app/SVT-HEVC/Bin/Release/")

**x265**

1. Set environmental variables SVT_HEVC_INCLUDE_DIR and SVT_HEVC_LIBRARY_DIR to help x265 locate SVT-HEVC. For example:
	* export SVT_HEVC_INCLUDE_DIR = /home/app/SVT-HEVC/Source/API/
	* export SVT_HEVC_LIBRARY_DIR = /home/app/SVT-HEVC/Bin/Release/
2. Enable the cmake option ENABLE_SVT_HEVC and continue the normal build process

CLI options Mapping
===================
Once x265 is compiled with SVT-HEVC, SVT-HEVC encoder can be invoked at run time using 
:option:`--svt`. :option:`--svt` can be added anywhere in the command line, x265 application will automatically
parse it first and maps all other x265 CLI's present in the command line to SVT-HEVC encoder options 
internally and configures the same. Below table shows the actual mapping of x265 CLI options to  
SVT-HEVC encoder options (Name as shown in SVT-HEVC's sample configuration file)

+-------------------------------------------+------------------------------+------------------------------+
| x265 CLI option                           | SVT-HEVC Encoder Option      | Range                        |
+===========================================+==============================+==============================+
| :option:`--input`                         | InputFile                    | Any String                   |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--output`                        | StreamFile                   | Any String                   |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--input-depth`                   | EncoderBitDepth              | [8, 10]                      |
+-------------------------------------------+------------------------------+------------------------------+
|                                           | SourceWidth                  | [64 - 8192]                  |
| :option:`--input-res`                     +------------------------------+------------------------------+
|                                           | SourceHeight                 | [64 - 8192]                  |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--frames`                        | FrameToBeEncoded             | Any Positive Integer         |
+-------------------------------------------+------------------------------+------------------------------+
|                                           | FrameRateNumerator           | Any Positive Integer         |
| :option:`--fps`                           +------------------------------+------------------------------+
|                                           | FrameRateDenominator         | Any Positive Integer         |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--profile`                       | Profile                      | [main, main10]               |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--level-idc`                     | Level                        | [1, 2, 2.1, 3, 3.1, 4, 4.1,  |
|                                           |                              |  5, 5.1, 5.2, 6, 6.1, 6.2]   |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--high-tier`                     | Tier                         |                              |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--qpmin`                         | MinQpAllowed                 | [0 - 50]                     |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--qpmax`                         | MaxQpAllowed                 | [0 - 51]                     |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--rc-lookahead`                  | LookAheadDistance            | [0 - 250]                    |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--scenecut`                      | SceneChangeDetection         | Any Positive Integer         |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--open-gop`                      | IntraRefreshType             |                              |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--deblock`                       | LoopFilterDisable            | Any Integer                  |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--sao`                           | SAO                          |                              |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--keyint`                        | IntraPeriod                  | [(-2) - 255]                 |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--constrained-intra`             | ConstrainedIntra             |                              |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--vui-timing-info`               | VideoUsabilityInfo           |                              |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--hdr`                           | HighDynamicRangeInput        |                              |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--aud`                           | AccessUnitDelimeter          |                              |
+-------------------------------------------+------------------------------+------------------------------+
|                                           | RateControlMode              | RateControlMode = 0          |
| :option:`--qp`                            +------------------------------+------------------------------+
|                                           | QP                           | [0 - 51]                     |
+-------------------------------------------+------------------------------+------------------------------+
|                                           | RateControlMode              | RateControlMode = 1          |
| :option:`--bitrate`                       +------------------------------+------------------------------+
|                                           | TargetBitrate                | Any Positive Integer         |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--interlace`                     | InterlacedVideo              | [0 - 2]                      |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--pools`                         | TargetSocket,                | Maximum NUMA Nodes = 2       |
|                                           | LogicalProcessors            |                              |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--master-display`                | MasteringDisplayColorVolume  | Any String                   |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--max-cll`                       | maxCLL, maxFALL              | Any Positve Integer          |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--dolby-vision-profile`          | DolbyVisionProfile           | [8.1]                        |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--dolby-vision-rpu`              | DolbyVisionRpuFile           | Any String                   |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--nalu-file`                     | NaluFile                     | Any String                   |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--hrd`                           | hrdFlag                      | [0, 1]                       |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--vbv-maxrate`                   | vbvMaxrate                   | Any Positive Integer         |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--vbv-bufsize`                   | vbvBufsize                   | Any Positive Integer         |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--vbv-init`                      | VbvBufInit                   | [0 - 100]                    |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--frame-threads`                 | ThreadCount                  | Any Number                   |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--svt-search-width`              | SearchAreaWidth              | [1 - 256]                    |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--svt-search-height`             | SearchAreaHeight             | [1 - 256]                    |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--svt-hierarchical-level`        | HierarchicalLevels           | [0 - 3]                      |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--svt-base-layer-switch-mode`    | BaseLayerSwitchMode          | [0, 1]                       |
|                                           |                              |                              |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--svt-pred-struct`               | PredStructure                | [0 - 2]                      |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--svt-hme`                       | HME, UseDefaultMeHme         |                              |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--svt-compressed-ten-bit-format` | CompressedTenBitFormat       |                              |
|                                           |                              |                              |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--svt-speed-control`             | SpeedControlFlag             |                              |
+-------------------------------------------+------------------------------+------------------------------+
| :option:`--svt-fps-in-vps`                | FpsInVps                     |                              |
+-------------------------------------------+------------------------------+------------------------------+

x265 CLI options which are not present in the above table will have no effect if SVT-HEVC is enabled 
and would be ignored silently with a warning. If SVT-HEVC is enabled, accepted input range of x265 CLI 
options will change, because it follows SVT-HEVC encoder's specs, which are mentioned in the Range 
section in the above table. Options starting with prefix "--svt-" are newly added to 
fecilitate access to the features of SVT-HEVC which couldn't be mapped to the existing x265 CLI's. 
So these options will have effect only if SVT-HEVC is enabled and would be ignored with default x265 encode.

Preset Option Mapping
=============================
x265 has 10 presets from ultrafast to placebo whereas SVT-HEVC has 12 presets. Use :option:`--svt-preset-tuner` 
with Placebo preset to access the additional 2 presets of SVT-HEVC. Note that :option:`--svt-preset-tuner` should be 
used only if SVT-HEVC is enabled and only with Placebo preset, would be ignored otherwise. 
Below table shows the actual mapping of presets,

+----------------------------------------+------------------------------+
| x265 Preset                            | SVT-HEVC Preset              |
+========================================+==============================+
| Ultrafast                              | 11                           |
+----------------------------------------+------------------------------+
| Superfast                              | 10                           |
+----------------------------------------+------------------------------+
| Veryfast                               | 9                            |
+----------------------------------------+------------------------------+
| Faster                                 | 8                            |
+----------------------------------------+------------------------------+
| Fast                                   | 7                            |
+----------------------------------------+------------------------------+
| Medium                                 | 6                            |
+----------------------------------------+------------------------------+
| Slow                                   | 5                            |
+----------------------------------------+------------------------------+
| Slower                                 | 4                            |
+----------------------------------------+------------------------------+
| Veryslow                               | 3                            |
+----------------------------------------+------------------------------+
| Placebo                                | 2                            |
+----------------------------------------+------------------------------+
| Placebo :option:`--svt-preset-tuner` 0 | 0                            |
+----------------------------------------+------------------------------+
| Placebo :option:`--svt-preset-tuner` 1 | 1                            |
+----------------------------------------+------------------------------+
