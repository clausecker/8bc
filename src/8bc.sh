#!%shell%

# (c) 2019 Robert Clausecker <fuz@fuz.su>
# B compiler driver

progname=`basename $0`

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

# move $1 to the location specified in $ofile
mvout() {
	case "$ofile" in
	-) cat "$1"; rm "$1";;
	"") ;;
	*) mv "$1" "$ofile";;
	esac
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
if [ $# -ne 1 ]
then
	usage
fi

# if -k is provided, don't delete anything
if [ -z "$kflag" ]
then
	rmtmp="rm -f"
else
	rmtmp=true
fi

# use expr instead of ${1%.b} to work on old shells
#stem="`expr "$1" : "\(.*\).b"`"
stem="${1%.b}"
if [ "$stem.b" != "$1" ]
then
	echo "$progname": B source file required			>&2
	usage
fi

(
	cat "%brtloc%"
	echo
	"%bc1loc%" <"$1"
) >"$stem.pal"
status=$?

if [ $status -ne 0 ]
then
	$rmtmp "$stem.pal"
	exit $status
fi

if [ ! -z "$Sflag" ]
then
	mvout "$stem.pal"
	exit 0
fi

"%palloc%" "$stem.pal"
status=$?
$rmtmp "$stem.pal" "$stem.lst"

if [ $status -ne 0 ]
then
	$rmtmp "$stem.bin"
else
	mvout "$stem.bin"
fi

exit $status
