Preset Options
--------------

.. _presets:

Presets
=======

x265 has ten predefined :option:`--preset` options that optimize the
trade-off between encoding speed (encoded frames per second) and
compression efficiency (quality per bit in the bitstream).  The default
preset is medium.  It does a reasonably good job of finding the best
possible quality without spending excessive CPU cycles looking for the
absolute most efficient way to achieve that quality.  When you use 
faster presets, the encoder takes shortcuts to improve performance at 
the expense of quality and compression efficiency.  When you use slower
presets, x265 tests more encoding options, using more computations to  
achieve the best quality at your selected bit rate (or in the case of
--crf rate control, the lowest bit rate at the selected quality).

The presets adjust encoder parameters as shown in the following table.
Any parameters below that are specified in your command-line will be 
changed from the value specified by the preset.
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

+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| preset          |  0  |  1  |  2  |   3 |   4 |   5 |   6  |   7  |   8  |  9   |
+=================+=====+=====+=====+=====+=====+=====+======+======+======+======+
| ctu             | 32  | 32  | 64  |  64 |  64 |  64 |  64  |  64  |  64  | 64   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| min-cu-size     | 16  |  8  |  8  |   8 |   8 |   8 |   8  |   8  |   8  |  8   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| bframes         |  3  |  3  |  4  |   4 |   4 |   4 |   4  |   8  |   8  |  8   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| b-adapt         |  0  |  0  |  0  |   0 |   0 |   2 |   2  |   2  |   2  |  2   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| rc-lookahead    |  5  | 10  | 15  |  15 |  15 |  20 |  25  |  30  |  40  | 60   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| lookahead-slices|  8  |  8  |  8  |   8 |   8 |   8 |   4  |   4  |   1  |  1   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| scenecut        |  0  | 40  | 40  |  40 |  40 |  40 |  40  |  40  |  40  | 40   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| ref             |  1  |  1  |  2  |   2 |   3 |   3 |   4  |   4  |   5  |  5   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| limit-refs      |  0  |  0  |  3  |   3 |   3 |   3 |   3  |   2  |   1  |  0   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| me              | dia | hex | hex | hex | hex | hex | star | star | star | star |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| merange         | 57  | 57  | 57  |  57 |  57 |  57 |  57  |  57  |  57  | 92   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| subme           |  0  |  1  |  1  |   2 |   2 |   2 |   3  |   3  |   4  |  5   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| rect            |  0  |  0  |  0  |   0 |   0 |   0 |   1  |   1  |   1  |  1   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| amp             |  0  |  0  |  0  |   0 |   0 |   0 |   0  |   1  |   1  |  1   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| limit-modes     |  0  |  0  |  0  |   0 |   0 |   0 |   1  |   1  |   1  |  0   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| max-merge       |  2  |  2  |  2  |   2 |   2 |   2 |   3  |   3  |   4  |  5   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| early-skip      |  1  |  1  |  1  |   1 |   0 |   0 |   0  |   0  |   0  |  0   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| recursion-skip  |  1  |  1  |  1  |   1 |   1 |   1 |   1  |   1  |   0  |  0   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| fast-intra      |  1  |  1  |  1  |   1 |   1 |   0 |   0  |   0  |   0  |  0   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| b-intra         |  0  |  0  |  0  |   0 |   0 |   0 |   0  |   1  |   1  |  1   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| sao             |  0  |  0  |  1  |   1 |   1 |   1 |   1  |   1  |   1  |  1   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| signhide        |  0  |  1  |  1  |   1 |   1 |   1 |   1  |   1  |   1  |  1   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| weightp         |  0  |  0  |  1  |   1 |   1 |   1 |   1  |   1  |   1  |  1   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| weightb         |  0  |  0  |  0  |   0 |   0 |   0 |   0  |   1  |   1  |  1   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| aq-mode         |  0  |  0  |  1  |   1 |   1 |   1 |   1  |   1  |   1  |  1   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| cuTree          |  1  |  1  |  1  |   1 |   1 |   1 |   1  |   1  |   1  |  1   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| rdLevel         |  2  |  2  |  2  |   2 |   2 |   3 |   4  |   6  |   6  |  6   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| rdoq-level      |  0  |  0  |  0  |   0 |   0 |   0 |   2  |   2  |   2  |  2   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| tu-intra        |  1  |  1  |  1  |   1 |   1 |   1 |   1  |   2  |   3  |  4   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| tu-inter        |  1  |  1  |  1  |   1 |   1 |   1 |   1  |   2  |   3  |  4   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+
| limit-tu        |  0  |  0  |  0  |   0 |   0 |   0 |   0  |   4  |   4  |  0   |
+-----------------+-----+-----+-----+-----+-----+-----+------+------+------+------+

