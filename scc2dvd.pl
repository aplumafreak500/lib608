#!"c:/perl/bin/perl.exe"
#!/usr/local/bin/perl

sub usage;
sub frame;
sub printdata;

# initial variables
$starttime = "00:00:00:00";
$fps = 30000/1001; # non-drop NTSC framerate
$output = "~"; # place-holder for no output file yet
$input = "~";

# process command line arguments
while ($_ = shift) {
#  print ($_, "\n");
  if (s/-s//) {
    $starttime = $_;
    next;
  }
  if (s/-f//) {
    $fps = $_;
    next;
  }
  if ($input =~ m\~\) {
    $input = $_;
    next;
  }
  $output = $_;
}

# print ("Start time: ", $starttime);
# print ("\nFPS: ", $fps);
# print ("\nInput: ", $input);
# print ("\nOutput: ", $output);

if ($output eq "~") {
  usage();
  die "No output file, stopped";
}

if ($input eq "~") {
  usage();
  die "No input file, stopped";
}

if (($fps < 12)||($fps > 60)) {
  usage();
  die "Frames per second out of range, stopped";
}

if (($starttime =~ m/\d\d:\d\d:\d\d:\d\d/) != 1) {
  usage();
  die "Wrong format for start time, stopped";
}

$currentframe = frame($starttime);

open (WH, ">".$output) or die "Unable to write to file: $!";

open (RH, $input) or die "Unable to read from file: $!";
$header = <RH>;
chomp $header;
($formattype, $formatversion) = split (/ /, $header);
if ($formattype ne "Scenarist_SCC") {
  die "Not SCC format, stopped";
}
$skip = <RH>; # skip blank line after header
$loopcount = 1; # loop 1: 5 elements, loop 2: 8 elements, loop 3: 11 elements, rest: 15 elements
$datacount = 0; # counts within loop
# read loop
LINELOOP: while (<RH>) {
  if ($_ eq "\n") {
    next LINELOOP;
  }
  m/^(..:..:..:..)(\s)(.+)/; # split into timecode and rest of line
  ($timecode, $skip, $line) = ($1, $2, $3);
  $nextframe = frame($timecode);
  $fillframes = int($nextframe - $currentframe + 0.5);
  if ($fillframes < 0) {
    $fillframes = 0;
  }
  while ($fillframes) {
    --$fillframes;
    printdata "\x80\x80";
  }
  $currentframe = $nextframe;
  (@words) = split (/ /, $line);
  WORDLOOP: foreach $word (@words) {
    $word =~ m/(..)(..)/;
    $hi = hex $1;
    $lo = hex $2;
    printdata chr($hi).chr($lo);
    ++$currentframe;
    next WORDLOOP;
  }
  next LINELOOP;
}
close RH;
close WH;
exit;

sub usage {
  print "\n\nSCC2DVD: Converts Scenarist inport format to NTSC Closed-Caption DVD binary data.\n\n";
  print "  Syntax: SCC2DVD -s01:00:00:00 -f25 infile.scc outfile.bin\n";
  print "    -s: Timecode of start point, in HH:MM:SS:FF format\n";
  print "         (DEFAULT: 00:00:00:00)\n";
  print "    -f: number of frames per second (range 12 - 60)\n";
  print "         (DEFAULT: 29.97)\n";
  print "    outfile will be overwritten if it exists\n\n";
}

sub frame {
  my $timecode = shift(@_);
  ($hh, $mm, $ss, $ff) = split(m/:/, $timecode, 4);
  my $framecount = ($hh * 3600) + ($mm * 60) + $ss;
  $framecount *= $fps;
  $framecount += $ff;
  return $framecount;
}

sub printdata {
  my $data = shift(@_);
  if ($datacount == 0) {
    print WH "\x00\x00\x01\xb2\x43\x43\x01\xf8";
    if ($loopcount == 1) {
      print WH "\x8a";
    }
    if ($loopcount == 2) {
      print WH "\x8f";
    }
    if ($loopcount == 3) {
      print WH "\x16\xfe\x00\x00";
    }
    if ($loopcount > 3) {
      print WH "\x1e\xfe\x00\x00";
    }
  }
  ++$datacount;
  print WH "\xff".$data;
  if ((($loopcount == 1) and ($datacount < 5)) or (($loopcount == 2) and ($datacount < 8)) or
      (($loopcount == 3) and ($datacount < 11)) or (($loopcount > 3) and ($datacount < 15))) {
    print WH "\xfe\x00\x00";
  } else {
    if ($loopcount == 1) {
      print WH "\xfe\x00\x00";
    }
    ++$loopcount;
    $datacount = 0;
  }
}
