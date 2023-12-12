<?php

// File:   server.php
// Author: Robert L Walton <walton@acm.org>
// Date:   Tue Dec 12 05:55:11 UTC 2023
// 
// The authors have placed EPM (its files and the
// content of these files) in the public domain; they
// make no warranty and accept no liability for EPM.

if ( $rec_method != 'POST' )
    error ( "bad method: $rec_method" );

if ( ! isset ( $_POST['ID'] ) )
    error ( "no ID=..." );
$ID = $_POST['ID'];
session_name ( $rec_session_name );
session_start();
if ( $ID != $_SESSION['ID'] )
    error ( "bad ID: $ID" );

clearstatcache();
umask ( 07 );

if ( ! isset ( $_POST['op'] ) )
    error ( "no op=..." );
$op = $_POST['op'];
if ( ! isset ( $_POST['filename'] ) )
    error ( "no filename=..." );
$filename = $_POST['filename'];
if ( ! isset ( $_POST['contents'] ) )
    error ( "no contents=..." );
$contents = $_POST['contents'];

echo ( "ID = $ID" . PHP_EOL );
echo ( "op = $op" . PHP_EOL );
echo ( "filename = $filename" . PHP_EOL );
echo ( $contents );

?>
