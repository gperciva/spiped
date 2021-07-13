#!/bin/sh

# Goal of this test:
# - ensure that pushbits works

### Constants
c_valgrind_min=1

### Actual command
scenario_cmd() {
	# Copy a file
	setup_check_variables "test_pushbits 1"
	${c_valgrind_cmd} ${scriptdir}/pushbits/test_pushbits		\
		-i ${scriptdir}/lorem-send.txt -o ${s_basename}-copy 1
	echo $? > ${c_exitfile}

	setup_check_variables "test_pushbits 1 output"
	cmp -s ${scriptdir}/lorem-send.txt ${s_basename}-copy
	echo $? > ${c_exitfile}

	# Echo stdin to stdout
	setup_check_variables "test_pushbits 2"
	${c_valgrind_cmd} ${scriptdir}/pushbits/test_pushbits 2
	echo $? > ${c_exitfile}

	# Copy a message through one pushbits()
	setup_check_variables "test_pushbits 3"
	${c_valgrind_cmd} ${scriptdir}/pushbits/test_pushbits 3
	echo $? > ${c_exitfile}

	# Copy a message through two pushbits()
	setup_check_variables "test_pushbits 4"
	${c_valgrind_cmd} ${scriptdir}/pushbits/test_pushbits 4
	echo $? > ${c_exitfile}

	# Echo stdin to stdout, but cancel it immediately after starting
	setup_check_variables "test_pushbits 5"
	${c_valgrind_cmd} ${scriptdir}/pushbits/test_pushbits 5
	echo $? > ${c_exitfile}
}
