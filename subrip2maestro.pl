# subrip2maestro.pl: Convert Subripper subtitles to DVDMaestro format
# Argument is name of .srt file to convert to same name.stl
# If no arguments, uses default of subtitles.srt
# Uses subrip2maestro.ini file in directory with files (creates with 
#  default settings if missing).
#
# Version 1.0
# McPoodle (mcpoodle@sonic.net)

sub intro;

# initial variables
$fps = 25; # PAL framerate
$input = "subtitles.srt"; # default subtitle file
$inifile = "subrip2maestro.ini";
$header[0] = "\$FontName           = Arial";
$header[1] = "\$FontSize           = 36";
$header[2] = "\$Bold               = FALSE";
$header[3] = "\$UnderLined         = FALSE";
$header[4] = "\$Italic             = FALSE";
$header[5] = "\$HorzAlign          = Center";
$header[6] = "\$VertAlign          = Bottom";
$header[7] = "\$XOffset            = 0";
$header[8] = "\$YOffset            = 0";
$header[9] = "\$ColorIndex1        = 0";
$header[10] = "\$ColorIndex2        = 1";
$header[11] = "\$ColorIndex3        = 2";
$header[12] = "\$ColorIndex4        = 3";
$header[13] = "\$Contrast1          = 15";
$header[14] = "\$Contrast2          = 0";
$header[15] = "\$Contrast3          = 15";
$header[16] = "\$Contrast4          = 0";
$header[17] = "\$ForceDisplay       = FALSE";
$header[18] = "\$FadeIn             = 0";
$header[19] = "\$FadeOut            = 0";
$header[20] = "\$TapeOffset         = TRUE";
$header[21] = "";

# read or create INI file
if (-e $inifile) {
  open (RH, $inifile) or die "Unable to read from INI file: $!";
  $section = "";
  INILineLoop: while (<RH>) {
    chomp;
    if (m/^\#/) { # comment line
      next INILineLoop;
    }
    if (m/^\[.*$\]/) { # header line
      $section = "";
      m/(^.)(.*)(.$)/; # strip off first and last characters
      ($left, $center, $right) = ($1, $2, $3);
      $center = uc $center;
      if (($center eq "SETTINGS") or ($center eq "HEADER")) {
        $section = $center;
        if ($section eq "HEADER") {
          @header = ();
        }
      }
      next INILineLoop;
    }
    if ($section eq "SETTINGS") {
      ($variable, $value) = split /=/;
      $variable = lc $variable;
      if ($variable eq "framerate") {
        if ($value == 1) {$value = 25;}
        if ($value == 2) {$value = 29.97;}
        if ($value == 3) {$value = 30 / 1.001;}
        $fps = $value;
      }
    }
    if ($section eq "HEADER") {
      push @header, $_;
    }
  }
  close RH;
} else {
  open (WH, ">".$inifile) or die "Unable to write to INI file: $!";
  print WH "[SETTINGS]\n";
  print WH "# framerate: 1 for 30 fps (PAL), \n";
  print WH "#            2 for 29.97 (NTSC drop-frame), \n";
  print WH "#            3 for 30 / 1.001 (NTSC nondrop-frame) \n";
  print WH "#   Any other value is actual frame rate.\n";
  $framerate = $fps;
  if ($fps == 25) {$framerate = 1;}
  if ($fps == 29.97) {$framerate = 2;}
  if ($fps == 30 / 1.001) {$framerate = 3;}
  printf WH "framerate=%d\n\n", $framerate;
  print WH "[HEADER]\n";
  print WH "# Spruce Subtitle Global Settings\n";
  for (@header) {
    printf WH "%s\n", $_;
  }
  intro();
}

# process command line arguments
while ($_ = shift) {
#  print ($_, "\n");
  $input = $_; 
}

$numdot = ($input =~ tr|\.|\.|);
if ($numdot == 0) {
  $inputfile = $input.".srt";
  $outputfile = $input.".stl";
} else {
  $inputfile = $input;
  $input =~ s/.srt/.stl/;
  $outputfile = $input;
  if ($inputfile eq $outputfile) {
    die "Input file must have .srt extension, $!";
  }
}
printf ("Converting %s to %s at %.5f frames per second...", 
 $inputfile, $outputfile, $fps);

open (RH, $inputfile) or die "Unable to read from SRT file: $!";
open (WH, ">".$outputfile) or die "Unable to write to STL file: $!";
for (@header) {
  printf WH "%s\n", $_;
}

$starttime = "";
$endtime = "";
$line = "";

readloop: while (<RH>) {
  chomp;
  if ($_ eq "") { # blank line means end of subtitle
    print WH $starttime.",".$endtime.",".$line."\n";
    $starttime = "";
    $endtime = "";
    $line = "";
    next readloop;
  }
  if (($starttime eq "") and (m/(..):(..):(..),(...) --> (..):(..):(..),(...)/)) {
   # grab times
    # split start and end times into hours, minutes, seconds, and thousandths
    ($sh, $sm, $ss, $st, $eh, $em, $es, $et) = ($1, $2, $3, $4, $5, $6, $7, $8);
    # convert thousandths into frames
    $sf = int(($st / 1000 * $fps) + 0.5);
    $ef = int(($et / 1000 * $fps) + 0.5);
    $starttime = sprintf ("%02d:%02d:%02d:%02d", $sh, $sm, $ss, $sf);
    $endtime = sprintf ("%02d:%02d:%02d:%02d", $eh, $em, $es, $ef);
    next readloop;
  }
  if ($starttime eq "") { # throw away subtitle count
    next readloop;
  }
  if ($line eq "") {
    $line = $_;
  } else { # "|" character in Maestro format means line break
    $line = $line."|".$_;
  }
}
close WH;
close RH;
print "done!\n";
exit;

sub intro {
  print "Welcome to Subrip2Maestro!\n\n";
  print "The file subrip2maestro.ini has just been created in this directory.\n";
  print "  This file sets the defaults for the conversion.  Modify it if you don't like\n";
  print "  the results and run again.\n\n";
  print "Some tips for customizing the INI and STL files:\n";
  print "* Put the characters ^B, ^I, or ^U around text for bold, italics, or underline.\n";
  print "* For the \$ColorIndex lines, \$ColorIndex1 controls the inner text outline,\n";
  print "   \$ColorIndex2 controls the outer text outline, \$ColorIndex3 controls the\n";
  print "   text itself, and \$ColorIndex4 controls the background.\n";
  print "* The values of the \$ColorIndex lines refer to a 16-color palette, which can be\n";
  print "   customized in DVDMaestro.  Default values are 0=Black, 1=Off-Black, 2=White,\n";
  print "   3=Red, 4=Gray, 5=Silver, 6=Aqua, 7=Fuschia, 8=Yellow, 9=Navy, 10=Green,\n";
  print "   11=Maroon, 12=Teal, 13=Purple, 14=Olive, 15=White\n";
  print "* The \$Contrast lines use the same order as the \$ColorIndex lines.  The values\n";
  print "   go from 0 (totally transparent) to 15 (totally opaque).\n";
  print "* \$TapeOffset refers to whether the subtitles go off of the timecodes in the\n";
  print "   MPEG file (TRUE), or if they are absolute to the start of the DVDMaestro\n";
  print "   timeline (FALSE).\n\n";
  return;
}
