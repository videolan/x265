*********************************
Application Programming Interface
*********************************

Introduction
============

x265 is written primarily in C++ and x86 assembly language but the
public facing programming interface is C for the widest possible
portability.  This C interface is wholly defined within :file:`x265.h`
in the source/ folder of our source tree.  All of the functions and
variables and enumerations meant to be used by the end-user are present
in this header.

Where possible, x265 has tried to keep its public API as close as
possible to x264's public API. So those familiar with using x264 through
its C interface will find x265 quite familiar.

This file is meant to be read in-order; the narrative follows linearly
through the various sections

Build Considerations
====================

The choice of Main or Main10 profile encodes is made at compile time;
the internal pixel depth influences a great deal of variable sizes and
thus 8 and 10bit pixels are handled as different build options
(primarily to maintain the performance of the 8bit builds). libx265
exports a variable **x265_max_bit_depth** which indicates how the
library was compiled (it will contain a value of 8 or 10). Further,
**x265_version_str** is a pointer to a string indicating the version of
x265 which was compiled, and **x265_build_info_str** is a pointer to a
string identifying the compiler and build options.

.. Note::

	**x265_version_str** is only updated when **cmake** runs. If you are
	making binaries for others to use, it is recommended to run
	**cmake** prior to **make** in your build scripts.

x265 will accept input pixels of any depth between 8 and 16 bits
regardless of the depth of its internal pixels (8 or 10).  It will shift
and mask input pixels as required to reach the internal depth. If
downshifting is being performed using our CLI application, the
:option:`--dither` option may be enabled to reduce banding. This feature
is not available through the C interface.

Encoder
=======

The primary object in x265 is the encoder object, and this is
represented in the public API as an opaque typedef **x265_encoder**.
Pointers of this type are passed to most encoder functions.

A single encoder generates a single output bitstream from a sequence of
raw input pictures.  Thus if you need multiple output bitstreams you
must allocate multiple encoders.  You may pass the same input pictures
to multiple encoders, the encode function does not modify the input
picture structures (the pictures are copied into the encoder as the
first step of encode).

Encoder allocation is a reentrant function, so multiple encoders may be
safely allocated in a single process. The encoder access functions are
not reentrant for a single encoder, so the recommended use case is to
allocate one client thread per encoder instance (one thread for all
encoder instances is possible, but some encoder access functions are
blocking and thus this would be less efficient).

.. Note::

	There is one caveat to having multiple encoders within a single
	process. All of the encoders must use the same maximum CTU size
	because many global variables are configured based on this size.
	Encoder allocation will fail if a mis-matched CTU size is attempted.
	If no encoders are open, **x265_cleanup()** can be called to reset
	the configured CTU size so a new size can be used.

An encoder is allocated by calling **x265_encoder_open()**::

	/* x265_encoder_open:
	 *      create a new encoder handler, all parameters from x265_param are copied */
	x265_encoder* x265_encoder_open(x265_param *);

The returned pointer is then passed to all of the functions pertaining
to this encode. A large amount of memory is allocated during this
function call, but the encoder will continue to allocate memory as the
first pictures are passed to the encoder; until its pool of picture
structures is large enough to handle all of the pictures it must keep
internally.  The pool size is determined by the lookahead depth, the
number of frame threads, and the maximum number of references.

As indicated in the comment, **x265_param** is copied internally so the user
may release their copy after allocating the encoder.  Changes made to
their copy of the param structure have no affect on the encoder after it
has been allocated.

Param
=====

The **x265_param** structure describes everything the encoder needs to
know about the input pictures and the output bitstream and most
everything in between.

The recommended way to handle these param structures is to allocate them
from libx265 via::

	/* x265_param_alloc:
	 *  Allocates an x265_param instance. The returned param structure is not
	 *  special in any way, but using this method together with x265_param_free()
	 *  and x265_param_parse() to set values by name allows the application to treat
	 *  x265_param as an opaque data struct for version safety */
	x265_param *x265_param_alloc();

