

class Tst 

  attr_accessor  :val

  def initialize(value)
    @val = value
  end  

end


t = Tst.new(12)

print t.val

y = t

y.val = 11

print y.val

print t.val


print "###\n"

a = 1
b = a
b = 2

print a
print b


print "###\n"

a = "aaa"
b = a
b = "bbb"

print a
print b


