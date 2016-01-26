if [[ -z "$UNION_HOME" ]] ; then
	UNION_HOME=/cygdrive/d/src/union/union_2.1.1/union-dev/union
fi
if [[ -z "$UNION_START_CMD" ]] ; then
	UNION_START_CMD=startserver.sh
fi
if [[ -z "$UNION_STOP_CMD" ]] ; then
	UNION_STOP_CMD=stopserver.sh
fi
(
	cd $UNION_HOME
	./$UNION_START_CMD >& union_output &
	echo "Starting server at " $UNION_HOME
	while true; do
		if grep -q "Started....OK" union_output; then
			break;
		fi
		if grep -q "Another instance of Derby may have already booted" ; then
			break;
		fi
	done
)
if [[ "$#" -le 1 ]] ; then
	args="badhost 9100";
else
	args="$@"
fi 
echo "Running failed connection Test " $UNION_TEST_PATH/ConnectTest $args
./ConnectSuccess $args
(
	echo "Stopping server ..."
	cd $UNION_HOME
	pwd
	./$UNION_STOP_CMD
)
