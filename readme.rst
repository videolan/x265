=================
x265 HEVC Encoder
=================

| **Read:** | Online `documentation <http://x265.readthedocs.org/en/default/>`_ | Developer `wiki <http://bitbucket.org/multicoreware/x265/wiki/>`_
| **Download:** | `releases <http://ftp.videolan.org/pub/videolan/x265/>`_ 
| **Interact:** | #x265 on freenode.irc.net | `x265-devel@videolan.org <http://mailman.videolan.org/listinfo/x265-devel>`_ | `Report an issue <https://bitbucket.org/multicoreware/x265/issues?status=new&status=open>`_

`x265 <https://www.videolan.org/developers/x265.html>`_ is an open
source HEVC encoder. See the developer wiki for instructions for
downloading and building the source.

x265 is free to use under the `GNU GPL <http://www.gnu.org/licenses/gpl-2.0.html>`_ 
and is also available under a commercial `license <http://x265.org>`_

## Modifications ##
Here the x265 code was modified to support a lowpass subband approximation for the DCT.
During performance tests this approximation had a gain of around 10% in performance. 
Thus allowing encoding time to be reduced at the same rate.

It also produced very small loss for 23 <= Qp =< 25 and minimal loss for Qp > 25, compared to the standard DCT.
