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
# PreprocessorArgs --
#
#	Run C preprocessor on files.  First need to replace compile
#	option -c with -E in the original argument list.  All other
#	arguments that are appropriate are kept.  The argument list
#	passed to this function must already have been converted via
#	the subroutine RestoreQuotes.  User must compile everything to
#	.o form one file at a time.
#
# Results:
#
#	Returns several values to the calling process:
#
#	$file:		Full value passed to -o in @args list or undef.
#	$source:	Source file found with extension stripped or undef.
#	$sourceExt:	Source file extension or undef.
#	$doPreprocess:	Boolean whether preprocessing is needed (may be undef).
#	@preargs:	Converted @args list appropriate for preprocessing the 
#			source file.
#
# Side effects:
#
#	If more than one source file is found on the command line, the
#	function will cause the program to exit and the build process
#	to fail.
#
########################################################################

sub PreprocessorArgs 
{
   my(@args) = @_;
   my($grabNext) = 0;
   my($arg);
   my(@preargs,$file,$source,$sourceExt,$doPreprocess);

   #
   # Process each arg in turn.  Keep, replace, or discard as required.
   #

   foreach $arg (@args) {
      if ($grabNext > 0) {
	 ($file = $arg) =~ s/([\.\/\w-]+)\.o$/$1/;	# Strip .o extension
	 $grabNext = 0;
	 next;
      }

      if ("-c" eq substr($arg, 0, 2)) {
	 @preargs = (@preargs, "-E");
	 next;
      } elsif ("-o" eq substr($arg, 0, 2)) {
	 if ("-o" eq $arg) {
	    $grabNext = 1;
	 } else {
	    ($file = $arg) =~ s/-o([\.\/\w-]+)\.o$/$1/;
	 }
	 next;
      } elsif ($arg =~ /([\.\/\w-]+)\.(cxx|cpp|c\+\+|ii|cc|cp|[cCsSi])$/) {
	 if (defined($source)) {
	    die("You must only specify a single source file!\n");
	 }
	 $source = $1;
	 $sourceExt = $2;

	 $doPreprocess = ($2 =~ /^cxx|cpp|c\+\+|cc|cp|[cCS]$/);
      }
      @preargs = (@preargs, $arg);	# Keep this arg
   }

   return ($file,$source,$sourceExt,$doPreprocess,@preargs);
}


########################################################################
#
# DoFileSum --
#
#	Run the preprocessor (if required) and store in the signature
#	file the compilation arguments and the file hash result.  In
#	addition, the build directory and working directory may be
#	stored in the signature file to later help with verification.
#	The temporary preprocessed file (if needed) is stored in the
#	same location as the output file and deleted after the hash
#	has been computed.  The signature file is always referenced at
#	the top level build directory.
#
# Results:
#
#	None.
#
# Side effects:
#
#	- Temporary file to hold the preprocessor output is created
#	and deleted if the preprocessor is invoked.
#
#	- At least one line is added to the signature file to
#	correspond to the hash of the (preprocessed) source file.
#
#	- If the signature file cannot be opened, this function will
#	cause the program to exit and stop the build.
#
#	- If the preprocessor does not run successfully or the output
#	is not stored in a new file, this function will cause the
#	program to exit and stop the build.
#
########################################################################

sub DoFileSum
{
   my($dir,@args) = @_;
   my($showPath) = 0;

   my($outfile,$source,$ext,$preprocess,@preargs) = PreprocessorArgs(@args);

   unless (defined($source)) {
      print ("No supported files found in command.\n");
      return;
   }

   unless (defined($outfile)) {
      $outfile = $source;	# Use source instead.
   }

   open(SIGFILE, ">>$dir/driver.sig") or 
      die("Cannot open $dir/driver.sig: $!\n");

   if ($preprocess) {
      my $pre_ext;
      my $line;
      #
      # Undefine preprocessor variable that will not be constants.
      #

      @preargs = (@preargs,"-U__TIME__","-U__DATE__");
      print SIGFILE ("@preargs\n");

      if ($ext =~ /cxx|cpp|c\+\+|cc|cp|C/) {
	 $pre_ext = "ii";
      } elsif ($ext eq "c") {
	 $pre_ext = "i";
      } elsif ($ext eq "S") {
	 $pre_ext = "s";
      }

      #
      # Process the output of the preprocessor one line at a time.
      # Remove lines which begin with a '#' or which contain only
      # whitespace.  This removes nonessential lines from the
      # preprocessed output which could otherwise adversely affect the
      # checksum verification.  One possible problem is that header
      # file paths are included in comment lines.  If this refers to a
      # non-DDK header file, the path will most likely be different on
      # the system on which verification is done.
      #

      open(OUTFILE,">$outfile.$pre_ext") or 
	 die("Cannot open $outfile.$pre_ext\n");

      open(INFILE,"@preargs 2>/dev/null |") or
	 die("Cannot execute $preargs[0]\n");

      while ($line = <INFILE>) {

	 #
	 # Discard comments and blank lines.
	 #

	 if ($line =~ /^(?:#.*|\s*)$/) {
	    next;
	 }

	 print OUTFILE $line;
      }

      close(INFILE);
      close(OUTFILE);

      unless (-s "$outfile.$pre_ext"){
	 die("Output of preprocessor was empty!\n");
      }

      $md5sum = `$md5SumTool $outfile.$pre_ext`;

      unlink "$outfile.$pre_ext";

      if (File::Spec->file_name_is_absolute($outfile)) {
	 $showPath = 1;
      }
   } else {
      $md5sum = `$md5SumTool $source.$ext`;

      if (File::Spec->file_name_is_absolute($source)) {
	 $showPath = 1;
      }
   }

   #
   # Output the working and build directories if they either differ or
   # an absolute path was used to reference the summed file.  This
   # will help the verification script to correctly identify sources
   # when these file are moved to a different absolution location (on
   # another machine).
   #

   $cwd = File::Spec->canonpath($ENV{PWD});
   if ($cwd ne $dir || $showPath) {
      print SIGFILE ("WD: $cwd\nBD: $dir\n");
   }

   print SIGFILE $md5sum;

   close(SIGFILE);
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
# Save the version of the DDK, gcc and md5 tool.
#

$topdir = GetTopdir();
unless (-e "$topdir/driver.sig") {
   system("rpm -qa VMware-esx-ddk-devtools > $topdir/driver.sig") && 
	   die("Cannot create file $topdir/driver.sig\n");

   system("$ARGV[0] --version >> $topdir/driver.sig") && 
	   die("Cannot append to file $topdir/driver.sig\n");

   system("$md5SumTool --version >> $topdir/driver.sig") && 
	   die("Cannot append to file $topdir/driver.sig\n");
}

@args = RestoreQuotes();

DoFileSum($topdir, @args);

#
# Finally, run the command as requested by the makefile rule.
#

$cmd = "@args";
exec ($cmd);

print ("Cannot execute: @args[0]\n");
exit -1;
