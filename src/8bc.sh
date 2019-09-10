#!/bin/sh

# (c) 2019 Robert Clausecker <fuz@fuz.su>
# B compiler driver

progname="$0"

usage() {
	echo Usage: "$progname" [-kSV] [-o file.bin] file.b	>&2
	echo " -k  keep temporary files"			>&2
	echo " -o  set output file name"			>&2
	echo " -S  do not assemble"				>&2
	echo " -V  print program version and exit"		>&2
	exit 2
}

version() {
	version=%version%
	[ -z "$version" ] && version='???'
	echo "8bc version $version"
	echo "(c) 2019 Robert Clausecker <fuz@fuz.su>"
	exit 0
}

kflag=
ofile=
Sflag=
while getopts ko:SV opt
do
	case $opt in
	k) kflag=1;;
	o) ofile="$OPTARG";;
	S) Sflag=1;;
	V) version;;
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
cat "%brtloc%"
echo
"%bc1loc%" <"$1"
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

"%palloc%" "$stem.pal"
status=$?
[ -z "$kflag" ] && rm -f "$stem.pal" "$stem.lst"

if [ $status -ne 0 ]
then
	rm -f "$stem.pal" "$stem.lst" "$stem.bin"
else
	[ ! -z "$ofile" ] && mv "$stem.bin" "$ofile"
fi

exit $status
