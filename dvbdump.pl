# dvbdump.pl: Rip user data out of MPEG files (diagnostic for closed caption ripping)
# Run file without arguments to see usage
#
# Version 1.0
# McPoodle (mcpoodle43@yahoo.com)
#
# Version History
# 1.0 initial release

sub printdata;

# initial variables
$linelimit = 100;
$inputfile = "~";
$anything = "~";
$outputfile = "";

# process command line arguments
while ($_ = shift) {
#  print ($_, "\n");
  $anything = "";
  if (s/-n//) {
    $linelimit = $_;
    next;
  }
  if ($inputfile eq "~") {
    $inputfile = $_;
    next;
  }
}

# print ("\nLine Limit: ", $linelimit);
# print ("\nInput: ", $inputfile);

if ($anything eq "~") {
  usage();
  exit;
}

if ($inputfile eq "~") {
  usage();
  die "No input file, stopped";
}

$inputfile =~ m/(.+)\.(\w+)$/; # remove extension
($filebase, $skip) = ($1, $2);
if ($filebase eq "") {
  $filebase = $inputfile;
}
$outputfile = $filebase.".txt";
# print ("\nDiagnostic Output File: ", $outputfile);

print "Creating $outputfile";

open (WH, ">".$outputfile) or die "Unable to write to file $rawoutput: $!";

print WH "User Data Extract from $inputfile:\n";

sysopen(RH, $inputfile, O_RDONLY) or die "Unable to read from file: $!, stopped";
binmode RH;

$header = "\xff\xff\xff\xff";
$total = 0;
$linecount = 0;

HEADERLOOP: while (sysread (RH, $byte, 1)) {
  $header = substr($header, 1, 3).$byte;
  $total += 1;
  if ($total % 0x100000 == 0) {
    print ".";
  }
  if ($header ne "\x00\x00\x01\xb2") { # user data header
    next HEADERLOOP;
  }

  $linecount++;
  last HEADERLOOP if ($linecount > $linelimit);
  print WH "\n00 00 01 b2 ";
  $data = "\xff\xff\xff";
  READLOOP: while (sysread (RH, $byte, 1)) {
    printdata $byte;
    $data = substr($data, 1, 2).$byte;
    $total += 1;
    if ($total % 0x100000 == 0) {
      print ".";
    }
    if ($data eq "\x00\x00\x01") { # next packet
      $header = "\xff\xff\xff\xff";
      next HEADERLOOP;
    }
    next READLOOP;
  }
}
print "\n";
close RH;
close WH;
exit;

sub usage {
  print "\nDVBDUMP Version 1.0\n";
  print "  Rips user data from MPEG files.\n";
  print "    (used to figure out closed captions for DVB files.)\n\n";
  print "  Syntax: DVBDUMP -n100 infile.mpg\n";
  print "    -n (Optional): number of lines to output (default 100)\n";
  print "    infile.mpg: file to process\n";
  print "    outfile will have base name of infile plus .TXT\n\n";
}

sub printdata {
  my $data = shift(@_);
  my $out = sprintf "%02x ", ord $data;
  print WH $out;
}

