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
		if grep -q "Another instance of Derby may have already booted"  union_output; then
			break;
		fi
	done
)
if [[ "$#" -le 1 ]] ; then
	args="-p XXXXPXJvb21uYW1lJmNhbl9lZGl0PTEmdXNlcm5hbWU9dXNlcm5hbWUmbmFtZT1QcmV6aStNb25pdG9yJnV0Yz0xMzE3NjkzMzk5LjAxJnJvb21fZW1wdHk9MQ== -s pYlX7CA+nNLNr3kvD9ksimoewXc=";
else
	args="$@"
fi 
echo "Running failed login test, expired policy" $UNION_TEST_PATH/LoginTest $args
./LoginSuccess $args
(
	echo "Stopping server ..."
	cd $UNION_HOME
	pwd
	./$UNION_STOP_CMD
)
