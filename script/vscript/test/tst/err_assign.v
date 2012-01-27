//PURPOSE: this code does type check errors 

// declares a as array, b as hash, c as scalar
var a,@b,%c

// ERROR: type check error, 
a = b

// ERROR: type check error,
a = c

// ERROR: type check error,
b = c
