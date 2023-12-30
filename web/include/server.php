<?php

// File:   server.php
// Author: Robert L Walton <walton@acm.org>
// Date:   Fri Dec 29 22:46:55 EST 2023
// 
// The authors have placed RECKON (its files and the
// content of these files) in the public domain; they
// make no warranty and accept no liability for RECKON.

$time_format = "Y-m-d\Th:i:sT";

if ( $method != 'POST' )
    exit ( "bad method: $method" );

session_name ( $session_name );
session_start();

if ( ! isset ( $ipaddr_check ) )
    $ipaddr_check = true;
if (    $ipaddr_check 
     && $_SESSION['IPADDR'] != $_SERVER['REMOTE_ADDR'] )
    exit ( "IPADDR does not match" );

if ( ! isset ( $_POST['ID'] ) )
    exit ( "Request ID not set" );

$ID = $_POST['ID'];

if ( ! isset ( $_SESSION['LOG_TIME'] ) )
    exit ( "Session LOG)TIME not set" );
$log_time =
    date ( $time_format,
	   filemtime
	       ( "$DDIR/log/$ID-start.log" ) );
if ( $log_time != $_SESSION['LOG_TIME'] )
    exit ( "bad log time: $log_time" );

clearstatcache();
umask ( 07 );

function ERROR_HANDLER
	( $errno, $message, $file, $line )
{
    echo ( "ERROR:" . PHP_EOL );
    echo ( "  ERRNO = $errno" . PHP_EOL );
    echo ( "  MESSAGE: $message" . PHP_EOL );
    echo ( "  FILE = $file" . PHP_EOL );
    echo ( "  LINE = $line" . PHP_EOL );
}
set_error_handler ( 'ERROR_HANDLER' );

$rundir = "$DDIR/runs/$ID";
if ( is_dir ( $rundir ) )
{

    $pid = glob ( "$rundir/*.pid" );
    if ( count ( $pid ) > 0 )
    {
	$pid = trim ( file_get_contents ( $pid[0] ) );
	$count = 0;
	while ( file_exists ( "/proc/$pid" )
		&&
		$count < 5 )
	{
		exec ( "kill KILL $pid" );
		sleep ( 1 );
		++ $count;
	}
    }
    foreach ( scandir ( $rundir ) as $f )
    {
	if ( $f == "." ) continue;
	if ( $f == ".." ) continue;
	unlink ( "$rundir/$f" );
    }
    $list = scandir ( $rundir );
    if ( count ( $list ) > 2 )
	exit ( "could not unlink " .
               implode ( ' ', $list ) );
}
else
{
    @mkdir ( "$DDIR/runs" );
    @mkdir ( $rundir );
    if ( ! is_dir ( $rundir ) )
	exit ( "Could not make $rundir" );
}

// PHP symlink appears to fail inexplicably sometimes.
//
function symbolic_link ( $target, $link )
{
    return exec ( "ln -snf $target $link 2>&1" ) == '';
}

if ( exec ( "ln -snf $IDIR/reckon/test/reckon" .
            " $rundir/reckon 2>&1" ) != '' )
    exit ( "could not make symbolic link to reckon" );
if ( ! is_file ( "$rundir/reckon" ) )
    exit ( "reckon program does not exist" );

if ( ! isset ( $_POST['op'] ) )
    exit ( "no op=..." );
$op = $_POST['op'];
if ( ! isset ( $_POST['filename'] ) )
    exit ( "no filename=..." );
$filename = $_POST['filename'];
if ( ! isset ( $_POST['contents'] ) )
    exit ( "no contents=..." );
$contents = $_POST['contents'];


$pattern = '/\A([^\/.][^\/]*)\.rec\Z/';
$r = preg_match ( $pattern, $filename, $matches );
if ( $r !== 1 )
    exit ( "$filename is not a legal .rec file name" );

$base = $matches[1];

if ( ! @file_put_contents ( "$rundir/$filename",
                            $contents ) )
    exit ( "Could not write $filename" );

if ( ! @file_put_contents ( "$rundir/$base.status",
                            "No Status Yet" . PHP_EOL) )
    exit ( "Could not write $base.status" );

$script = "";
$script .= "trap 'exit 129' HUP" . PHP_EOL;
    // If we do not do this, sending HUP
    // returns exit code 0.
$script .= "trap 'echo \$? >$base.exit' EXIT" . PHP_EOL;
$script .= "echo $$ > $base.pid" . PHP_EOL;
$script .= "set -e" . PHP_EOL;
$script .= "cat < $filename" . PHP_EOL;
if ( ! @file_put_contents ( "$rundir/$base.sh",
                            $script ) )
    exit ( "Could not write $base.sh" );

$exec = exec ( "cd $rundir; " .
               "setsid bash $base.sh " .
               ">$base.out 2>$base.err & ",
               $exec_output, $exec_code );
if ( $exec === false )
    exit ( "could not exec" );
elseif ( $exec_code != 0 )
{
    echo ( "bad exec: result code = $exec_code" .
           " output:" . PHP_EOL );
    echo ( join ( PHP_EOL, $exec_output ) );
    exit ( PHP_EOL );
}

if ( ! isset ( $run_delay ) )
    $run_delay = 10;
$start = microtime ( true );
while ( true )
{
    // On a 1-CPU computer, we MUST sleep to give up
    // the CPU to the process we have just started,
    // otherwise the process will never create files
    // like $base.pid, $base.out, and $base.exit.
    //
    time_nanosleep ( 0, 200000000 );
    if ( file_exists ( "$rundir/$base.exit" ) )
        break;
    if ( microtime ( true ) > $start + $run_delay )
        break;
}

if ( file_exists ( "$rundir/$base.exit" ) )
{
    $status =
	file_get_contents ( "$rundir/$base.exit" );
    $status = trim ( $status );
    $output =
	file_get_contents ( "$rundir/$base.out" );
    $errors =
	file_get_contents ( "$rundir/$base.err" );
    if ( $status != "0"
         ||
         strlen ( $errors ) > 0 )
    {
	echo ( "EXIT STATUS: $status" . PHP_EOL );
	if ( strlen ( $errors ) > 0 )
	{
	    echo ( "ERRORS:" . PHP_EOL );
	    echo ( $errors );
	}
	echo ( "================================" .
	       PHP_EOL );
    }
    echo ( $output );
}
elseif ( file_exists ( "$rundir/$base.pid" ) )
{
    $output =
	file_get_contents ( "$rundir/$base.status" );
    echo ( "\0STATUS\0\n" );
    echo ( $output );
}
else
    echo ( "run failed to start (no $base.pid)" );

?>
