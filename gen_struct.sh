#!/bin/sh
name="$1"
ret="$2"

shift 2
signature="$@"

echo "dce_${name}=$name;" >> imports.c
echo "$ret (*dce_${name})$signature;" >> imports.h

