<?php

// File:    server.php
// Author:  Robert L Walton <walton@acm.org>
// Date:    Sat Jun  3 13:20:49 EDT 2023

// The authors have placed RECKON (its files and the
// content of these files) in the public domain; they
// make no warranty and accept no liability for RECKON.


// Method must be GET.
//
$method = $_SERVER['REQUEST_METHOD'];
if ( $method != 'POST' )
    exit ( "UNACCEPTABLE HTTP METHOD $method" );

$ID = $_SESSION['ID'];
if ( ! isset ( $_POST['id'] )
     ||
     $ID != $_POST['id'] )
    exit ( "ID MISMATCH" );

if ( $_SESSION['REMOTE_ADDR'] != $_SERVER['REMOTE_ADDR'] )
    exit ( "IP ADDRESS MISMATCH" );

// Update $_SESSION['ID'.
//
$key = hex2bin ( $_SESSION['KEY'] );
$id = hex2bin ( $ID );
$iv = hex2bin ( '00000000000000000000000000000000' );
$id    = @openssl_encrypt ( $id, 'aes-128-cbc', $key,   
                           OPENSSL_RAW_DATA +
			   OPENSSL_ZERO_PADDING, $iv );
$_SESSION['ID'] = bin2hex ( $id );

echo ( $_SESSION['ID'] + $_POST['input'] );

?>


