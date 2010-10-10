#!/bin/zsh
echo "Testing NT version"

foreach xx ({1..40})
((xx = xx*128))
./run $xx 2 1
end
