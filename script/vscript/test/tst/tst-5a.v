
var %h

h{ "a" } = "another string"
h{ "b" } = "before time"
h{ "c" } = "c is the best language"


print( h{"a"} .. " : " .. h {"c" } )


var i = 0
var @k = keys(h)

while i < length(k)
  print( "name: " .. k[i] .. " value: " .. h{ k[i] } )
  i = i + 1
end

k = values(h)

i = 0
while i < length(k)
  print( "value: " .. k[i] )
  i = i + 1
end








