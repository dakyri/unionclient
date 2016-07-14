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
	./$UNION_STOP_CMD
	./$UNION_START_CMD >& union_output &
	while true; do
		if grep -q "Started....OK" union_output; then
			break;
		fi
	done
)
echo "Running connection Test " $UNION_TEST_PATH/TisztaTeszta
./TisztaTeszta -c
(
	cd $UNION_HOME
	pwd
	./$UNION_STOP_CMD
)
