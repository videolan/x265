Lossless
--------

Lossless Encoding
=================

x265 can encode HEVC bitstreams that are entirely lossless (the
reconstructed images are bit-exact to the source images) by using the
:option:`--lossless` option.  Lossless operation is theoretically
simple. Rate control, by definition, is disabled and the encoder
disables all quality metrics since they would only waste CPU cycles.
Instead, x265 reports only a compression factor at the end of the
encode.

In HEVC, lossless coding means bypassing both the DCT transforms and
bypassing quantization (often referred to as transquant bypass).  Normal
predictions are still allowed, so the encoder will find optimal inter or
intra predictions and then losslessly code the residual (with transquant
bypass).

All :option:`--preset` options are capable of generating lossless video
streams, but in general the slower the preset the better the compression
ratio (and the slower the encode). Here are some examples::

	./x265 ../test-720p.y4m o.bin --preset ultrafast --lossless
	... <snip> ...
	encoded 721 frames in 238.38s (3.02 fps), 57457.94 kb/s

	./x265 ../test-720p.y4m o.bin --preset faster --lossless
	... <snip> ...
	x265 [info]: lossless compression ratio 3.11::1
	encoded 721 frames in 258.46s (2.79 fps), 56787.65 kb/s

	./x265 ../test-720p.y4m o.bin --preset slow --lossless
	... <snip> ...
	x265 [info]: lossless compression ratio 3.36::1
	encoded 721 frames in 576.73s (1.25 fps), 52668.25 kb/s

	./x265 ../test-720p.y4m o.bin --preset veryslow --lossless
	x265 [info]: lossless compression ratio 3.76::1
	encoded 721 frames in 6298.22s (0.11 fps), 47008.65 kb/s
 
.. Note::
	In HEVC, only QP=4 is truly lossless quantization, and thus when
	encoding losslesly x265 uses QP=4 internally in its RDO decisions.

Near-lossless Encoding
======================

Near-lossless conditions are a quite a bit more interesting.  Normal ABR
rate control will allow one to scale the bitrate up to the point where 
quantization is entirely bypassed (QP <= 4), but even at this point
there is a lot of SSIM left on the table because of the DCT transforms,
which are not lossless::

	./x265 ../test-720p.y4m o.bin --preset medium --bitrate 40000 --ssim
	encoded 721 frames in 326.62s (2.21 fps), 39750.56 kb/s, SSIM Mean Y: 0.9990703 (30.317 dB)
	
	./x265 ../test-720p.y4m o.bin --preset medium --bitrate 50000 --ssim
	encoded 721 frames in 349.27s (2.06 fps), 44326.84 kb/s, SSIM Mean Y: 0.9994134 (32.316 dB)
	
	./x265 ../test-720p.y4m o.bin --preset medium --bitrate 60000 --ssim
	encoded 721 frames in 360.04s (2.00 fps), 45394.50 kb/s, SSIM Mean Y: 0.9994823 (32.859 dB)

For the encoder to get over this quality plateau, one must enable
lossless coding at the CU level with :option:`--cu-lossless`.  It tells
the encoder to evaluate trans-quant bypass as a coding option for each
CU, and to pick the option with the best rate-distortion
characteristics.

The :option:`--cu-lossless` option is very expensive, computationally,
and it only has a positive effect when the QP is extremely low, allowing
RDO to spend a large amount of bits to make small improvements to
quality.  So this option should only be enabled when you are encoding
near-lossless bitstreams::

	./x265 ../test-720p.y4m o.bin --preset medium --bitrate 40000 --ssim --cu-lossless
	encoded 721 frames in 500.51s (1.44 fps), 40017.10 kb/s, SSIM Mean Y: 0.9997790 (36.557 dB)
	
	./x265 ../test-720p.y4m o.bin --preset medium --bitrate 50000 --ssim --cu-lossless
	encoded 721 frames in 524.60s (1.37 fps), 46083.37 kb/s, SSIM Mean Y: 0.9999432 (42.456 dB)
	
	./x265 ../test-720p.y4m o.bin --preset medium --bitrate 60000 --ssim --cu-lossless
	encoded 721 frames in 523.63s (1.38 fps), 46552.92 kb/s, SSIM Mean Y: 0.9999489 (42.917 dB)

.. Note::
	It is not unusual for bitrate to drop as you increase lossless coding.
	Having "perfectly coded" reference blocks reduces residual in later
	frames. It is quite possible for a near-lossless encode to spend
	more bits than a lossless encode.

