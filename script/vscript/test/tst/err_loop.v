//PURPOSE: illegal break - not nested in while loop.

while  1
    break
end

// break must me nested in while
break


sub foo()

    while 1
	break
    end
    
    // same thing
    break
end