.. _tunings:

Tuning
======

There are a few :option:`--tune` options available, which are applied
after the preset.

.. Note::

	The *psnr* and *ssim* tune options disable all optimizations that
	sacrafice metric scores for perceived visual quality (also known as
	psycho-visual optimizations). By default x265 always tunes for
	highest perceived visual quality but if one intends to measure an
	encode using PSNR or SSIM for the purpose of benchmarking, we highly
	recommend you configure x265 to tune for that particular metric.

+--------------+-----------------------------------------------------+
| --tune       | effect                                              |
+==============+=====================================================+
| psnr         | disables adaptive quant, psy-rd, and cutree         |
+--------------+-----------------------------------------------------+
| ssim         | enables adaptive quant auto-mode, disables psy-rd   |
+--------------+-----------------------------------------------------+
| grain        | improves retention of film grain. more below        |
+--------------+-----------------------------------------------------+
| fastdecode   | no loop filters, no weighted pred, no intra in B    |
+--------------+-----------------------------------------------------+
| zerolatency  | no lookahead, no B frames, no cutree                |
+--------------+-----------------------------------------------------+



Film Grain
~~~~~~~~~~

:option:`--tune` *grain* aims to encode grainy content with the best 
visual quality. The purpose of this option is neither to retain nor 
eliminate grain, but prevent noticeable artifacts caused by uneven 
distribution of grain. :option:`--tune` *grain* strongly restricts 
algorithms that vary the quantization parameter within and across frames.
Tune grain also biases towards decisions that retain more high frequency
components.

    * :option:`--aq-mode` 0
    * :option:`--cutree` 0
    * :option:`--ipratio` 1.1
    * :option:`--pbratio` 1.0
    * :option:`--qpstep` 1
    * :option:`--sao` 0
    * :option:`--psy-rd` 4.0
    * :option:`--psy-rdoq` 10.0
    * :option:`--recursion-skip` 0
    
It also enables a specialised ratecontrol algorithm :option:`--rc-grain` 
that strictly minimises QP fluctuations across frames, while still allowing 
the encoder to hit bitrate targets and VBV buffer limits (with a slightly 
higher margin of error than normal). It is highly recommended that this 
algorithm is used only through the :option:`--tune` *grain* feature. 
Overriding the `--tune` *grain* settings might result in grain strobing, especially
when enabling features like :option:`--aq-mode` and :option:`--cutree` that modify
per-block QPs within a given frame.

Fast Decode
~~~~~~~~~~~

:option:`--tune` *fastdecode* disables encoder features which tend to be
bottlenecks for the decoder. It is intended for use with 4K content at
high bitrates which can cause decoders to struggle. It disables both
HEVC loop filters, which tend to be process bottlenecks:

    * :option:`--no-deblock`
    * :option:`--no-sao`

It disables weighted prediction, which tend to be bandwidth bottlenecks:

    * :option:`--no-weightp`
    * :option:`--no-weightb`

And it disables intra blocks in B frames with :option:`--no-b-intra`
since intra predicted blocks cause serial dependencies in the decoder.

Zero Latency
~~~~~~~~~~~~

There are two halves to the latency problem. There is latency at the
decoder and latency at the encoder. :option:`--tune` *zerolatency*
removes latency from both sides. The decoder latency is removed by:

    * :option:`--bframes` 0

Encoder latency is removed by:

    * :option:`--b-adapt` 0
    * :option:`--rc-lookahead` 0
    * :option:`--no-scenecut`
    * :option:`--no-cutree`
    * :option:`--frame-threads` 1

With all of these settings x265_encoder_encode() will run synchronously,
the picture passed as pic_in will be encoded and returned as NALs. These
settings disable frame parallelism, which is an important component for
x265 performance. If you can tolerate any latency on the encoder, you
can increase performance by increasing the number of frame threads. Each
additional frame thread adds one frame of latency.
