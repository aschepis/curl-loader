/*
    multiline comment
 */

var a,@b,%c

a = 2 + 3

print a

// array as left hand side
b[a] = 2 * 3

print b[a]


// simple case
a = b[ 0 ]

// array as right hand side
a = 1 + b[ a ]


// hash a left hand side
c{ "a" } = a

print c{ "a" }

// hash as right hand side
a = 1 + c{ "a" } 

print a

// more simple case

a = c{ "a" } 

print a
