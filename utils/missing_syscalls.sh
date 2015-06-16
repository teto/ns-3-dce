#!/bin/bash
# output the last K lines, instead of the last 10; or use -n +K to output lines starting with the Kth
# ./elfread ~/dce/build/bin/ntpd | tail -n +5 | wc -l 
if [ $# -lt 1 ]; then
	echo "Use: $0 <binary>"
	exit 1
fi

binary="$1"
tail -n +5 <(./elfread "$binary") | grep -Fxv -f <(./list_implemented_syscalls.sh)
