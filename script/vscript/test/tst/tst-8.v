sub fib(a)
  if a <= 1 
    return a
  else 
    return fib(a-1) + fib(a-2)
  end
end


print( fib(5) )

var a = 3

while a < 10
  print( fib(a) )
  a = a + 1
end  

