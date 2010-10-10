#!/bin/zsh

./test_NN.sh | tee log_NN
./test_TN.sh | tee log_TN
./test_NT.sh | tee log_NT
./test_TT.sh | tee log_TT