Enabling psycho-visual rate distortion will improve lossless coding.
:option:`--psy-rd` influences the RDO decisions in favor of energy
(detail) preservation over bit cost and results in more blocks being
losslessly coded.  Our psy-rd feature is not yet assembly optimized, so
this makes the encodes run even slower::

	./x265 ../test-720p.y4m o.bin --preset medium --bitrate 40000 --ssim --cu-lossless --psy-rd 1.0
	encoded 721 frames in 581.83s (1.24 fps), 40112.15 kb/s, SSIM Mean Y: 0.9998632 (38.638 dB)
	
	./x265 ../test-720p.y4m o.bin --preset medium --bitrate 50000 --ssim --cu-lossless --psy-rd 1.0
	encoded 721 frames in 587.54s (1.23 fps), 46284.55 kb/s, SSIM Mean Y: 0.9999663 (44.721 dB)
	
	./x265 ../test-720p.y4m o.bin --preset medium --bitrate 60000 --ssim --cu-lossless --psy-rd 1.0
	encoded 721 frames in 592.93s (1.22 fps), 46839.51 kb/s, SSIM Mean Y: 0.9999707 (45.334 dB)

:option:`--cu-lossless` will also be more effective at slower
presets which perform RDO at more levels and thus may find smaller
blocks that would benefit from lossless coding::

	./x265 ../test-720p.y4m o.bin --preset veryslow --bitrate 40000 --ssim --cu-lossless
	encoded 721 frames in 12969.25s (0.06 fps), 37331.96 kb/s, SSIM Mean Y: 0.9998108 (37.231 dB)
	
	./x265 ../test-720p.y4m o.bin --preset veryslow --bitrate 50000 --ssim --cu-lossless
	encoded 721 frames in 46217.84s (0.05 fps), 42976.28 kb/s, SSIM Mean Y: 0.9999482 (42.856 dB)
	
	./x265 ../test-720p.y4m o.bin --preset veryslow --bitrate 60000 --ssim --cu-lossless
	encoded 721 frames in 13738.17s (0.05 fps), 43864.21 kb/s, SSIM Mean Y: 0.9999633 (44.348 dB)
	
And with psy-rd and a slow preset together, very high SSIMs are
possible::

	./x265 ../test-720p.y4m o.bin --preset veryslow --bitrate 40000 --ssim --cu-lossless --psy-rd 1.0
	encoded 721 frames in 11675.81s (0.06 fps), 37819.45 kb/s, SSIM Mean Y: 0.9999181 (40.867 dB)
	    
	./x265 ../test-720p.y4m o.bin --preset veryslow --bitrate 50000 --ssim --cu-lossless --psy-rd 1.0
	encoded 721 frames in 12414.56s (0.06 fps), 42815.75 kb/s, SSIM Mean Y: 0.9999758 (46.168 dB)
	
	./x265 ../test-720p.y4m o.bin --preset veryslow --bitrate 60000 --ssim --cu-lossless --psy-rd 1.0
	encoded 721 frames in 11684.89s (0.06 fps), 43324.48 kb/s, SSIM Mean Y: 0.9999793 (46.835 dB)


It's important to note in the end that it is easier (less work) for the
encoder to encode the video losslessly than it is to encode it
near-losslessly. If the encoder knows up front the encode must be
lossless, it does not need to evaluate any lossy coding methods. The
encoder only needs to find the most efficient prediction for each block
and then entropy code the residual.

It is not feasible for :option:`--cu-lossless` to turn itself on when
the encoder determines it is encoding a near-lossless bitstream (ie:
when rate control nearly disables all quantization) because the feature
requires a flag to be enabled in the stream headers. At the time the
stream headers are being coded we do not know whether
:option:`--cu-lossless` would be a help or a hinder.  If very few or no
blocks end up being coded as lossless, then having the feature enabled
is a net loss in compression efficiency because it adds a flag that must
be coded for every CU. So ignoring even the performance aspects of the
feature, it can be a compression loss if enabled without being used. So
it is up to the user to only enable this feature when they are coding at
near-lossless quality.

Transform Skip
==============

A somewhat related feature, :option:`--tskip` tells the encoder to
evaluate transform-skip (bypass DCT but with quantization still enabled)
when coding small 4x4 transform blocks. This feature is intended to
improve the coding efficiency of screen content (aka: text on a screen)
and is not really intended for lossless coding.  This feature should
only be enabled if the content has a lot of very sharp edges in it, and
is mostly unrelated to lossless coding.
