<?php

// File:   server.php
// Author: Robert L Walton <walton@acm.org>
// Date:   Wed Dec 27 00:48:32 EST 2023
// 
// The authors have placed RECKON (its files and the
// content of these files) in the public domain; they
// make no warranty and accept no liability for RECKON.

if ( $method != 'POST' )
    exit ( "bad method: $method" );

if ( ! isset ( $_POST['ID'] ) )
    exit ( "no ID=..." );
$ID = $_POST['ID'];
session_name ( $session_name );
session_start();
if ( $ID != $_SESSION['ID'] )
    exit ( "bad ID: $ID" );

clearstatcache();
umask ( 07 );

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