In this way, your application does not need to know the exact size of
the param structure (the build of x265 could potentially be a bit newer
than the copy of :file:`x265.h` that your application compiled against).

Next you perform the initial *rough cut* configuration of the encoder by
chosing a performance preset and optional tune factor
**x265_preset_names** and **x265_tune_names** respectively hold the
string names of the presets and tune factors (see :ref:`presets
<preset-tune-ref>` for more detail on presets and tune factors)::

	/*      returns 0 on success, negative on failure (e.g. invalid preset/tune name). */
	int x265_param_default_preset(x265_param *, const char *preset, const char *tune);

Now you may optionally specify a profile. **x265_profile_names**
contains the string names this function accepts::

	/*      (can be NULL, in which case the function will do nothing)
	 *      returns 0 on success, negative on failure (e.g. invalid profile name). */
	int x265_param_apply_profile(x265_param *, const char *profile);

Finally you configure any remaining options by name using repeated calls to::

	/* x265_param_parse:
	 *  set one parameter by name.
	 *  returns 0 on success, or returns one of the following errors.
	 *  note: BAD_VALUE occurs only if it can't even parse the value,
	 *  numerical range is not checked until x265_encoder_open().
	 *  value=NULL means "true" for boolean options, but is a BAD_VALUE for non-booleans. */
	#define X265_PARAM_BAD_NAME  (-1)
	#define X265_PARAM_BAD_VALUE (-2)
	int x265_param_parse(x265_param *p, const char *name, const char *value);

See :ref:`string options <string-options-ref>` for the list of options (and their
descriptions) which can be set by **x265_param_parse()**.

After the encoder has been created, you may release the param structure::

	/* x265_param_free:
	 *  Use x265_param_free() to release storage for an x265_param instance
	 *  allocated by x265_param_alloc() */
	void x265_param_free(x265_param *);

.. Note::

	Using these methods to allocate and release the param structures
	helps future-proof your code in many ways, but the x265 API is
	versioned in such a way that we prevent linkage against a build of
	x265 that does not match the version of the header you are compiling
	against. This is function of the X265_BUILD macro.

**x265_encoder_parameters()** may be used to get a copy of the param
structure from the encoder after it has been opened, in order to see the
changes made to the parameters for auto-detection and other reasons::

	/* x265_encoder_parameters:
	 *      copies the current internal set of parameters to the pointer provided
	 *      by the caller.  useful when the calling application needs to know
	 *      how x265_encoder_open has changed the parameters.
	 *      note that the data accessible through pointers in the returned param struct
	 *      (e.g. filenames) should not be modified by the calling application. */
	void x265_encoder_parameters(x265_encoder *, x265_param *);

**x265_encoder_reconfig()** may be used to reconfigure encoder parameters mid-encode::

	/* x265_encoder_reconfig:
	 *       used to modify encoder parameters.
	 *      various parameters from x265_param are copied.
	 *      this takes effect immediately, on whichever frame is encoded next;
	 *      returns 0 on success, negative on parameter validation error.
	 *
	 *      not all parameters can be changed; see the actual function for a
	 *      detailed breakdown.  since not all parameters can be changed, moving
	 *      from preset to preset may not always fully copy all relevant parameters,
	 *      but should still work usably in practice. however, more so than for
	 *      other presets, many of the speed shortcuts used in ultrafast cannot be
	 *      switched out of; using reconfig to switch between ultrafast and other
	 *      presets is not recommended without a more fine-grained breakdown of
	 *      parameters to take this into account. */
	int x265_encoder_reconfig(x265_encoder *, x265_param *);
	
Pictures
========

Raw pictures are passed to the encoder via the **x265_picture** structure.
Just like the param structure we recommend you allocate this structure
from the encoder to avoid potential size mismatches::

	/* x265_picture_alloc:
	 *  Allocates an x265_picture instance. The returned picture structure is not
	 *  special in any way, but using this method together with x265_picture_free()
	 *  and x265_picture_init() allows some version safety. New picture fields will
	 *  always be added to the end of x265_picture */
	x265_picture *x265_picture_alloc();

