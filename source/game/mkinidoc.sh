#!/bin/bash
# Written by Nathan Turner (2011) for Glest Advanced Engine
# Script to generate INI documentation in Wikia format for http://glest.wikia.com/index.php?title=GAE/INI

input_file=config.db
output_file=ini_doc.txt

echo "Started.."
HEADER="==INI documentation==\n{| class='sortable wikitable' style='empty-cells: show;' \n!INI value \n!Default \n! class='unsortable' | Description"
FOOTER="|}"

# using $1, $3, $6
# Variable					Type	Default			Min		Max		Comments

 egrep -v '^$|^#' $input_file | awk '
 BEGIN { FS = "\\t+"; print "'"$HEADER"'"}
 NF > 0 {
   print "|-"
   print "|"toupper(substr($1, 1, 1)) substr($1, 2)
   if ($3 != "-") {
     if (length($3) > 13)
       print "|"substr($3,0,13)"..."
     else
       print "|"$3
   } else
     print "|"
  
  if ($6 != "-")
    print "|"$6
  else
    print "|"
}
END { print "'$FOOTER'"}' > $output_file
 

 echo "Finished.."