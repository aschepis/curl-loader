

sub foo3(a,b,c)
    return aio_inc( b )
end

sub foo2(z,y)
    foo3( z * (z - 1), z, 3 * y)
end

sub foo(x)
    foo2( x - 1, x + 1 )
end

print( foo(3) )
