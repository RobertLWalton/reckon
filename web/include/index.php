<?php

// File:   index.php
// Author: Robert L Walton <walton@acm.org>
// Date:   Mon Jul  8 17:01:13 EDT 2024
//
// The authors have placed RECKON (its files and the
// content of these files) in the public domain; they
// make no warranty and accept no liability for RECKON.

$session_name = "REC_765309476514";
    // Reset 12 digit number to NON-PUBLIC, SITE-
    //     // SPECIFIC 12 digit random number.

$IDIR = "/home2/reckon";
    // Installation directory.

$DDIR = "/home2/reckon/web-data";
    // Location of data directory.  Should not be
    // visible to the web (i.e., should not be a
    // subdirectory of the directory containing
    // this index.php file).

$method = $_SERVER['REQUEST_METHOD'];

function ERROR_HANDLER
	( $errno, $message, $file, $line )
{

    if ( ( error_reporting() & E_WARNING ) == 0 )
        return true;
        // Return if @ operator has suppressed E_WARNING
        // error handling.  Returning true suppresses
        // normal error handling.

    echo ( "SYSTEM ERROR:" . "<br>" .
           "  ERRNO = $errno" . "<br>" .
           "  MESSAGE: $message" . "<br>" .
	   "  ERROR AT LINE $line IN $file" . "<br>" );
}
set_error_handler ( 'ERROR_HANDLER' );

if ( $method == 'POST' )
    require "$IDIR/reckon/web/include/server.php";
else if ( isset ( $_GET['ID'] ) )
    require "$IDIR/reckon/web/include/client.html";
else
    require "$IDIR/reckon/web/include/startup.html";
?>