Regardless of whether you allocate your picture structure this way or
whether you simply declare it on the stack, your next step is to
initialize the structure via::

	/***
	 * Initialize an x265_picture structure to default values. It sets the pixel
	 * depth and color space to the encoder's internal values and sets the slice
	 * type to auto - so the lookahead will determine slice type.
	 */
	void x265_picture_init(x265_param *param, x265_picture *pic);

x265 does not perform any color space conversions, so the raw picture's
color space (chroma sampling) must match the color space specified in
the param structure used to allocate the encoder. **x265_picture_init**
initializes this field to the internal color space and it is best to
leave it unmodified.

The picture bit depth is initialized to be the encoder's internal bit
depth but this value should be changed to the actual depth of the pixels
being passed into the encoder.  If the picture bit depth is more than 8,
the encoder assumes two bytes are used to represent each sample
(little-endian shorts).

The user is responsible for setting the plane pointers and plane strides
(in units of bytes, not pixels). The presentation time stamp (**pts**)
is optional, depending on whether you need accurate decode time stamps
(**dts**) on output.

If you wish to override the lookahead or rate control for a given
picture you may specify a slicetype other than X265_TYPE_AUTO, or a
forceQP value other than 0.

x265 does not modify the picture structure provided as input, so you may
reuse a single **x265_picture** for all pictures passed to a single
encoder, or even all pictures passed to multiple encoders.

Structures allocated from the library should eventually be released::

	/* x265_picture_free:
	 *  Use x265_picture_free() to release storage for an x265_picture instance
	 *  allocated by x265_picture_alloc() */
	void x265_picture_free(x265_picture *);


Analysis Buffers
================

Analysis information can be saved and reused to between encodes of the
same video sequence (generally for multiple bitrate encodes).  The best
results are attained by saving the analysis information of the highest
bitrate encode and reuse it in lower bitrate encodes.

When saving or loading analysis data, buffers must be allocated for
every picture passed into the encoder using::

	/* x265_alloc_analysis_data:
	 *  Allocate memory to hold analysis meta data, returns 1 on success else 0 */
	int x265_alloc_analysis_data(x265_picture*);

Note that this is very different from the typical semantics of
**x265_picture**, which can be reused many times. The analysis buffers must
be re-allocated for every input picture.

Analysis buffers passed to the encoder are owned by the encoder until
they pass the buffers back via an output **x265_picture**. The user is
responsible for releasing the buffers when they are finished with them
via::

	/* x265_free_analysis_data:
	 *  Use x265_free_analysis_data to release storage of members allocated by
	 *  x265_alloc_analysis_data */
	void x265_free_analysis_data(x265_picture*);


Encode Process
==============

The output of the encoder is a series of NAL packets, which are always
returned concatenated in consecutive memory. HEVC streams have SPS and
PPS and VPS headers which describe how the following packets are to be
decoded. If you specified :option:`--repeat-headers` then those headers
will be output with every keyframe.  Otherwise you must explicitly query
those headers using::

	/* x265_encoder_headers:
	 *      return the SPS and PPS that will be used for the whole stream.
	 *      *pi_nal is the number of NAL units outputted in pp_nal.
	 *      returns negative on error, total byte size of payload data on success
	 *      the payloads of all output NALs are guaranteed to be sequential in memory. */
	int x265_encoder_headers(x265_encoder *, x265_nal **pp_nal, uint32_t *pi_nal);

Now we get to the main encode loop. Raw input pictures are passed to the
encoder in display order via::

	/* x265_encoder_encode:
	 *      encode one picture.
	 *      *pi_nal is the number of NAL units outputted in pp_nal.
	 *      returns negative on error, zero if no NAL units returned.
	 *      the payloads of all output NALs are guaranteed to be sequential in memory. */
	int x265_encoder_encode(x265_encoder *encoder, x265_nal **pp_nal, uint32_t *pi_nal, x265_picture *pic_in, x265_picture *pic_out);

These pictures are queued up until the lookahead is full, and then the
frame encoders in turn are filled, and then finally you begin receiving
a output NALs (corresponding to a single output picture) with each input
picture you pass into the encoder.

