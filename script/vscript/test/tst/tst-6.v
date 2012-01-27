
var res

// TODO: compiler - if function returns a value that is not used, then release it.

sub foo( a, b, c)
    print( 123 )
    print( a + b )
    print( a )
    print( b )
    print( c )

    return c
end

res = foo( 1, 2, 3 + 4 * 5 )


