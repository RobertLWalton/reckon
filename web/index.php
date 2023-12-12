<?php

$rec_session_name = "REC_765309476514";
    // Reset 12 digit number to NON-PUBLIC, SITE-
    //     // SPECIFIC 12 digit random number.

$rec_home = "/home2/reckon/reckon/web";
    // Location of cloned reckon/web directory
    // containing startup.html, client.html, etc.

$rec_data = "/home2/reckon/reckon/web-data";
    // Location of data directory.  Should not be
    // visible to the web (i.e., should not be a
    // subdirectory of the directory containing
    // this index.php file).

$rec_method = $_SERVER['REQUEST_METHOD'];

if ( $rec_method == 'POST' )
    require "$rec_home/server.php";
else if ( isset ( $_GET['client'] ) )
    require "$rec_home/client.html";
else
    require "$rec_home/startup.html";
?>
