================================================
vsimagereader - Image reader for VapourSynth
================================================

Image file reader plugin for VapourSynth.

Supported formats are
    - BMP (Windows Bitmap)
    - JPEG
    - PNG (Portable Network Graphics)
    - TARGA (Truevision Advanced Raster Graphics Adapter)

Usage:
------
    >>> import vapoursynth as vs
    >>> core = vs.Core()
    >>> core.std.LoadPlugin('/path/to/vsimagereader.dll')

    - read single file:
    >>> clip = core.imgr.Read(['/path/to/file'])

    - read two or more files:
    >>> srcs = ['/path/to/file1', '/path/to/file2', ... ,'/path/to/fileX']
    >>> clip = core.imgr.Read(srcs)

    - read image sequence:
    >>> import os
    >>> ext = '.png'
    >>> dir = '/path/to/the/directory/'
    >>> srcs = [dir + src for src in os.listdir(dir) if src.endswith(ext)]
    >>> clip = core.imgr.Read(srcs)

About supported format:
-----------------------

    - BMP:
        1/2/4/8/24/32bit color RGB are supported except RLE compressed.

        output format is always RGB24.

    - JPEG:
        Generally, JPEG images are compressed after converted source RGB to 8bit planar YUV, and they are reconverted to RGB at the time of decoding.

        vsimagereader omits the reconversion to RGB, and keep them with YUV.

        If chroma-subsample-type of the image is YUV420 or YUV422, the width of that will be make into mod 2 with padding.

        Also, If subsample-type of the image is YUV420 or YUV440, the height of that will be make into mod 2 with padding.

    - PNG:
        1/2/4bits samples will be expanded to 8bits.

        All alpha channel data will be stripped.

    - TARGA:
        Only 24bit/32bit-RGB(uncompressed or RLE compressed) are supported. Color maps are not.

        All alpha channel data will be stripped.

    When reading two or more images, all those width, height, and output formats need to be the same.

Note:
-----
    - vsimagereader is using TurboJPEG/OSS library for parsing/decoding JPEG image.
      TurboJPEG/OSS is part of libjpeg-turbo project. libjpeg-turbo is a derivative of libjpeg that uses SIMD instructions (MMX, SSE2, NEON) to accelerate baseline JPEG compression and decompression on x86, x86-64, and ARM systems.
    - vsimagereader is using libpng for parsing/decoding PNG image.
    - vsimagereader is using part of libtga's source code for decoding compressed TARGA image.

How to compile:
---------------
    vsimagereader requires libpng-1.2(1.2.50 or later is recomended) and libturbojpeg.

    And, libpng requires zlib-1.0.4 or later(1.2.7 or later is recomended).

    Therefore, you have to install these libraries at first.

    If you have already installed them, type as follows.::

    $ git clone git://github.com/chikuzen/vsimagereader.git
    $ cd ./vsimagereader
    $ ./configure
    $ make

    - Currentry, libpng-1.4/1.5 cannot be used for vsimagereader.

Link:
-----
    vspimagereader source code repository:
        https://github.com/chikuzen/vsimagereader

    libjpeg-turbo:
        http://www.libjpeg-turbo.org/

    Independent JPEG Group:
        http://www.ijg.org/

    libpng.org:
        http://www.libpng.org/

    zlib:
        http://www.zlib.net/

    libtga:
        http://tgalib.sourceforge.net/

Author: Oka Motofumi (chikuzen.mo at gmail dot com)
