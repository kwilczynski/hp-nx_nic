#! /bin/bash
# $1 = system pcitable file to update
# $2 = file with new entries in pci.ids file format
# $3 = pcitable output file
# $4 = driver name for use in pcitable file

if [ $# != 4 ]; then
	echo "usage : $0 <system pcitable> <pci.updates> <pcitable.new> <driver name>"
	exit 1
fi

if [ ! -f $1 -a ! -L $1 ]; then
	echo "The source pcitable file ($1) does not exist or is not a file."
	exit 2
fi

if [ ! -f $2 ]; then
	echo "The source pci.updates file ($2) does not exist."
	exit 3
fi

if [ -e $3 ]; then
	echo "The destination pcitable filename ($3) already exists."
	exit 4
fi

exec 0<$1
exec 1>$3
exec 3<$2

if [ -z "$4" ]; then
	echo "Need driver name."
	exit 0
fi

if [ "`grep -ic taroon /etc/redhat-release`" != "0" ]; then
	oldformat=1
fi

driver=$4
IFS=

# pattern matching strings
ID="[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]]"
DEV="	${ID}*"
TABLE_DEV="0x${ID}	0x${ID}	\"*"

line=
table_line=
table_in=
device=

# get the first line of the pcitable file to jump start things
read -r table_in

# outer loop reads lines from the updates file
while read -r -u 3 line; do

	# device entry
	if [[ $line == $DEV ]]; then

		device=0x${line:1:4}

		if [ -z $oldformat ]; then
			table_line="0x4040	$device	\"$driver\""
		else
			dev_str=${line#${line:0:7}}
			table_line="0x4040	$device	\"$driver\"	\"NetXen|$dev_str\""
		fi

		# Find where this pci.updates entry should
		# go within the provided pcitable file.
		while [[ $table_in != $TABLE_DEV ||
			  ${table_in:0:6} < 0x4040 ||
			 ( ${table_in:0:6} == 0x4040 &&
			   ${table_in:7:6} < $device ) ]]
		do
			echo "$table_in"
			read -r table_in
		done

		# Add our table line.  If there are collisions
		# with entries in the pcitable file, prefer our
		# entries to the preexisting ones.
		echo "$table_line"

		# If there was a preexisting entry, prevent
		# duplicate entries by throwing the older one away.
		if [[ ${table_in:0:6} == 0x4040 &&
		      ${table_in:7:6} == $device ]]
		then
			read -r table_in
		fi

	fi

done

echo "$table_in"
while read -r table_in
do
        echo "$table_in"
done

exec 0<&-
exec 1>&-
exec 3<&-
