<?php

// File:   server.php
// Author: Robert L Walton <walton@acm.org>
// Date:   Thu Dec 28 20:04:31 EST 2023
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

if ( ! isset ( $_POST['op'] ) )
    exit ( "no op=..." );
$op = $_POST['op'];
if ( ! isset ( $_POST['filename'] ) )
    exit ( "no filename=..." );
$filename = $_POST['filename'];
if ( ! isset ( $_POST['contents'] ) )
    exit ( "no contents=..." );
$contents = $_POST['contents'];

echo ( "ID = $ID" . PHP_EOL );
echo ( "op = $op" . PHP_EOL );
echo ( "filename = $filename" . PHP_EOL );
echo ( $contents );

?>
