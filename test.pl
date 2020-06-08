sub asXDS {
  my $line = shift(@_);
  my @xdslist = (-1) x 32; # up to 32 values, using -1 for no value
  my $position = 4; # first four characters are "XDS "
  my $counter = 2; # steps through position in @xdslist
  my $class = substr $line, $position, 2;
  my $typeDefined = 1; # flag for undefined type
  SWITCH: for ($class) {
    /Cs/ && do {$xdslist[0] = 0x01; last SWITCH;}; # Current start
    /Cc/ && do {$xdslist[0] = 0x02; last SWITCH;}; # Current continue
    /Fs/ && do {$xdslist[0] = 0x03; last SWITCH;}; # Future start         
    /Fc/ && do {$xdslist[0] = 0x04; last SWITCH;}; # Future continue
    /Hs/ && do {$xdslist[0] = 0x05; last SWITCH;}; # Channel start
    /Hc/ && do {$xdslist[0] = 0x06; last SWITCH;}; # Channel continue
    /Ms/ && do {$xdslist[0] = 0x07; last SWITCH;}; # Miscellaneous start
    /Mc/ && do {$xdslist[0] = 0x08; last SWITCH;}; # Miscellaneous continue
    /Ps/ && do {$xdslist[0] = 0x09; last SWITCH;}; # Public service start
    /Pc/ && do {$xdslist[0] = 0x0a; last SWITCH;}; # Public service continue
    # the remaining classes have no defined types
    /Rs/ && do {$xdslist[0] = 0x0b; $typeDefined = 0; last SWITCH;}; # Reserved start
    /Rc/ && do {$xdslist[0] = 0x0c; $typeDefined = 0; last SWITCH;}; # Reserved continue
    /Us/ && do {$xdslist[0] = 0x0d; $typeDefined = 0; last SWITCH;}; # Undefined start
    /Uc/ && do {$xdslist[0] = 0x0e; $typeDefined = 0; last SWITCH;}; # Undefined continue
    # if none of the above, then there was a typo
    die "Invalid class code ".$class.", stopped";
  }
  $position += 3;
  my $type = substr $line, $position, 2;
  $position += 3;
  if ($class =~ /.c$/) {
    $position += $XDSPosition{$type};
  }
  my %MonthCode = ( 'Jan' => 1, 'Feb' => 2, 'Mar' => 3, 'Apr' => 4, 'May' => 5, 'Jun' => 6,
                    'Jul' => 7, 'Aug' => 8, 'Sep' => 9, 'Oct' => 10, 'Nov' => 11, 'Dec' => 12 );
  my %LanguageCode = ( 'Unknown' => 0, 'English' => 1, 'Español' => 2, 'Français' => 3, 
                       'Deutsch' => 4, 'Italiano' => 5, 'Other' => 6, 'None' => 7,
                       'Espanol' => 2, 'Francais' => 3 );
  SWITCH: for (substr $class, 0, 1) {
    /H/ && do {
      SWITCH2: for ($type) {
        /TS/ && do {                        # Transmission Signal Identifier
          $xdslist[1] = 0x04;
          if (substr($line, 10, 1) eq "\\") {
            last SWITCH2;
          }
          # digits are output in reverse order
          # this gets tricky, as the value could be split between packets
          my $digit;
          if ($position == 10) {
            $digit = substr $line, 13, 1;
            if ($digit ne "_") {
              # byte 2
              $xdslist[$counter++] = hex($digit) + 64;
            }
            $digit = substr $line, 12, 1;
            if ($digit ne "_") {
              # byte 3
              $xdslist[$counter++] = hex($digit) + 64;
            }
            $digit = substr $line, 11, 1;
            if ($digit ne "_") {
              # byte 4
              $xdslist[$counter++] = hex($digit) + 64;
            }
            $digit = substr $line, 10, 1;
            if ($digit ne "_") {
              # byte 5
              $xdslist[$counter++] = hex($digit) + 64;
              $position = 15;
            }
          }
          last SWITCH2;
        };
        # if you got here, then this is an undefined cHannel type
        $typeDefined = 0;
      }
      last SWITCH;
    };
  }
  # undefined types just have space-delimited hex data
  if ($typeDefined == 0) {
    for (; $position < (length ($line) - 4); $position += 3) {
      $xdslist[$counter++] = hex (substr $line, $position, 2);
    }
  }
  # at this point, @xdslist contains all the values to output and $counter is the next @xdslist index
  if ($counter % 2 == 1) {
    $xdslist[$counter++] = 0;
  }
  my $sum;
  if (substr($class, 1, 1) eq "s") {
    $sum = $xdslist[0] + $xdslist[1];
  } else {
    $sum = $XDSSum{$type};
  }
  for (my $i = 2; $i < $counter; $i++) {
    $sum += $xdslist[$i];
  }
  if (substr($line, $position - $XDSPosition{$type}, 1) eq "\\") {
    # 15 is the code to indicate the checksum is next
    $xdslist[$counter++] = 15;
    $sum += 15;
    # calculate checksum, the value necessary
    #  to make the sum evenly-divisible by 128
    $xdslist[$counter++] = 128 - ($sum % 128); 
    $XDSPosition{$type} = 0;
    $XDSSum{$type} = 0;
  } else {
    $XDSPosition{$type} = $position;
    $XDSSum{$type} = $sum;
  }
  # now to convert this list into a string of odd-parity hexidecimal pairs
  my $xdsOut = '';
  for (my $i = 0; $i < $counter; $i+= 2) {
    # print $xdslist[$i]." ".$xdslist[$i+1]." ";
    $xdsOut = $xdsOut.sprintf(" %02x%02x", oddParity($xdslist[$i]), oddParity($xdslist[$i+1]));
  }
  # print "\n";
  return (substr $xdsOut, 1, 90);
}

