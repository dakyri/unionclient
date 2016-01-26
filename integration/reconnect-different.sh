if [[ -z "$UNION_HOME" ]] ; then
	UNION_HOME=/cygdrive/d/src/union/union_2.1.1/union-dev/union
fi
if [[ -z "$UNION_START_CMD" ]] ; then
	UNION_START_CMD=startserver.sh
fi
if [[ -z "$UNION_STOP_CMD" ]] ; then
	UNION_STOP_CMD=stopserver.sh
fi
kill_server_delay=5
restart_server_delay=10
export LD_PRELOAD="${UNION_HOME}/patch/bind_reuseaddr.so"
echo setting "LD_PRELOAD" "${UNION_HOME}/patch/bind_reuseaddr.so"
killall $UNION_START_CMD
killall java
(
	cd $UNION_HOME
	./$UNION_START_CMD >& union_output &
	echo "Starting server at " $UNION_HOME
	while true; do
		if grep -q "Started....OK" union_output; then
			break;
		fi
		if grep -q "Another instance of Derby may have already booted"  union_output; then
			break;
		fi
	done
)
(
	cd $UNION_HOME
	sleep $kill_server_delay
	echo "Stopping server at " $UNION_HOME
	./$UNION_STOP_CMD
	sleep $restart_server_delay
	echo "Restarting server at " $UNION_HOME
	./$UNION_START_CMD >& union_output &
	while true; do
		if grep -q "Started....OK" union_output; then
			break;
		fi
		if grep -q "java.net.BindException: Address already in use" union_output; then
			echo "Adress in use ... trying restart server again at " $UNION_HOME " in " $restart_server_delay " secs "
			sleep $restart_server_delay
			echo "Restarting now ..."
			./$UNION_START_CMD >& union_output &
		fi
		if grep -q "UNION SHUTTING DOWN" union_output; then
			echo "Server shutdown ... trying restart server again at " $UNION_HOME
			sleep 1
			./$UNION_START_CMD >& union_output &
		fi
		if grep -q "Another instance of Derby may have already booted"  union_output; then
			break;
		fi
	done
) &
if [[ "$#" -le 1 ]] ; then
	args="";
else
	args="$@"
fi 
echo "Running login Test " $UNION_TEST_PATH/LoginTest $args
./TisztaTeszta -retries 1 $args
(
	echo "Stopping server ..."
	cd $UNION_HOME
	pwd
	./$UNION_STOP_CMD
)
