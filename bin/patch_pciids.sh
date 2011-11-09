#! /bin/bash
# $1 = system pci.ids file to update
# $2 = file with new entries in pci.ids file format
# $3 = pci.ids output file

if [ $# != 3 ]; then
	echo "usage : $0 <system pci.ids> <pci.updates> <pci.ids.new>"
	exit 1
fi

if [ ! -f $1 -a ! -L $1 ]; then
	echo "The source pci.ids file ($1) does not exist or is not a file."
	exit 2
fi

if [ ! -f $2 ]; then
	echo "The source pci.updates file ($2) does not exist."
	exit 3
fi

if [ -e $3 ]; then
	echo "The destination filename ($3) already exists."
	exit 4
fi

exec 0<$1
exec 1>$3
exec 3<$2

IFS=

# pattern matching strings
ID="[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]]"
VEN="${ID}*"
DEV="	${ID}*"
SUB="		${ID}*"

line=
ids_in=
device=
subdev=
subdev=

# get the first line of each data file to jump start things
read -r ids_in

# outer loop reads lines from the updates file
while read -r -u 3 line; do

	# vendor entry
	if [[ $line == $VEN ]]; then

		# Find where the vendor line would be placed in the
		# pci.ids file, outputting the lines as we go.
		while [[ $ids_in != $VEN ||
		        0x${ids_in:0:4} < 0x4040 ]]; do

			echo "$ids_in"
			read -r ids_in

		done

		# Insert our vendor line into the output stream.
		echo "$line"

		# If the next line is our vendor line,
		# discard it by priming the next read.
		if [[ 0x${ids_in:0:4} == 0x4040 ]]; then
			read -r ids_in
		fi

	# device entry
	elif [[ $line == $DEV ]]; then

		device=0x${line:1:4}

		# Start where we left off with the vendor line.  Find
		# where the current device line should be placed, again
		# outputting the lines we're reading in as we go.
		while [[ $ids_in != $DEV || 0x${ids_in:1:4} < $device ]]; do

			# If we've reached a vendor line, we've gone far enough.
			if [[ $ids_in == $VEN ]]; then
				break
			fi

			echo "$ids_in"
			read -r ids_in

		done

		# Insert our device line into the output stream.
		echo "$line"

		# If the next line also describes our current device,
		# discard it by priming the next line.
		if [[ 0x${ids_in:1:4} == $device ]]; then
			read -r ids_in
		fi

	# subsystem entry
	elif [[ $line == $SUB ]]; then

		subven=0x${line:2:4}
		subdev=0x${line:7:4}

		# Start where we left off with the device line.  Find
		# where the current device line should be placed, again
		# outputting the lines we're reading in as we go.
		while [[ $ids_in == $SUB &&
		       ( 0x${ids_in:2:4} <  $subven ||
		       ( 0x${ids_in:2:4} == $subven &&
		         0x${ids_in:7:4} <  $subdev )) ]]; do

			echo "$ids_in"
			read -r ids_in

		done

		# Insert our subvendor / subdevice line into the output stream.
		echo "$line"

		# If the next line also describes our current subvendor / subdevice,
		# discard it by priming the next line.
		if [[ 0x${ids_in:2:4} == $subven  &&
		      0x${ids_in:7:4} == $subdev ]]; then
			read -r ids_in
		fi

	fi

done

# print the remainder of the original files
echo "$ids_in"
while read -r ids_in; do
        echo "$ids_in"
done

exec 0<&-
exec 1<&-
exec 3<&-
