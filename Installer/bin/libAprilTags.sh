#! /bin/bash

# functions I need for AprilTags that could be used by the manager or the other script in bin

# get proc pid (0 notfound/error, else is the pid)
function getProcPID() {
    # NOTE: this returns the OLDEST PID
    paths=$(ps -aux | grep $1 | grep -v 'grep' | grep -v 'libAprilTags.sh pid') #last section removes command syntax for this command
    # the quotes preserve linebreaks
    # rewriting the for loop, since its done in a subshell and causes issues when written normally
    while read line; do
        pid=$(echo $line | awk '{ printf $2}')
        re='^[0-9]+$'
        if [[ $pid =~ $re ]]; then
            echo $pid
            return
        fi
    done <<< "$(echo "$paths")"
    echo 0
}

if [[ $1 == "pid" ]]; then
    getProcPID $2
fi
