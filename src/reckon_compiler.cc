// Reckon Compiler
//
// File:	reckon_compiler.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Tue Jul 23 02:09:40 EDT 2024
//
// The authors have placed this program in the public
// domain; they make no warranty and accept no liability
// for this program.

// Table of Contents
//
//	Usage and Setup

// Usage and Setup
// ----- --- -----

# include <reckon.h>
# include <mex.h>
# include <mexcom.h>
# include <mexstack.h>
# define REC reckon
# define PAR ll::parser

void REC::init_compiler ( PAR::parser parser )
{
    MIN_REQUIRE
        ( parser->input_file != min::NULL_STUB );

    mex::default_printer = parser->printer;
    mexcom::input_file = parser->input_file;
    mexcom::output_module = (mex::module_ins)
	mex::create_module ( mexcom::input_file );

    mexcom::error_count = 0;
    mexcom::warning_count = 0;
    mexstack::init();
}
