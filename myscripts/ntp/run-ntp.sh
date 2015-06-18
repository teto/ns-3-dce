#!/bin/bash
path=$(dirname "${BASH_SOURCE:-$0}")
#echo 'Can I delete files-0 and files-1 directories ? (y/n)'
#read rep
rep='y'
if [ $rep == 'y' ]
then
	rm -Rvf files-0 files-1
fi
mkdir -p "files-0" "files-1"
#echo "copy files to node 0 (client)"
cp -v "$path/chronyclient.conf" "$path/ntpclient.conf" files-0/
#echo copy server configurations to node 1

cp -v  "$path/ntpserver.conf" "$path/chronyserver.conf" files-1/
#cp -v  myscripts/ntp/chrony.conf files-1/
#echo run scenario dce-ntpd
# ../../waf --run dce-ntpd

#echo ls -l files-1
#ls -l files-1


