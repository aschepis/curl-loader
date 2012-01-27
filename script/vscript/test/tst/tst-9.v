

sub next(v)
    return v+1
end


var v = next(1)
print( v )

print( next(1) )

print( next( next(1) ) )


sub print_both( a, b )
  print(a)
  print(b)
end

print_both( next(1), next( next(1) ) )

