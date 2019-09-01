#!/bin/sh

# B compiler driver

progname="$0"
bcdir="`dirname $0`"
bc1="$bcdir/8bc1"
brt="$bcdir/brt.pal"

usage() {
	echo Usage: "$progname" [-S] [-o file.bin] file.b	>&2
	echo " -S  do not assemble"				>&2
	echo " -o  set output file name"			>&2
	exit 2
}

Sflag=
ofile=
while getopts So: opt
do
	case $opt in
	S) Sflag=1;;
	o) ofile="$OPTARG";;
	?) usage;;
	esac
done

shift $((OPTIND-1))
[ $# -eq 1 ] || usage

stem="${1%.b}"
if [ "$stem.b" != "$1" ]
then
	echo "$1": B source file required			>&2
	usage
fi

exec >"$stem.pal"
cat "$brt"
echo
"$bc1" <"$1"
status=$?
exec >/dev/null

if [ $status -ne 0 ]
then
	rm -f "$stem.pal"
	exit $status
fi

if [ ! -z "$Sflag" ]
then
	[ ! -z "$ofile" ] && mv "$stem.pal" "$ofile"
	exit 0
fi

pal "$stem.pal"
status=$?
rm -f "$stem.pal" "$stem.lst"

if [ $status -ne 0 ]
then
	rm -f "$stem.bin"
else
	[ ! -z "$ofile" ] && mv "$stem.bin" "$ofile"
fi

exit $status
