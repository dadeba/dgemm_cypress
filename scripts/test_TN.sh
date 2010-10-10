#!/bin/zsh
echo "Testing TN version"

foreach xx ({1..40})
((xx = xx*128))
./run $xx 0 1
end
