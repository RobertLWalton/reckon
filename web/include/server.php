<?php

// File:   server.php
// Author: Robert L Walton <walton@acm.org>
// Date:   Fri Dec 29 07:27:05 EST 2023
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

exec ( "cd $rundir; " .
       "setsid bash $base.sh " .
       ">$base.out 2>$base.err & " );

sleep ( 2 );

echo ( "ID = $ID" . PHP_EOL );
echo ( "op = $op" . PHP_EOL );
echo ( "filename = $filename" . PHP_EOL );
$f = "$rundir/$base.out";
$c = @file_get_contents ( $f );
echo ( $c );
?>
