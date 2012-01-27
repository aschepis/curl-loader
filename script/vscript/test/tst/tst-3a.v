var i,b

i = 0
b = 2
do
   if i % 2 == 0 
     i = i + 1
     continue
   end
   
   print(i)

   i = i + 1
   b = b * 2
until i < 7

print(i)
print(b)

