cmd=Assign1

# if the file ${cmd} exits then remove it
if [ -f ${cmd} ]; then
    rm ${cmd}
fi

#Compile the code
g++ -std=c++11 *.cpp -o ${cmd}

# if the file ${cmd} does not exit then exit the test
if [ ! -f ${cmd} ]; then
    echo -e "\033[1;91mCompile FAILED.\033[0m"
    exit
fi

# clean folder
for casenum in `seq 10 1 14`; do
    if [ -f my${casenum}.out ]; then
        rm my${casenum}.out
    fi
    if [ -f ${casenum}.stderr ]; then
        rm ${casenum}.stderr
    fi
    if [ -f ${casenum}.stdcout ]; then
        rm ${casenum}.stdcout
    fi
done

# Run the code
for casenum in `seq 10 1 14`; do
	./${cmd} < "input${casenum}.txt" > "myoutput${casenum}.txt"

	diff -iBwu output${casenum}.txt myoutput${casenum}.txt > ${casenum}.diff
    
	if [ $? -ne 0 ]; then

    		echo -e "Test case ${casenum}    \033[1;91mFAILED.\033[0m"
	else
    		echo -e "Test case ${casenum}    \033[1;92mPASSED.\033[0m"

    		rm -f ${casenum}.diff
            rm -f ${casenum}.stderr
            rm -f ${casenum}.stdcout
	fi
done
