#!/usr/bin/perl
if ( ! -e "configure.ac" || ! -e "./mk/autoconf/" || ! -e "./mk/jam/" )
{
	die "Run from mk/linux\n";
}
use Cwd;
$top = getcwd;
@directories = ( $top, "$top/mk/autoconf/", "$top/mk/jam/" );
foreach $directory (@directories)
{
	opendir (DIR, $directory) or die ( "Failed to open$directory\n" );
	print "processing dir $directory\n";
	chdir $directory;	
	while ( ($file = readdir(DIR)) )
	{
		if ( $file =~ /.*\.sh|.*-sh|.*\.ac|.*\.ini|.*\.m4|.*\.rpath|.*\.guess|.*\.sub|Jamfile|Jamrules|.*\.jam/ )
		{
			print "   processing file $file\n";
			rename $file, "$file.bak" or die "Could not rename $file to $file.bak\nError:$!\n"; 
			open (INFILE, "<$file.bak") or die "Open $file.bak failed.\n";
			open (OUT, ">$file") or die "Open $file failed.\n";
			while ($line = <INFILE>) 
			{ 
				$line =~ s/\r\n/\n/;
				print OUT $line;
			} 
			close INFILE;
			unlink ( "$file.bak" );
			close OUT;	
		}
		else
		{
			print "   Skipping file $file\n";
		}
	}
	closedir(DIR);
}

# ./mk/autoconf/
# ./mk/jam/
# *.sh *-sh *.ac *.ini *.m4 *.rpath *.guess *.sub Jamfile Jamrules *.jam
