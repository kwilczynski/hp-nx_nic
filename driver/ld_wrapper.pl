#!/build/toolchain/lin32/perl-5.8.8/bin/perl

use File::Basename;
use File::Spec;



########################################################################
#
# GetTopdir --
#
#	Retrieve the directory that represents the top level build
#	directory.
#
# Results:
#
#	Returns the canonical top level path string.
#
# Side effects:
#
#	None.
#
########################################################################

sub GetTopdir 
{
   my($topdir);

   if (defined($ENV{CURRENT_DIR})) {
      $topdir = $ENV{CURRENT_DIR};
   } else {
      $topdir = dirname($0);
   }

   return (File::Spec->canonpath($topdir));
}


########################################################################
#
# GetShaTool --
#
#	Need to find an appropriate command to do SHA1/SHA2 sums.
#
# Results:
#
#	Returns the full path to the chosen tool.
#
# Side effects:
#
#	None.
#
########################################################################

sub GetShaTool 
{
   my($core597) = "/build/toolchain/lin32/coreutils-5.97/bin";
   my($core69) = "/build/toolchain/lin32/coreutils-6.9/bin";
   my($shaSumTool);

   #
   # Prefer SHA2-256.  Also prefer toolchain installation over unknown 
   # version.
   #

   if (-e "$core69/sha256sum") {
      $shaSumTool = "$core69/sha256sum";
   } elsif (!system("sha256sum --version > /dev/null 2>&1")) {
      $shaSumTool = `which sha256sum`;	# Use sha256sum found in path.
   } elsif (-e "$core69/sha1sum") {

      #
      # This is highly unlikely since this version of coreutils 
      # includes sha256sum, which is used preferentially.
      #

      $shaSumTool = "$core69/sha1sum";
   } elsif (-e "$core597/sha1sum") {
      $shaSumTool = "$core597/sha1sum";
   } elsif (!system("sha1sum --version > /dev/null 2>&1")) {
      $shaSumTool = `which sha256sum`;	# Use sha1sum found in path.
   }

   chomp($shaSumTool);
   return ($shaSumTool);
}


########################################################################
#
# GetMD5Tool --
#
#	Need to find an appropriate command to do md5 sums.
#
# Results:
#
#	Returns the full path to the chosen tool.
#
# Side effects:
#
#	None.
#
########################################################################

sub GetMD5Tool 
{
   my($core597) = "/build/toolchain/lin32/coreutils-5.97/bin";
   my($md5SumTool);

   #
   # Prefer toolchain installation over unknown version.
   #

   if (-e "$core597/md5sum") {
      $md5SumTool = "$core597/md5sum";
   } elsif (!system("md5sum --version > /dev/null 2>&1")) {
      $md5SumTool = `which md5sum`;	# Use md5sum found in path.
   }

   chomp($md5SumTool);
   return ($md5SumTool);
}


########################################################################
#
# RestoreQuotes --
#
#	This loop changes command line args into a form that will
#	preserve quotation.  This is necessary since the invocation of
#	this script loses importatant quotation.  Any quotation
#	remaining in this input needs to be escaped to preserve it for
#	the actual command invocation.
#
# Results:
#
#	Returns a modified copy of the @ARGV array that is appropriate
#	for use in a fork'd or exec'd command.
#
# Side effects:
#
#	None.
#
########################################################################

sub RestoreQuotes 
{
   my(@args) = @ARGV;

   foreach (@args) {
      s/"/""/g;					# Double each remaining quote.
      s/("")(([^"]|\\")*)("")/"\\"$2\\""/;	# Escape inner quotes.

      #
      # Quote anything of the form a=b -> a="b".  This will restore
      # some consistency since a="b c" as an input will be preserved.
      #

      s/(-?[\w-]+=)([^"].*)/$1"$2"/;
   }

   return (@args);
}


########################################################################
#
# ProcessArgs --
#
#	Look through the @args list for the specified output file
#	(using the -o switch).
#
# Results:
#
#	Returns the output file (including extension) or undef if not
#	found.
#
# Side effects:
#
#	None.
#
########################################################################

sub ProcessArgs 
{
   my(@args) = @_;
   my($grab_next) = 0;
   my($arg);
   my($file);

   # Process each arg in turn.
   foreach $arg (@args) {
      if ($grab_next > 0) {
	 $file = $arg;
	 $grab_next = 0;
	 last;
      }

      if ("-o" eq substr($arg, 0, 2)) {
	 if ("-o" eq $arg) {
	    $grab_next = 1;
	 } else {
	    ($file = $arg) =~ s/-o([\.\/\w-]+\.o)$/$1/;
	    last;
	 }
      }
   }

   return ($file);	# Includes .o extension.

}



########################################################################
#
# DoFileSum --
#
#	Compute the file hash of the output file and store in the
#	signature file.
#
# Results:
#
#	None.
#
# Side effects:
#
#	- The command line and hash of driver binary are added to the
#	signature file.
#
#	- If the signature file cannot be opened, this function will
#	cause the program to exit and stop the build.
#
########################################################################

sub DoFileSum
{
   my($dir,@args) = @_;

   my($outfile) = ProcessArgs(@args);
   unless (defined($outfile)) {
      print ("No files found in command\n");
      return;
   }

   open(SIGFILE, ">>$dir/driver.sig") or 
      die("Cannot open $dir/driver.sig: $!\n");

   $md5sum = `$md5SumTool $outfile`;

   print SIGFILE ("@args\n");

   #
   # Output the working and build directories if they either differ or
   # an absolute path was used to reference the summed file.  This
   # will help the verification script to construct paths when these
   # file are moved to a different absolution location (on another
   # machine).
   #

   $cwd = File::Spec->canonpath($ENV{PWD});
   if ($cwd ne $dir || File::Spec->file_name_is_absolute($outfile)) {
      print SIGFILE ("WD: $cwd\nBD: $dir\n");
   }

   print SIGFILE $md5sum;

   close(SIGFILE);
}


########################################################################
#
# DoStripObject --
#
#	Strip the release object the way ESX wants it (otherwise ESX
#	will strip it during ramdisk building and the md5sums wont match).
#
# Results:
#
#	None.
#
# Side effects:
#
#	Only strips release object 
#
########################################################################

sub DoStripObject
{
   my (@args) = @_;

   my($outfile) = ProcessArgs(@args);

   unless (defined($outfile)) {
      print ("No files found in command\n");
      return;
   }

   my($cmd) = '/build/toolchain/lin32/binutils-2.14.90.0.4-42/i686-linux/bin/strip';
   my($options) = '--strip-debug';

   if (index($outfile, "release/") >= 0) {
      system ("$cmd $options $outfile");
   }
}


#
# MAIN starts here.
#

unless (-x $ARGV[0]) {
   $fullpath = `which $ARGV[0]`;
   chomp $fullpath;
   unless (-x $fullpath) {
      die "First argument must be executable.\n";
   }
}

$md5SumTool = GetMD5Tool();
unless (defined($md5SumTool)) {
   die "Cannot find appropriate checksum tool in path (md5sum).\n";
}

#
# Save the version of ld.
#

$topdir = GetTopdir();
system("$ARGV[0] --version >> $topdir/driver.sig") && 
	die("Cannot append to file $topdir/driver.sig\n");

@args = RestoreQuotes();

#
# Run the command as requested by the makefile rule.
#

system("@args") && die("Cannot execute command");

DoStripObject(@args);

DoFileSum($topdir, @args);
