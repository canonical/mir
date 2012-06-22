#!/bin/bash


#add a executable name from the top-level directory, and 
#it will be ran while running all the tests
test_array=(
		unit-tests/compositor/test-compositor
		tests/test_renderloop
		tests/test_input_dispatch
	   )


number_failed=0
number_not_found=0
for i in ${test_array[*]}
do
	tput bold
	tput setaf 3
	echo "test_all executing: " $i
	tput sgr0
	#execute test here
	if [ -e ${i} ]; then
		$i

		if [ $? -ne 0 ]; then
			number_failed=$(($number_failed + 1))
		fi
        else
		number_not_found=$(($number_not_found + 1))
	fi 

done

tput bold
tput setaf 3
if [ $number_not_found -gt 0 ]; then
	echo "test_all done numnnot: " $number_not_found " tests"
fi
echo "test_all done, failed: " $number_failed " tests"
tput sgr0