Once the pipeline is completely full, **x265_encoder_encode()** will
block until the next output picture is complete.

.. note:: 

	Optionally, if the pointer of a second **x265_picture** structure is
	provided, the encoder will fill it with data pertaining to the
	output picture corresponding to the output NALs, including the
	recontructed image, POC and decode timestamp. These pictures will be
	in encode (or decode) order.

When the last of the raw input pictures has been sent to the encoder,
**x265_encoder_encode()** must still be called repeatedly with a
*pic_in* argument of 0, indicating a pipeline flush, until the function
returns a value less than or equal to 0 (indicating the output bitstream
is complete).

At any time during this process, the application may query running
statistics from the encoder::

	/* x265_encoder_get_stats:
	 *       returns encoder statistics */
	void x265_encoder_get_stats(x265_encoder *encoder, x265_stats *, uint32_t statsSizeBytes);

Cleanup
=======

At the end of the encode, the application will want to trigger logging
of the final encode statistics, if :option:`--csv` had been specified::

	/* x265_encoder_log:
	 *       write a line to the configured CSV file.  If a CSV filename was not
	 *       configured, or file open failed, or the log level indicated frame level
	 *       logging, this function will perform no write. */
	void x265_encoder_log(x265_encoder *encoder, int argc, char **argv);

Finally, the encoder must be closed in order to free all of its
resources. An encoder that has been flushed cannot be restarted and
reused. Once **x265_encoder_close()** has been called, the encoder
handle must be discarded::

	/* x265_encoder_close:
	 *      close an encoder handler */
	void x265_encoder_close(x265_encoder *);

When the application has completed all encodes, it should call
**x265_cleanup()** to free process global, particularly if a memory-leak
detection tool is being used. **x265_cleanup()** also resets the saved
CTU size so it will be possible to create a new encoder with a different
CTU size::

	/* x265_cleanup:
	 *     release library static allocations, reset configured CTU size */
	void x265_cleanup(void);


Multi-library Interface
=======================

If your application might want to make a runtime selection between
a number of libx265 libraries (perhaps 8bpp and 16bpp), then you will
want to use the multi-library interface.

Instead of directly using all of the **x265_** methods documented
above, you query an x265_api structure from your libx265 and then use
the function pointers within that structure of the same name, but
without the **x265_** prefix. So **x265_param_default()** becomes
**api->param_default()**. The key method is x265_api_get()::

    /* x265_api_get:
     *   Retrieve the programming interface for a linked x265 library.
     *   May return NULL if no library is available that supports the
     *   requested bit depth. If bitDepth is 0, the function is guarunteed
     *   to return a non-NULL x265_api pointer from the system default
     *   libx265 */
    const x265_api* x265_api_get(int bitDepth);

Note that using this multi-library API in your application is only the
first step.

Your application must link to one build of libx265 (statically or 
dynamically) and this linked version of libx265 will support one 
bit-depth (8 or 10 bits). 

Your application must now request the API for the bitDepth you would 
prefer the encoder to use (8 or 10). If the requested bitdepth is zero, 
or if it matches the bitdepth of the system default libx265 (the 
currently linked library), then this library will be used for encode.
If you request a different bit-depth, the linked libx265 will attempt 
to dynamically bind a shared library with a name appropriate for the 
requested bit-depth:

    8-bit:  libx265_main.dll
    10-bit: libx265_main10.dll

    (the shared library extension is obviously platform specific. On
    Linux it is .so while on Mac it is .dylib)

For example on Windows, one could package together an x265.exe
statically linked against the 8bpp libx265 together with a
libx265_main10.dll in the same folder, and this executable would be able
to encode main and main10 bitstreams.

On Linux, x265 packagers could install 8bpp static and shared libraries
under the name libx265 (so all applications link against 8bpp libx265)
and then also install libx265_main10.so (symlinked to its numbered solib).
Thus applications which use x265_api_get() will be able to generate main
or main10 bitstreams.
