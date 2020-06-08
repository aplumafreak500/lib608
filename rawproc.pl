# rawproc.pl: Perform various adjustments to raw closed caption files
# Run file without arguments to see usage
#
use strict;
my $Version = "1.2";
# McPoodle (mcpoodle43@yahoo.com)
#
# Version History
# 1.0 initial release
# 1.1 added processing of raw DVD closed caption files
#     (replacing ffff with 0000).
# 1.2 fixed XDS exclusion

sub usage;

# initial variables
my @keepPO = (0, 0, 0, 0, 0); # keep Pop-On captions for channels 1 - 4
my @keepPA = (0, 0, 0, 0, 0); # keep Paint-On captions
my @keepRU = (0, 0, 0, 0, 0); # keep Roll-Up captions
my @keepT  = (0, 0, 0, 0, 0); # keep Text service
my $keepXDS = 0;
my $channelUsage = "~"; # placeholder to see if any usage commands are made
my $input = "~"; # place-holder for no input file yet
my $output = "~"; # place-holder for no output file yet
my $anything = "~";

# process command line arguments
while ($_ = shift) {
#  print ($_, "\n");
  $anything = "";
  if (s/-c//) {
    $channelUsage = $_;
    if (s/ITV//) {
      $keepT[2] = 1;
    }
    if (s/XDS//) {
      $keepXDS = 1;
    }
    if (s/PO//) {
      if (s/1//) {
        $keepPO[1] = 1;
      }
      if (s/2//) {
        $keepPO[2] = 1;
      }
      if (s/3//) {
        $keepPO[3] = 1;
      }
      if (s/4//) {
        $keepPO[4] = 1;
      }
      if (join($", @keepPO) eq "0 0 0 0 0") {
        @keepPO = (0, 1, 1, 1, 1);
      }
    }
    if (s/PA//) {
      if (s/1//) {
        $keepPA[1] = 1;
      }
      if (s/2//) {
        $keepPA[2] = 1;
      }
      if (s/3//) {
        $keepPA[3] = 1;
      }
      if (s/4//) {
        $keepPA[4] = 1;
      }
      if (join($", @keepPA) eq "0 0 0 0 0") {
        @keepPA = (0, 1, 1, 1, 1);
      }
    }
    if (s/RU//) {
      if (s/1//) {
        $keepRU[1] = 1;
      }
      if (s/2//) {
        $keepRU[2] = 1;
      }
      if (s/3//) {
        $keepRU[3] = 1;
      }
      if (s/4//) {
        $keepRU[4] = 1;
      }
      if (join($", @keepRU) eq "0 0 0 0 0") {
        @keepRU = (0, 1, 1, 1, 1);
      }
    }
    if (s/CC//) {
      if (s/1//) {
        $keepPO[1] = 1;
        $keepPA[1] = 1;
        $keepRU[1] = 1;
      }
      if (s/2//) {
        $keepPO[2] = 1;
        $keepPA[2] = 1;
        $keepRU[2] = 1;
      }
      if (s/3//) {
        $keepPO[3] = 1;
        $keepPA[3] = 1;
        $keepRU[3] = 1;
      }
      if (s/4//) {
        $keepPO[4] = 1;
        $keepPA[4] = 1;
        $keepRU[4] = 1;
      }
      if (join($", @keepPO) eq "0 0 0 0 0") {
        @keepPO = (0, 1, 1, 1, 1);
        @keepPA = (0, 1, 1, 1, 1);
        @keepRU = (0, 1, 1, 1, 1);
      }
    }
    if (s/T//) {
      if (s/1//) {
        $keepT[1] = 1;
      }
      if (s/2//) {
        $keepT[2] = 1;
      }
      if (s/3//) {
        $keepT[3] = 1;
      }
      if (s/4//) {
        $keepT[4] = 1;
      }
      if (join($", @keepT) eq "0 0 0 0 0") {
        @keepT = (0, 1, 1, 1, 1);
      }
    }
    if (s/1//) {
      $keepPO[1] = 1;
      $keepPA[1] = 1;
      $keepRU[1] = 1;
      $keepT[1] = 1;
    }
    if (s/2//) {
      $keepPO[2] = 1;
      $keepPA[2] = 1;
      $keepRU[2] = 1;
      $keepT[2] = 1;
    }
    if (s/3//) {
      $keepPO[3] = 1;
      $keepPA[3] = 1;
      $keepRU[3] = 1;
      $keepT[3] = 1;
      $keepXDS = 1;
    }
    if (s/4//) {
      $keepPO[4] = 1;
      $keepPA[4] = 1;
      $keepRU[4] = 1;
      $keepT[4] = 1;
    }
    if (m/.+/) {
      die "Unknown channel usage $channelUsage, stopped";
    }
    next;
  }
  if ($input =~ m\~\) {
    $input = $_;
    next;
  }
  $output = $_; 
}

#print ("\nLast Channel Usage: ", $channelUsage);
#print ("\nInput: ", $input);
#print ("\nOutput: ", $output);
#print ("\nPop-On Usage: @keepPO");
#print ("\nPaint-Over Usage: @keepPA");
#print ("\nRoll-Up Usage: @keepRU");
#print ("\nText Usage: @keepT");
#print ("\nXDS Usage: ", $keepXDS);

if ($anything eq "~") {
  usage();
  exit;
}

if ($channelUsage eq "~") {
  @keepPO = (0, 1, 1, 1, 1);
  @keepPA = (0, 1, 1, 1, 1);
  @keepRU = (0, 1, 1, 1, 1);
  @keepT  = (0, 1, 1, 1, 1);
  $keepXDS = 1;
  $channelUsage = "1234";
}

if ($input eq "~") {
  usage();
  die "No input file, stopped";
}

if ($output eq "~") {
  if (($input =~ m/(.*)\.(.*)$/i)) {
    $output = $1."_2.".$2;
  }
}
if ($output eq "~") {
  usage();
  die "Could not determine output file name, stopped";
}

if ($input eq $output) {
  die "Input and output files cannot be the same, stopped";
}

my $data;

sysopen(RH, $input, 0) or die "Unable to read from $input: $!, stopped";
binmode RH;

sysread RH, $data, 4; # grab file header (hopefully)

if ($data eq "\x00\x00\x01\xb2") {
  print "\nInput file in DVD format--replacing ffff with 0000.\n";
  open (WH, ">".$output.".1") or die "Unable to write to $output: $!";
  binmode WH;
  print WH $data;
  # pass 1: replace fe ff ff with fe 00 00
DVDLOOP1: while (sysread RH, $data, 1) {
  if ($data ne "\xfe") {
    print WH $data;
    next DVDLOOP1;
  }
  if ((sysread RH, $data, 2) != 2) {
    print WH "\xfe";
    last DVDLOOP1;
  }
  if ($data eq "\xff\xff") {
    print WH "\xfe\x00\x00";
  } else {
    print WH "\xfe", $data;
  }
}
  close WH;
  close RH;
  
  sysopen(RH, $output.".1", 0) or die "Unable to read from $output: $!, stopped";
  binmode RH;
  open (WH, ">".$output) or die "Unable to write to $output: $!";
  binmode WH;

  # pass 2: replace ff ff ff with ff 00 00
DVDLOOP2: while (sysread RH, $data, 1) {
  if ($data ne "\xff") {
    print WH $data;
    next DVDLOOP2;
  }
  if ((sysread RH, $data, 2) != 2) {
    print WH "\xff";
    last DVDLOOP2;
  }
  if ($data eq "\xff\xff") {
    print WH "\xff\x00\x00";
  } else {
    print WH "\xff", $data;
  }
}
  close WH;
  close RH;
  exit;
}

open (WH, ">".$output) or die "Unable to write to $output: $!";
binmode WH;

print "\nWriting to file $output.\n";
my $header = "~";
if ($data eq "\xff\xff\xff\xff") {
  $header = "";
}
if (($header eq "~") && ($data =~ "^\xff\xff\xff")) {
  $header = "\xff";
}
if (($header eq "~") && ($data =~ "^\xff\xff")) {
  $header = "\xff\xff";
}
if (($header eq "~") && ($data =~ "^\xff")) {
  $header = "\xff\xff\xff";
}
if ($header eq "~") {
  $header = "\xff\xff\xff\xff";
}
if ($header."~" ne "~") {
  print "\nFixed header.\n";
}
print WH $header, $data;

my $channel = 3; # current channel (default to XDS Channel 3)
my $mode = "XDS"; # current mode (PO, PA, RU, T or XDS)
my @foundPO = (0, 0, 0, 0, 0);
my @foundPA = (0, 0, 0, 0, 0);
my @foundRU = (0, 0, 0, 0, 0);
my @foundT = (0, 0, 0, 0, 0);
my $foundXDS = 0;
my $foundITV = 0;

# read loop
READLOOP: while (sysread RH, $data, 2) {
  if ($data eq "\x80\x80") {
    print WH $data;
    next READLOOP;
  }
  $data =~ m/(.)(.)/;
  my $hi = ord $1;
  my $lo = ord $2;
  # remove any odd parity for testing
  if ($hi > 127) {$hi -= 128;}
  if ($lo > 127) {$lo -= 128;}
  
  if (($hi == 0x14) || ($hi == 0x15) || ($hi == 0x1c) || ($hi == 0x1d)) {
    if (($lo >= 0x20) && ($lo < 0x30)) {
      if ($hi == 0x14) { $channel = 1; }
      if ($hi == 0x1c) { $channel = 2; }
      if ($hi == 0x15) { $channel = 3; }
      if ($hi == 0x1d) { $channel = 4; }
      
      if ($lo == 0x20) { $mode = "PO"; $foundPO[$channel] = 1; } # {RCL}
      # 0x21 - 0x24 are {BS}, {AON}, {AOF}, and {DER}
      if (($lo == 0x25) || ($lo == 0x26) || ($lo == 0x27)) {
        # printf "%02x %02x ", $hi, $lo;
        $mode = "RU"; # {RU2}, {RU3}, {RU4}
        $foundRU[$channel] = 1;
      }
      # 0x28 is {FON}
      if ($lo == 0x29) { $mode = "PA"; $foundPA[$channel] = 1; } # {RDC}
      if (($lo == 0x2a) || ($lo == 0x2b)) {
        $mode = "T"; # {TR}, {RTD}
        if ($channel == 2) {
          $foundITV = 1;
        } else {
          $foundT[$channel] = 1;
        }
      }
      # 0x2c - 0x2f are {EDM}, {CR}, {ENM}, and {EOC}
    }
  }
  if ($hi < 0x10) {
    $mode = "XDS";
    $channel = 3;
    $foundXDS = 1;
  }
  
  if ($mode eq "PO") {
    if ($keepPO[$channel] == 0) { $data = "\x80\x80"; }
  }
  if ($mode eq "PA") {
    if ($keepPA[$channel] == 0) { $data = "\x80\x80"; }
  }
  if ($mode eq "RU") {
    if ($keepRU[$channel] == 0) { $data = "\x80\x80"; }
  }
  if ($mode eq "T") {
    if ($keepT[$channel] == 0) { $data = "\x80\x80"; }
  }
  if ($mode eq "XDS") {
    if ($keepXDS == 0) { $data = "\x80\x80"; }
  }

  print WH $data;
}

close WH;
close RH;
for ( $channel = 1; $channel < 5; $channel++ ) {
  if ($foundPO[$channel]) {
    print "\nFound ";
    if (!$keepPO[$channel]) { print "and excluded "; }
    print "Pop-On captions on Channel $channel.";
  }
  if ($foundPA[$channel]) {
    print "\nFound ";
    if (!$keepPA[$channel]) { print "and excluded "; }
    print "Paint-On captions on Channel $channel.";
  }
  if ($foundRU[$channel]) {
    print "\nFound ";
    if (!$keepRU[$channel]) { print "and excluded "; }
    print "Roll-Over captions on Channel $channel.";
  }
  if ($foundT[$channel]) {
    print "\nFound ";
    if (!$keepT[$channel]) { print "and excluded "; }
    print "Text Service on Channel $channel.";
  }
  if (($channel == 2) && ($foundITV)) {
    print "\nFound ";
    if (!$keepT[2]) { print "and excluded "; }
    print "ITV Service on Channel 2.";
  }
  if (($channel == 3) && ($foundXDS)) {
    print "\nFound ";
    if (!$keepXDS) { print "and excluded "; }
    print "XDS Service on Channel 3.";
  }
}

exit;

sub usage {
  printf "\nRAWPROC Version %s\n", $Version;
  print "  Fixes the header of raw closed caption files and performs various other fixes\n\n";
  print "  Syntax: RAWPROC -c1 -cCC2 infile.bin outfile.bin\n";
  print "    -c (OPTIONAL): output a channel (not using -c is the same as -c1234)\n";
  print "      Examples of -c arguments:\n";
  print "      -c1234: output everything on all four channels\n";
  print "      -c1: output everything on Channel 1\n";
  print "      -cCC1: output the closed captions on channel 1\n";
  print "      -cT: output the text services on all channels\n";
  print "        (CC, T, PO [Pop-On Captions], PA [Paint-On Captions], \n";
  print "         RU [Roll-Up Captions])\n";
  print "      -cITV: output the ITV service (same as -cT2)\n";
  print "      -cXDS: output the XDS service on Channel 3\n";
  print "    The -c argument currently only works with raw broadcast CC files\n";
  print "    Outfile argument is optional and is assumed to be infile_2\n";
  print "      with the extension of the input file\n";
  print "    Outfile will be overwritten if it exists\n\n";
}

