use strict;

my %hash;
my @arr;


$hash{ @arr } = "!!!";

print $hash{ @arr };


$hash{ 1 } = "1111";

$hash{ "1" } = "2222";

print $hash{ 1 };

print "\n";

print $hash{ "1" };

print "\n ... \n";


my @arr;

$arr[10]=10;
$arr[20]=20;

print $#arr."\n";

print ":$arr[0]\n";
print ":$arr[10]\n";
print ":$arr[20]\n";


print "[ @arr ]";
