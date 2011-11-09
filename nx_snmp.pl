#!/usr/bin/perl

#This script describes how to export the statistics information
#of the nic to the snmp agent (in this case snmpd daemon). The daemon passes 
#control to this script when it gets a request to MIBOID .1.3.6.1.2.1.10.7.11.1 
#This MIBOID should be specified in the snmpd daemon configuration file. For 
#example in /etc/snmp/snmpd.conf under "Pass through control". The Script reads 
#the statistics the driver exposes through the /proc interface and matches them 
#to the corresponding MIBOID. It gets invoked by the daemon using either of the 
#two command line arguments -g or -n.


$debug_filename ="/tmp/debugfile";
$UNM_PROC = "/proc/net/nx_nic";
use Switch;

%oidnamemap64 =(
".1.3.6.1.2.1.10.7.11.1.1" => "dot3HCStatsAlignmentErrors", 
".1.3.6.1.2.1.10.7.11.1.2" => "dot3HCStatsFCSErrors", 
".1.3.6.1.2.1.10.7.11.1.3" => "dot3HCStatsTransmitErrors", 
".1.3.6.1.2.1.10.7.11.1.4" => "dot3HCStatsFrameTooLongs", 
".1.3.6.1.2.1.10.7.11.1.5" => "dot3HCStatsReceiveErrors", 
".1.3.6.1.2.1.10.7.11.1.6" => "dot3HCStatsSymbolErrors");

$maxindex = 7;
$oid_argv = $ARGV[1];
$option = $ARGV[0];
$oidmap = ".1.3.6.1.2.1.10.7.11.1";
$base = ".1.3.6.1.2.1.10.7.11.1";
$index = 0;
$extra = 1;

populate_oidnamehash();
check();
get();
debug();

sub check 
{
	@segments = split /\./, $oid_argv;
	$index = $segments[11];
	$extra = $segments[12];
	$oidmap = $base .'.'. $index;
	if ($extra == 1) {
		exit();
	}
# Getnext when oid received contains subset which is present in our hash.
	if (($option eq "-n") && (exists $oidnamemap64{$oidmap})) {
		if ($index >= $maxindex) {
			exit();
		}
	# Assign parent node (oidmap) as an oid.
		$index = $index +2;
		$oid_argv = $base .'.' . $index;
		if($index ==5) {
			$oidmap = $base .'.'. $index;
                	$oid_argv = $base .'.'. $index;
                	display("counter");
			exit(0);	
		}
		else{
			exit(0);
		}
# Get when oid received is base.
	}elsif (($option eq "-g") && ($oid_argv eq $base)) {
		$index = 3;
		$oidmap = $base .'.'. $index;
		$oid_argv = $oidmap;
#		$oid_argv = $base .'.'. $index;
	}elsif (($option eq "-n") && ($oid_argv eq $base)) {
		$index = 3;
		$oidmap = $base .'.'. $index;
		$oid_argv = $base .'.'. $index;
		display("counter");
		exit(0);
	}
}

sub get
{
	switch($oidnamemap64{$oidmap}) {
#	case "dot3HCStatsAlignmentErrors"	{ display("counter") }
#	case "dot3HCStatsFCSErrors"		{ display("counter") }
	case "dot3HCStatsTransmitErrors"	{ display("counter") }
#	case "dot3HCStatsFrameTooLongs"		{ display("counter") }
	case "dot3HCStatsReceiveErrors"		{ display("counter") }
#	case "dot3HCStatsSymbolErrors"		{ display("counter") }
#	else					{ debug() }
	}
}

sub display
{
	$s = $oid_argv . "\n$_[0]\n";
	$name = $oidnamemap64{$oidmap};
	$r = $oidnamehash{$name};
	$s = $s . $r;
	print $s;
}

sub debug 
{
  open DEBUG_FILE, ">>/tmp/debugfile";
  print DEBUG_FILE "ARGV[1] $ARGV[1]\n";
  print DEBUG_FILE "\$index $index\n";
  print DEBUG_FILE "\$oidmap $oidmap\n";
  print DEBUG_FILE "\$oid_argv $oid_argv\n";
  $s = $oid_argv . "\ncounter\n"; 
  $r = $oidnamehash{$oidnamemap64{$oidmap}};
  $s = $s . $r;
  print DEBUG_FILE "\$s => $s\n";
  print DEBUG_FILE "\$r => $r\n";
  print DEBUG_FILE "\$option => $option\n";
  print DEBUG_FILE "End of file \n\n";
  close DEBUG_FILE;
}

sub populate_oidnamehash
{
	my @interfaces = `ls $UNM_PROC/dev*`;

	foreach $PROC_FILE(@interfaces) {
		open PROC_FILE or die "$UNM_PROC does not exist\n";
		@unmproclst =<PROC_FILE>;
		$i = 1;

		@txerrors = grep /Cmd Desc Error /, @unmproclst;
		foreach $var(@txerrors) {
			@singleentry = split /:/, $var;
		}
		$s1="dot3HCStatsTransmitErrors";
		$oidnamehash{$s1}=$singleentry[1];

		@rcverrors = grep /Bad SKB  /, @unmproclst;
		foreach $var(@rcverrors) {
			@singleentry = split /:/, $var;
		}
		$s1="dot3HCStatsReceiveErrors";
		$oidnamehash{$s1}=$singleentry[1];

		$i++;
	}
}
