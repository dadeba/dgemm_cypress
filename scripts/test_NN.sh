#!/bin/zsh
echo "Testing NN version"

foreach xx ({1..40})
((xx = xx*128))
./run $xx 1 1
end
