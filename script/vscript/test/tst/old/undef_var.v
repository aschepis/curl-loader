
goto undefinedlabel

var @kuku

kuku = 12


// same function is declared twice
sub hello(param)
 param = param + 1
end


var other

// function declaration conflicts with global variable
sub other()
   return 12
end

// calling function with different number of arguments
hello(1,2,3)

var a, c

a = 12

c = b

// b is not defined as left side of assignment
b = a

// b is not defined as right hande side of assignment 
c = a + b

var @arr

// arr defined as array but is used as hash
arr { "aaa" } = 333

arr = 3


// c defined as scalar but is used as array
c[3] = 444

// hello is used as variable but has been defined as a function
hello = 123