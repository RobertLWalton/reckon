// Reckon Compiler
//
// File:	reckon_compiler.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Sun May 19 22:12:55 EDT 2024
//
// The authors have placed this program in the public
// domain; they make no warranty and accept no liability
// for this program.

// Table of Contents
//
//	Usage and Setup
//	Parser
//	Reformatters

// Usage and Setup
// ----- --- -----

# include <reckon.h>
# include <mex.h>
# define REC reckon
# define PAR ll::parser


// Reckon Compiler
// ------ --------

min::locatable_var<mex::module> REC::code;
min::uns64 REC::compile_errors;
min::uns64 REC::compile_warnings;

void REC::init_compiler ( PAR::parser parser )
{
    REC::compile_errors = 0;
    REC::compile_warnings = 0;

    MIN_REQUIRE
        ( parser->input_file != min::NULL_STUB );
    REC::code = mex::create_module
        ( parser->input_file );
}
