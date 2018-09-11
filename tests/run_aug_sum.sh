#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

LARGE=$1

printf "${GREEN}test aug_sum${NC}\n"
echo "(Results in Table 3)"

rm res.txt
echo $LARGE

printf "${BLUE}parallel run${NC}\n"
if [[ $LARGE == 1 ]]
then
	./runall -r 7 -l
else
	./runall -r 7
fi

echo
printf "${BLUE}sequential run${NC}\n"
if [[ $LARGE == 1 ]]
then
	./runall -r 3 -p 1 -l
else
	./runall -r 3 -p 1
fi

python comp.py
