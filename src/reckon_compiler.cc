// Reckon Compiler
//
// File:	reckon_compiler.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Sun Jul 28 04:32:14 EDT 2024
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

static min::locatable_gen opening_quote;

static void initialize ( void )
{
    ::opening_quote = min::new_str_gen ( "`" );
}
static min::initializer initializer ( ::initialize );

void REC::init_compiler
	( PAR::parser parser,
          mexstack::print print_switch )
{
    MIN_REQUIRE
        ( parser->input_file != min::NULL_STUB );

    mex::default_printer = parser->printer;
    mexcom::input_file = parser->input_file;
    min::init_printer
	( mexcom::input_file, mex::default_printer );
    mexcom::output_module = (mex::module_ins)
	mex::create_module ( mexcom::input_file );

    mexcom::error_count = 0;
    mexcom::warning_count = 0;
    mexstack::init();

    mexstack::print_switch = print_switch;
}

bool static compile_expression
	( min::gen expression,
          min::phrase_position pp );

void REC::compile_statement ( min::gen statement )
{
    min::phrase_position_vec ppv =
	min::get ( statement, min::dot_position );
    min::obj_vec_ptr vp = statement;

    if ( min::size_of ( vp ) == 1
         &&
	 ::compile_expression ( vp[0], ppv[0] ) )
        return;

    mexcom::compile_error
	( ppv->position, "cannot understand statement" );
}

bool static compile_constant
	( min::gen expression,
          min::phrase_position pp,
          min::gen type )
{
    if ( type == min::doublequote )
    {
	expression =
	    PAR::quoted_string_value ( expression );
        if ( expression == min::NONE() )
	    return false;
    }

    mex::instr instr =
	{ mex::PUSHI, 0, 0, 0, 0, 0, 0, expression };
    min::locatable_gen name
	( min::new_str_gen ( "*" ) );
    mexstack::push_instr ( instr, pp, name );
    return true;
}

bool static compile_expression
	( min::gen expression,
          min::phrase_position pp )
{
    min::gen type = min::get
	( expression, min::dot_type );
	// Returns NONE for non-objects and objects with
	// no type.
    if ( type != min::NONE()
         ||
	 min::is_num ( expression ) )
	return ::compile_constant
	    ( expression, pp, type );

    min::gen initiator = min::get
	( expression, min::dot_initiator );
    if ( initiator == ::opening_quote )
	return ::compile_constant
	    ( expression, pp, type );
    

     // Non-constant expressions are not compiled yet.
     //
     return false;
}
