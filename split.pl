# split.pl: Outputs alternate packets of bytes in 1 input file to 2 output files
# Run file without arguments to see usage
#
# Version 1.0
# McPoodle (mcpoodle43@yahoo.com)
#
# Version History
# 1.0 initial release

sub usage;

# initial variables
my $anything = "~";
my $input = "~";
my $headersize = 0;
my $chunksize = 1;
my $padding = "";
my $paddingHex = "~";

# process command line arguments
while ($_ = shift) {
#  print ($_, "\n");
  $anything = "";
  if (s/-h//) {
    $headersize = $_;
    next;
  }
  if (s/-s//) {
    $chunksize = $_;
    next;
  }
  if (s/-p//) {
    $paddingHex = $_;
    next;
  }
  if ($input =~ m\~\) {
    $input = $_;
    next;
  }
}

# print ("Header Size: ", $headersize);
# print ("Chunk Size: ", $chunksize);
# print ("\nPadding as Hex: ", $paddingHex);
# print ("\nInput: ", $input);

if ($anything eq "~") {
  usage();
  exit;
}

if ($input eq "~") {
  usage();
  die "No input file, stopped";
}

my $output1 = "~";
my $output2 = "~";

if (($input =~ m/(.*)(\.)(.*)$/i)) {
  $output1 = $1."_odd.".$3;
  $output2 = $1."_even.".$3;
} else {
  $output1 = $input."_odd";
  $output2 = $input."_even";
}

if ($output1 eq "~") {
  usage();
  die "Could not determine output file names from input file name; try renaming it";
}

$headersize = int($headersize);
if ($headersize < 0) {
  usage();
  die "Header size has illegal value, stopped";
}

$chunksize = int($chunksize);
if ($chunksize < 1) {
  usage();
  die "Chunk size has illegal value, stopped";
}

if ($paddingHex ne "~") {
  $padding = "";
  for (my $i = 0; $i < length($paddingHex); $i += 2) {
    my $chars = substr($paddingHex, $i, 2);
    my $byte = chr(hex $chars);
    $padding = $padding.$byte;
  }
}

sysopen(RH, $input, 0) or die "Unable to read from file: $!, stopped";
binmode RH;
open (WH1, ">".$output1) or die "Unable to write to file: $!";
binmode WH1;
open (WH2, ">".$output2) or die "Unable to write to file: $!";
binmode WH2;

my $data = "";
if ($headersize > 0) {
  sysread RH, $data, $headersize;
  syswrite WH1, $data;
  syswrite WH2, $data;
}

my $odd = 1;    # flag for which output file to use

# read loop
READLOOP: while (sysread RH, $data, $chunksize) {
  if ($odd) {
    syswrite WH1, $data.$padding;
  } else {
    syswrite WH2, $data.$padding;
  }
  $odd = 1 - $odd;
}
close WH2;
close WH1;
close RH;
exit;

sub usage {
  print "\nSPLIT Version 1.0\n";
  print "  Reads an input file and outputs alternate chunks to two output files.\n\n";
  print "  Syntax: SPLIT -h4 -s2 -p8080 infile.bin\n";
  print "    -h (OPTIONAL): number of bytes in header, which will be copied to both\n";
  print "         output files (DEFAULT: 0)\n";
  print "    -s (OPTIONAL): number of bytes in each chunk\n";
  print "         (DEFAULT: 1)\n";
  print "    -p (OPTIONAL): hexidecimal for bytes to output between each chunk\n";
  print "         (DEFAULT is null)\n";
  print "    infile.bin: name of file to process\n";
  print "    The two outfile names will be constructed from the infile name\n";
  print "      (infile_odd.bin and infile_even.bin for infile.bin)\n";
  print "    Outfiles will be overwritten if they exist\n\n";
}


