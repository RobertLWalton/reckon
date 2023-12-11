<?php

$rec_home = "/home2/reckon/reckon/web";

$rec_method = $_SERVER['REQUEST_METHOD'];

if ( $rec_method == 'POST' )
    require "$rec_home/server.php";
else if ( isset ( $_GET['client'] ) )
    require "$rec_home/client.html";
else
    require "$rec_home/startup.html";
?>
