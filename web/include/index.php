<?php

// File:   index.php
// Author: Robert L Walton <walton@acm.org>
// Date:   Fri Dec 22 03:31:05 EST 2023
//
// The authors have placed RECKON (its files and the
// content of these files) in the public domain; they
// make no warranty and accept no liability for RECKON.

$rec_session_name = "REC_765309476514";
    // Reset 12 digit number to NON-PUBLIC, SITE-
    //     // SPECIFIC 12 digit random number.

$rec_reckon = "/home2/reckon/reckon";
    // Location of cloned reckon directory.

$rec_data = "/home2/reckon/web-data";
    // Location of data directory.  Should not be
    // visible to the web (i.e., should not be a
    // subdirectory of the directory containing
    // this index.php file).

$rec_method = $_SERVER['REQUEST_METHOD'];

if ( $rec_method == 'POST' )
    require "$rec_reckon/web/include/server.php";
else if ( isset ( $_GET['client'] ) )
    require "$rec_reckon/web/include/client.html";
else
    require "$rec_reckon/web/include/startup.html";
?>
