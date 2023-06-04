<?php

// File:    client.php
// Author:  Robert L Walton <walton@acm.org>
// Date:    Sat Jun  3 23:51:41 EDT 2023

// The authors have placed RECKON (its files and the
// content of these files) in the public domain; they
// make no warranty and accept no liability for RECKON.


// Method must be GET.
//
$method = $_SERVER['REQUEST_METHOD'];
if ( $method != 'GET' )
    exit ( "UNACCEPTABLE HTTP METHOD $method" );

// Set $key and $id to random 16 byte binary strings.
//
$rdesc = @fopen ( '/dev/random', 'r' );
if ( $rdesc === false )
    exit ( 'CANNOT OPEN /dev/random FOR' .
	   ' READING' );
$key = @fread ( $rdesc, 32 );  // Pseudo-random key.
$id = @fread ( $rdesc, 32 );   // Pseudo-random seed.
fclose ( $rdesc );

session_name ( 'RECKON-SESSION' );
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
white-space: pre;
overflow-y: scroll;
overflow-x: auto;
border: 5px solid red;
width: 100%;
height: 50vh;
}
div.input {
font-family: monospace;
white-space: pre;
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
let ID = '<?php echo $ID; ?>';

let xhttp = new XMLHttpRequest();

let request_in_progress = false;

function FAIL ( message )
{
    alert ( message );
    window.close();
    location.assign ( 'client.php' );
}

function SUBMIT()
{
    let input = document.getElementById ( 'input' );

    xhttp.onreadystatechange = function() {
        if ( this.readyState != XMLHttpRequest.DONE
	     ||
	     ! request_in_progress )
	    return;
	console.log ( 'STATUS: ' + this.status );
	request_in_progress = false;
	if ( this.status != 200 )
	    FAIL ( 'Bad response statue ('
		   + this.status
		   + ') from server on update request' );
	PROCESS_RESPONSE ( this.responseText );
    }

    xhttp.open
        ( 'POST', 'server.php', true /* async */ );
    xhttp.setRequestHeader
	( "Content-Type",
	  "application/x-www-form-urlencoded" );
    request_in_progress = true;

    data = 'id=' + ID + '&input=' +
           encodeURIComponent ( input.innerText );
    console.log ( 'SEND: ' + data );
    xhttp.send ( data );
}

let is_id_re = /^[a-fA-F0-9]{32}$/;

function PROCESS_RESPONSE ( responseText )
{
    console.log ( 'RECEIVE: ' + responseText );
    let output = document.getElementById ( 'output' );
    let ID = responseText.substring ( 0, 32 );
    if ( ! is_id_re.test ( ID ) )
        FAIL ( 'BAD RESPONSE: ' + responseText );
    let outputText = responseText.substring ( 32 );
    console.log ( responseText );
    console.log ( ID );
    console.log ( outputText );
    output.insertAdjacentHTML
        ( 'beforeend', outputText );
}

</script>
</head>
<body>
<h2 class='header'>Output</h2>
<div class='output' id='output'>
</div>
<div class='header'>Input
     <button type='button' onclick='SUBMIT()'>Submit</button></div>
<div class='input' contenteditable='true' id='input'>
</div>
</body>
</html>

