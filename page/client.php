<?php

// File:    client.php
// Author:  Robert L Walton <walton@acm.org>
// Date:    Fri Jun  2 05:54:57 EDT 2023

// The authors have placed RECKON (its files and the
// content of these files) in the public domain; they
// make no warranty and accept no liability for RECKON.


// Method must be GET.
//
$method = $_SERVER['REQUEST_METHOD'];
if ( $method != 'GET' )
    exit ( "UNACCEPTABLE HTTP METHOD $method" );

// Set $key, $id, and $name to random 16 byte binary
// strings.
//
$rdesc = @fopen ( '/dev/random', 'r' );
if ( $rdesc === false )
    exit ( 'CANNOT OPEN /dev/random FOR' .
	   ' READING' );
$key = @fread ( $rdesc, 32 );  // Pseudo-random key.
$id = @fread ( $rdesc, 32 );   // Pseudo-random seed.
$name = @fread ( $rdesc, 32 ); // Session name.
fclose ( $rdesc );

session_name ( 'RECKON-SESSION-' . bin2hex ( $name ) );
session_start();
clearstatcache();
umask ( 07 );
header ( 'Cache-Control: no-store' );

$_SESSION['KEY'] = bin2hex ( $key );
$ID = bin2hex ( $id );
$_SESSION['ID'] = $ID;

$_SESSION['REMOTE_ADDR'] = $_SERVER['REMOTE_ADDR'];
?>

<!DOCTYPE html>
<html>
<head>
<style>
div.output {
font-family: monospace;
overflow-y: scroll;
overflow-x: auto;
border: 5px solid red;
width: 100%;
height: 50vh;
}
div.input {
font-family: monospace;
overflow-y: scroll;
overflow-x: auto;
border: 5px dashed green;
width: 100%;
height: 30vh;
}
.header {
text-align: center;
font-size: 200%;
font-weight: bold;
}
button {
height: 2em;
}
</style>
<script>
var ID = '<?php echo $ID; ?>';
function LOG()
{
    var input = document.getElementById ( 'input' );
    console.log ( input.innerText );
    console.log ( 'FOO' );
}

</script>
</head>
<body>
<h2 class='header'>Output</h2>
<div class='output'>
</div>
<div class='header'>Input
     <button type='button' onclick='LOG()'>Submit</button></div>
<div class='input' contenteditable='true' id='input'>
</div>
</body>
</html>

