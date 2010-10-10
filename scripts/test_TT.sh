#!/bin/zsh
echo "Testing TT version"

foreach xx ({1..40})
((xx = xx*128))
./run $xx 3 1
end
