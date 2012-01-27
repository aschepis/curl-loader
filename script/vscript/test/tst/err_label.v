
// jump to undefined label (is defined as function)
goto hello

abc:

// ok: backward reference of label
goto abc

// ok: forward reference of label
goto ddd

ddd:

sub hello(param)
 

  abc2:

  // ok: backward reference of label
  goto abc2
 
  // ok: forward reference of label
  goto ddd2

  ddd2:

  // error
  goto undefinedlabel2

  // error
  goto undefinedlabel

  // error: can't jump out of local scope.
  goto ddd
end

