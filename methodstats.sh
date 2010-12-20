#!/bin/bash

# Copyright (C) 2010 by Nathan Turner (hailstone) for Glest Advanced Engine
# GPL V2, see source/licence.txt

if [ $# -lt 2 ]
then
	echo "Usage: $0 dir topn"
	exit 1
fi

files=`find $1 -name \\*.cpp -type f -print`
delim="^"
topn=`expr $2 "+" 1`

debug="#" # set to "print" to output debug info or "#" to hide

method_data=$(awk '
# if count is equal to 0 then the open has
# already been closed and therefore not valid
function isValidOpen(line)
{
	count = 0;
	n = split(line, str, "")
	for (i = 1; i <= n; i++)
	{
		if (str[i] == "{")
		{
			count++
		}
		else
		if (count != 0 && str[i] == "}")
		{
			count--
		}
	}
	'$debug' "#Is valid?"(count != 0)
	return (count != 0);
}

NF > 0 {
	// remove leading whitespace
	sub(/^[ \t]+/, "", $0)
	
	# skip lines that have // at the start
	if ($0 ~ /^[/][/]/)
		next
	# remove any other one line comments
	split($0, temp, "//")
	$0 = temp[1]
	
	# methods with return type and constructor/destructor
	if (!in_function && $0 ~ /(.+\ .+::.+\(|^\ *.+::.+\()/ && $0 !~ /;|case|if|while|new/)
	{
		# init variables for function
		current_function = $0
		lines[$0] = 0
		depth[$0] = 0
		max_depth[$0] = 0
		
		function_count++
		#print $0
		in_function = 1
		'$debug' "#entered function"
	}
	
	# close curly bracket
	if (in_function && $0 ~ /}/)
	{
		depth[current_function]--
		if (depth[current_function] == 0)
		{
			in_function = 0
			do_line_count = 0
			'$debug' "#NumLines: "lines[current_function]
			#print "left function"
		}
		'$debug' "#close",depth[current_function]
		'$debug' $0
	}
	
	# increment if within the curly braces of the method
	# ordering of the open and close bracket checks matters
	if (do_line_count)
	{
		lines[current_function]++
		#print lines[current_function]
	}
	
	# open curly bracket
	# make sure this doesnt mess up closing if its on one line
	if (in_function && $0 ~ /{/ && isValidOpen($0))
	{
		depth[current_function]++
		if (depth[current_function] > max_depth[current_function])
		{
			max_depth[current_function] = depth[current_function]
		}
		do_line_count = 1
		'$debug' "#open",depth[current_function]
		'$debug' $0
	}
}

END {
	print "Number of functions: "function_count
	for (f in lines)
		printf "%6i'$delim'%2i'$delim'%s\n",lines[f],max_depth[f],f
}

' $files)

# number of lines
echo -e "$method_data" | sort -r -t $delim -k 1 | awk '
BEGIN {
	printf "%6s %6s\n","#Lines","Method"
	print "----------------------------"
}

NR < '$topn' && NF > 2 {
	printf "%6i %s\n",$1,$3
}
' FS=$delim

echo

# block depth
echo -e "$method_data" | sort -r -t $delim -k 2 | awk '
BEGIN {
	printf "%6s %6s\n","Depth","Method"
	print "----------------------------"
}

NR < '$topn' && NF > 2 {
	printf "%6i %s\n",$2,$3
}
' FS=$delim

