.. Permission is granted to copy, distribute and/or modify this
.. document under the terms of the GNU Free Documentation License,
.. Version 1.1 or any later version published by the Free Software
.. Foundation, with no Invariant Sections, no Front-Cover Texts
.. and no Back-Cover Texts. A copy of the license is included at
.. Documentation/userspace-api/media/fdl-appendix.rst.
..
.. TODO: replace it to GFDL-1.1-or-later WITH no-invariant-sections

.. _V4L2-PIX-FMT-NV15:

**************************
V4L2_PIX_FMT_NV15 ('NV15')
**************************

Format with ½ horizontal and vertical chroma resolution, also known as
YUV 4:2:0. One luminance and one chrominance plane with alternating
chroma samples similar to ``V4L2_PIX_FMT_NV12`` but with 10-bit samples
that are grouped into four and packed into five bytes.

The '15' suffix refers to the optimum effective bits per pixel which is
achieved when the total number of luminance samples is a multiple of 8.


Description
===========

This is a packed 10-bit two-plane version of the YUV 4:2:0 format. The
three components are separated into two sub-images or planes. The Y plane
is first. The Y plane has five bytes per each group of four pixels. A
combined CbCr plane immediately follows the Y plane in memory. The CbCr
plane is the same width, in bytes, as the Y plane (and of the image), but
is half as tall in pixels. Each CbCr pair belongs to four pixels. For
example, Cb\ :sub:`00`/Cr\ :sub:`00` belongs to Y'\ :sub:`00`,
Y'\ :sub:`01`, Y'\ :sub:`10`, Y'\ :sub:`11`.

If the Y plane has pad bytes after each row, then the CbCr plane has as
many pad bytes after its rows.

**Byte Order.**
Little endian. Each cell is one byte. Pixels cross the byte boundary.


.. flat-table::
    :header-rows:  0
    :stub-columns: 0

    * - start + 0:
      - Y'\ :sub:`00[7:0]`
      - Y'\ :sub:`01[5:0]`\ Y'\ :sub:`00[9:8]`
      - Y'\ :sub:`02[3:0]`\ Y'\ :sub:`01[9:6]`
      - Y'\ :sub:`03[1:0]`\ Y'\ :sub:`02[9:4]`
      - Y'\ :sub:`03[9:2]`
    * - start + 5:
      - Y'\ :sub:`10[7:0]`
      - Y'\ :sub:`11[5:0]`\ Y'\ :sub:`10[9:8]`
      - Y'\ :sub:`12[3:0]`\ Y'\ :sub:`11[9:6]`
      - Y'\ :sub:`13[1:0]`\ Y'\ :sub:`12[9:4]`
      - Y'\ :sub:`13[9:2]`
    * - start + 10:
      - Cb'\ :sub:`00[7:0]`
      - Cr'\ :sub:`00[5:0]`\ Cb'\ :sub:`00[9:8]`
      - Cb'\ :sub:`01[3:0]`\ Cr'\ :sub:`00[9:6]`
      - Cr'\ :sub:`01[1:0]`\ Cb'\ :sub:`01[9:4]`
      - Cr'\ :sub:`01[9:2]`


**Color Sample Location:**

.. flat-table::
    :header-rows:  0
    :stub-columns: 0

    * -
      - 0
      -
      - 1
      - 2
      -
      - 3
    * - 0
      - Y
      -
      - Y
      - Y
      -
      - Y
    * -
      -
      - C
      -
      -
      - C
      -
    * - 1
      - Y
      -
      - Y
      - Y
      -
      - Y
