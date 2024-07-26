// Reckon Compiler
//
// File:	reckon_compiler.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Thu Jul 25 22:30:27 EDT 2024
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

void REC::init_compiler
	( PAR::parser parser,
          mexstack::print print_switch )
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

    mexstack::print_switch = print_switch;
}

void REC::compile_statement ( min::gen statement )
{
    min::phrase_position_vec ppv =
	min::get ( statement, min::dot_position );
    min::phrase_position pp = ppv->position;
    min::obj_vec_ptr vp = statement;
    mex::instr instr =
	{ mex::PUSHI, 0, 0, 0, 0, 0, 0, vp[0] };
    min::locatable_gen name
	( min::new_str_gen ( "*" ) );
    mexstack::push_instr ( instr, pp, name );
}
