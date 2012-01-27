
sub fact(a)

  if a == 1 
    return 1
  end

  return a * fact(a-1)
end

sub fact_loop(a)
  var res = 1
  
  while a > 0
    res = res * a
    a = a - 1
  end
  return res
end

var v = fact(10)
print( v )

var v1 = fact_loop(10)
print( v1 )

//v = fact( fact(4) )
//print( v )

//print( fact(10) )


