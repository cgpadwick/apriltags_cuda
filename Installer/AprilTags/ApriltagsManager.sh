#! /bin/bash

if [ $# -ne 1 ]; then
    echo "only run with one argument"
    exit 1
fi

if [ ! -f '/apps/AprilTags/Backend/ws_server' ]; then
    echo "missing backend"
    exit 2
fi

function killIfRunning() {
    PID=`libAprilTags.sh pid $1`
    if [ $PID -ne 0 ]; then
        kill -2 $PID
    fi
}

# set clocks to max frequency (jetson only)
jetson_clocks || true

#open source for args
source /apps/AprilTags/args

if [[ $1 == "start" ]]; then
    # check for lock
    if [ -f "/apps/AprilTags/servicerunning" ]; then
        echo "service is already running"
        echo "HINT: if you think the service is not running, then run 'AprilTags.sh --validate lockfile'"
        # now check if it is allowed to remove the lockfile..
        if [[ $autormlockfile == "true" ]]; then
        	# check for if the services are fine
        	status=$(/apps/bin/AprilTags.sh -V lockfile)
        	lockfilestatus=$(echo $status | awk -F ";" '{print $1}' | awk -F "=" '{print $2}')
        	if [[ $lockfilestatus == "false" ]]; then
        		# the lockfile is gone, and nothing is wrong
        		echo "removed old lockfile"
        	else
        		echo "the service is still running... exiting"
        		echo "Specific status: ${lockfilestatus}"
        		exit 4
        	fi
        else
        	# either unable to remove the lock or it is running already
        	exit 4
        fi
    fi

    # to fix some odd pathing things
    cd /apps/AprilTags

    touch /apps/AprilTags/servicerunning

    # abs path BECAUSE of the proc getting commands
    /apps/AprilTags/Backend/ws_server $backend &
    
    exit

elif [[ $1 == "stop" ]]; then
    rm /apps/AprilTags/servicerunning
    # and add the rest of it
    killIfRunning '/apps/AprilTags/Backend/ws_server'

else
    echo "input ${1} not understood"
    exit 3
fi
