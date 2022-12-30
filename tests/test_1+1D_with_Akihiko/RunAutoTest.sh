#!/usr/bin/env bash

cd EoS; rm -fr neos_2; tar -xf neos_2.tar.gz; cd ..

./MUSIChydro tests/test_1+1D_with_Akihiko/music_input_test_baryon_diffusion

python3 tests/test_1+1D_with_Akihiko/TestOutputFiles.py
STATUS=$?

if [ $STATUS == 0 ]; then
    echo "All tests passed! :)"
    rm -fr ./*.dat
else
    echo "Tests FAILED :("
    exit 1
fi
