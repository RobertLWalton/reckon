// Reckon Server
//
// File:	reckon_server.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Tue May  9 02:28:45 EDT 2023
//
// The authors have placed this program in the public
// domain; they make no warranty and accept no liability
// for this program.

// Table of Contents
//
//	Design
//	Usage and Setup
//	Reckon Server Functions
//	Reckon Server

// Design
// ------

// The RECKON Server listens to a port and serves all
// http requests made to that port.
//
// If an http request is a GET without a cookie, the
// server:
//
//     (1) creates a new cookie CCC
//     (2) creates the directory `/sessions/CCC'
//     (3) writes to the log file `/log/reckon.log' the
//         line: `TIME IPADDR CCC get'
//     (4) returns an http response containing the page
//         `/page/user.html' and the session cookie CCC
//         [Note: session cookies expire when the
//                browser shuts down]
//
// If an http request is a POST with cookie CCC, the
// server:
//
//     (1) writes the request to `/sessions/CCC/TIME.in'
//     (2) remembers the request connection file
//         descriptor and whether a trace is requested
//     (2) launches a `reckon' program process to read
//         input from `/sessions/CCC/TIME.in', write
//         document output to `/sessions/CCC/TIME.out',
//         and write trace output to `/sessions/CCC/
//         TIME.trace'.
//     (3) writes to the log file `/log/reckon.log' the
//         line: `TIME IPADDR CCC post'
//
// When a `reckon' program process terminates, the
// server:
//
//     (1) writes to the log file `/log/reckon.log' the
//         line: `TIME IPADDR CCC finish CPU-TIME'
//     (2) returns an http response containing the
//         TIME.out file and if trace requested the
//         TIME.trace file, plus CPU-TIME
//
// For an IPADDR, the TIMEs and CPU-TIMEs of the last
// several requests are remembered and use to throttle
// the system.  The number of current unfinished
// `reckon' program processes and current number of
// sessions are also used.
//
// System errors are logged to `/log/reckon.log' as
//     TIME IPADDR CCC error ERRNO DESCRIPTION
// where
//     IPADDR and CCC may be `-' if not relevant
//     DESCRIPTION has the form `while CONTEXT: STRING'
//     where STRING is the value of strerror(ERRNO).
//
//
//

// Usage and Setup
// ----- --- -----



// Reckon Server Functions
// ------ ------ ---------

FILE * err_file = NULL;

void sys_error ( const char * message )
{
    int err = errno;
    if ( err_file == NULL )
    {
        char name [100000];
	sfprint ( name,"%s/error.log", log_directory );
        err_file = fopen ( name, "a" );
	if ( err_file == NULL )
	    exit ( errno );
    }
    fprint ( err_file, "sys_error: while %s: %s\n",
                       message, strerror ( err ) );

    if ( log_file != NULL )
	fprint ( log_file, "sys_error: while %s: %s\n",
			   message, strerror ( err ) );
    exit ( err );
}



// Reckon Server
// ------ ------

int server ( void )
{
    int fd = socket ( AR_INET, SOCK_STREAM, 0 );
    if ( fd < 0 ) sys_error ( "opening TCP socket" );
