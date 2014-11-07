Preset Options
--------------

Presets
=======

.. _preset-tune-ref:

x265 has a number of predefined :option:`--preset` options that make
trade-offs between encode speed (encoded frames per second) and
compression efficiency (quality per bit in the bitstream).  The default
preset is medium, it does a reasonably good job of finding the best
possible quality without spending enormous CPU cycles looking for the
absolute most efficient way to achieve that quality.  As you go higher
than medium, the encoder takes shortcuts to improve performance at the
expense of quality and compression efficiency.  As you go lower than
medium, the encoder tries harder and harder to achieve the best quailty
per bit compression ratio.

The presets adjust encoder parameters to affect these trade-offs.

+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
|              | ultrafast | superfast | veryfast | faster | fast | medium | slow | slower | veryslow | placebo |
+==============+===========+===========+==========+========+======+========+======+========+==========+=========+
| ctu          |   32      |    32     |   32     |  64    |  64  |   64   |  64  |  64    |   64     |   64    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| bframes      |    4      |     4     |    4     |   4    |  4   |    4   |  4   |   8    |    8     |    8    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| b-adapt      |    0      |     0     |    0     |   0    |  2   |    2   |  2   |   2    |    2     |    2    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| rc-lookahead |   10      |    10     |   15     |  15    |  15  |   20   |  25  |   30   |   40     |   60    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| scenecut     |    0      |    40     |   40     |  40    |  40  |   40   |  40  |   40   |   40     |   40    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| refs         |    1      |     1     |    1     |   1    |  3   |    3   |  3   |   3    |    5     |    5    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| me           |   dia     |   hex     |   hex    |  hex   | hex  |   hex  | star |  star  |   star   |   star  |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| merange      |   25      |    44     |   57     |  57    |  57  |   57   | 57   |  57    |   57     |   92    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| subme        |    0      |     1     |    1     |   2    |  2   |    2   |  3   |   3    |    4     |    5    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| rect         |    0      |     0     |    0     |   0    |  0   |    0   |  1   |   1    |    1     |    1    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| amp          |    0      |     0     |    0     |   0    |  0   |    0   |  0   |   1    |    1     |    1    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| max-merge    |    2      |     2     |    2     |   2    |  2   |    2   |  3   |   3    |    4     |    5    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| early-skip   |    1      |     1     |    1     |   1    |  0   |    0   |  0   |   0    |    0     |    0    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| fast-intra   |    1      |     1     |    1     |   1    |  1   |    0   |  0   |   0    |    0     |    0    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| b-intra      |    0      |     0     |    0     |   0    |  0   |    0   |  0   |   1    |    1     |    1    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| sao          |    0      |     0     |    1     |   1    |  1   |    1   |  1   |   1    |    1     |    1    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| signhide     |    0      |     1     |    1     |   1    |  1   |    1   |  1   |   1    |    1     |    1    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| weightp      |    0      |     0     |    1     |   1    |  1   |    1   |  1   |   1    |    1     |    1    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| weightb      |    0      |     0     |    0     |   0    |  0   |    0   |  0   |   1    |    1     |    1    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| aq-mode      |    0      |     0     |    2     |   2    |  2   |    2   |  2   |   2    |    2     |    2    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| cuTree       |    0      |     0     |    0     |   0    |  1   |    1   |  1   |   1    |    1     |    1    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| rdLevel      |    2      |     2     |    2     |   2    |  2   |    3   |  4   |   6    |    6     |    6    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| deblock      |    0      |     1     |    1     |   1    |  1   |    1   |  1   |   1    |    1     |    1    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| tu-intra     |    1      |     1     |    1     |   1    |  1   |    1   |  1   |   2    |    3     |    4    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+
| tu-inter     |    1      |     1     |    1     |   1    |  1   |    1   |  1   |   2    |    3     |    4    |
+--------------+-----------+-----------+----------+--------+------+--------+------+--------+----------+---------+

Placebo mode enables transform-skip prediction evaluation.

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


Film Grain Retention
~~~~~~~~~~~~~~~~~~~~

:option:`--tune` grain tries to improve the retention of film grain in
the reconstructed output. To help rate distortion optimizations to
select modes which preserve high frequency noise:

    * :option:`--psy-rd` 0.5
    * :option:`--psy-rdoq` 30
    * :option:`--b-intra`

It lowers the strength of adaptive quantization, so residual energy can
be more evenly distributed across the (noisy) picture:

    * :option:`--aq-mode` 1
    * :option:`--aq-strength` 0.3

And it similarly tunes rate control to prevent the slice QP from
swinging too wildly from frame to frame:

    * :option:`--ipratio` 1.1
    * :option:`--pbratio` 1.1
    * :option:`--qcomp` 0.8

And lastly it reduces the strength of deblocking to prevent grain being
blurred on block boundaries:

    * :option:`--deblock` -2

