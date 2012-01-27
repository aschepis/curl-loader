



use strict;

sub test
{
  push(@_, "aaa");
}

my @arr = (1,2,3);

test(@arr);

print join(" : ",@arr)."\n";

my $kkk="kkk";

print $kkk + 3;
print "\n";

my @a=(1,2,3,4);
my %aa = { "aa"=>1, "bb" => 2 };

print @a + 3;
print "\n***\n";

print scalar(@a);
print "\n";
print scalar(%aa);
print "\n";

#my $a="1 bc ";
#my $b="2 de ";
#
#print $a + $b;

## output is 3

#print "\n";

#sub test
#{
#    print "hello\n";
#}
#
#my $test = 1;
#
#print test();
## very strange: output is hello\n1


#my $b="c";
#my @b=("d","e");
#
#print "\n".$b;
