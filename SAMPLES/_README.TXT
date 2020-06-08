Sample files for SCC Tools

The files in this directory represent most of the types of source files that
can be processed by SCC Tools.  MPEG and DVR-MS files are too large to be
included.

* dvd.bin: example of DVD Closed Captions ripped using method described in
  the DVD_FILTER.HTML webpage (now depreciated in favor of the VobSub method).
  Convert to SCC format using RAW2SCC and from there to CCD format using
  CCASDI.  The output is a Field 1 file containing channel CC1 pop-on
  captions.
* river.pas: example of CCWriter (DOS) captions.  Convert to SCC format using
  PAS2SCC and from there to CCD format using CCASDI.  The output is a Field 1
  file containing channel CC1 pop-on captions.
* spanish.bin: example of TV Pop-On Closed Captions captured with a TV
  capture card.  Convert to SCC format using RAW2SCC and from there to CCD
  format using CCASDI.  The output is a Field 1 file containing channel CC2
  pop-on captions.  The useful differences between this file and
  tv_popon.bin are that this is a Channel 2 file and that it uses lots of
  Special and Extended Characters.
* SubRip.srt: example of a SubRip subtitle file including every complexity
  SUBRIP2SCC can handle (it's even more complicated than SubRip can
  handle).  Convert to SCC format using SUBRIP2SCC and from there to CCD
  format using CCASDI.  The output is a Field 1 file containing channel CC1
  pop-on captions.
* text.bin: Example of TV Text.  I created this myself since I can't seem to
  find TV text anywhere anymore.  Convert to SCC format using RAW2SCC and
  from there to CCD format using CCASDI.  The output is a Field 1 file
  containing both a channel T2 ITV link and channel T1 text.  Text mode is
  used to provide screens of text that may be only slightly related to
  the video content.  In fact, Text mode by default puts an opaque box
  behind the text and in front of the video so it is easier to see.
  Interactive TeleVision (ITV) on the other hand is a system for embedding
  internet links in a television program so users of MSN TV can use them to
  browse from the TV program to a related web site.
* tv_painton.bin: Example of TV Paint-On Closed Captions captured with a TV
  capture card.  Convert to SCC format using RAW2SCC and from there to CCD
  format using CCASDI.  The output is a Field 1 file containing channel CC1
  paint-on captions.  Paint-on captions are used when there isn't time for a
  caption to be drawn in an off-screen buffer (the method used for pop-on
  captions).  The only place you'll see them is as the first caption of a
  commercial.
* tv_popon.bin: Example of TV Pop-On Closed Captions captured with a TV
  capture card.  Convert to SCC format using RAW2SCC and from there to CCD
  format using CCASDI.  The output is a Field 1 file containing channel CC1
  pop-on captions.  This is the format used for all pre-recorded television
  programs and video tapes.
* tv_rollup.bin: Example of TV Roll-Up Closed Captions captured with a TV
  capture card.  Convert to SCC format using RAW2SCC and from there to CCD
  format using CCASDI.  The output is a Field 1 file containing channel CC1
  roll-up captions.  This is the format used on live television programs,
  where the text appears as it is received in a box at the bottom of the
  screen and scrolls up as more lines are received.
* vts_01_0.sub.cc.raw: Example of a VobSub caption extract file ripped from
  a DVD using VobSub.  Convert to SCC format using VOBSUB2SCC and from there
  to CCD format using CCASDI.  The output is a Field 1 file containing
  channel CC1 pop-on captions.
* xds.bin: Example of TV eXtended Data Service commands, captured using a
  ReplayTV (4000 series) and extracted using CCExtract.  Convert to SCC format
  using RAW2SCC and from there to CCD format using CCASDI.  The output is a
  Field 2 file containing channel CC3 XDS commands.  XDS is primarily used to
  transmit the time of day and to provide V-chip ratings for television
  programs.


