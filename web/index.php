<?php

$rec_home = "/home2/reckon/reckon/web";

if ( isset ( $_GET['S'] ) )
    require "$rec_home/server.php";
else
    require "$rec_home/client.html";

?>


