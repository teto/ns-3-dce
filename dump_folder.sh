#!/bin/sh
if [ $# -lt 1 ]; then
	echo "Use $0 <folder>"
	exit 1
fi


folder="$1"

echo '========= STDOUT' && cat $folder/stdout
echo '========= STDERR' && cat $folder/stderr
echo '========= SYSLOG' && cat $folder/syslog

