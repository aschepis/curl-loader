

var g = 1


g = g + 2

sub foo( arg )
    // function accesses global variable foo.
    g = g - arg

end 

foo(4)
