#!/usr/bin/perl


sub clean
{
  my $dir = $_[0]; 

  print "Cleaning directory $dir\n";

  `rm $dir/*.err`;
  `rm $dir/*.out`;
  `rm $dir/*.xml`; 
  `rm $dir/*.lst`; 
  `rm $dir/*.lst2`; 
  `rm $dir/*.vout`; 
}



clean("tst") if ($ARGV[0] eq "all");
clean("out");
