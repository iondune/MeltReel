
MeltReel
========

Generates [Melt](https://www.mltframework.org/docs/melt/) XML configurations for highlight reels assembled from numerous video clips.

Usage
-----

First, `MeltReel` is used to generate Melt XML configuration from a list of timestamps.

`./MeltReel.exe clips.csv reel.xml`

`clips.csv` should be tab-separate values (no header) with each line consisting of
a part file (the video file containing the clip), a time stamp in, and a time stamp out.

Melt can then be used to render this XML configuration to a video.

`melt reel.xml -consumer avformat:reel.avi acodec=libmp3lame vcodec=libx264`

To Do
-----

The following features are planned but currently unsupported:

* Generate a list of timestamps for used in e.g. a YouTube Description
* Control over the transition type between clips (e.g. fade, quick cut, potentially other)
* Control over clip playback speed (e.g. slow motion and timelapse)
* Sub-second timestamps for clips
* Support for timestamps of the format `10h20m30s`
* Graphic and text overlays
* Support for framerates other than 25
