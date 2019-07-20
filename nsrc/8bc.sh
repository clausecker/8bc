#!/bin/sh

# PDP-8 B compiler driver

if [ $# -ne 1 ] || [ "${1%.b}" = "$1" ]
then
	echo Usage: $0 source.b >&2
	exit 1
fi

palfile="${1%.b}.pal"
bcdir="`dirname $0`"

exec >"$palfile"

cat "$bcdir/brt.pal"
echo
"$bcdir/8bc1" <"$1" || exit $?

exec >/dev/null

pal "$palfile" || exit $?
