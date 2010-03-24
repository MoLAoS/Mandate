#!/bin/bash

die() {
	echo "ERROR: $@"
}

rm -f langlist.raw langlist.noenc langlist.noreg langlist.unsorted

#Run GNU's locale and chop up the output to form a raw list in this format:
#code|Language Name|Region
#
locale -av |
egrep '^(locale: | language |territory )' |
perl -pe '
	s/archive: .*$//g;
	s/locale:/\|locale/g;
	s/( language|territory)//g;
	s/\s+/ /g;s/\|locale /\n/g;
' |
perl -pe '
	s/ +\| +/\|/g;
	s/ +$//g;
	s/^ +//g;
' |
grep -v '^$' > langlist.raw || die

# Create a new list without the encoding portion attached to the language_REGION code
perl -pe '
	s/(\.|@).*?\|/\|/g;
' langlist.raw |
sort -u > langlist.noenc || die

# Finally, create a list of only languages (i.e., remove regions)
perl -pe '
	s/_.*?\|/\|/g;
' langlist.noenc |
awk '-F|' '{print $1 "|" $2}' > langlist.noreg || die

awk '-F|' '{print $1 "=" $2}' langlist.noreg > langlist.unsorted || die
awk '-F|' '{print $1 "=" $2 " (" $3 ")"}' langlist.noenc >> langlist.unsorted || die
sort -u langlist.unsorted > ../../data/game/data/lang/langlist.txt || die


echo "Completed: langlist.txt.  To delete intermediate files, press enter.  Otherise, hit CTRL-C."
read && 
rm -f langlist.raw langlist.noenc langlist.noreg langlist.unsorted


#awk '-F|' '{print $1 "=" $2 " (" $3 ")"}'
