use strict;

my @a = (1,2,3,4);

my @b = @a;

push @a, 5;

print join(":",@a);

print "\n";

print join(":",@b);


