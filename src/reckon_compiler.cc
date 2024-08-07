// Reckon Compiler
//
// File:	reckon_compiler.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Fri Aug  9 05:35:47 EDT 2024
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
# include <ll_parser_primary.h>
# include <mex.h>
# include <mexcom.h>
# include <mexstack.h>
# define REC reckon
# define PAR ll::parser
# define PRIM ll::parser::primary

static min::locatable_gen opening_quote;
static min::locatable_gen equal_sign;
static min::locatable_gen next;

bool static compile_restricted_statement
	( min::gen statement );

bool static compile_assignment_statement
	( min::gen left_side,
          min::gen right_side );

bool static compile_expression
	( min::gen expression,
          min::phrase_position pp,
	  min::gen name = min::NONE() );

bool static compile_constant
	( min::gen expression,
          min::phrase_position pp,
          min::gen type = min::NONE(),
	  min::gen name = min::NONE() );

static void initialize ( void )
{
    ::opening_quote = min::new_str_gen ( "`" );
    ::equal_sign = min::new_str_gen ( "=" );
    ::next = min::new_str_gen ( "next" );
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

    mexstack::init();
    mexstack::print_switch = print_switch;
}

bool REC::compile_statement ( min::gen statement )
{
    if ( ::compile_restricted_statement( statement ) )
	return true;

    min::phrase_position_vec ppv =
	min::get ( statement, min::dot_position );
    mexcom::compile_error
	( ppv->position,
          "cannot understand statement" );
    return false;
}

bool static compile_restricted_statement
	( min::gen statement )
{
    min::phrase_position_vec ppv =
	min::get ( statement, min::dot_position );
    min::obj_vec_ptr vp = statement;

    min::uns32 vsize = min::size_of ( vp );

    if ( vsize == 1
         &&
	 ::compile_expression ( vp[0], ppv[0] ) )
        return true;

    if ( vsize == 3
         &&
	 vp[1] == ::equal_sign
	 &&
	 ::compile_assignment_statement
	     ( vp[0], vp[2] ) )
	return true;

    return false;
}

bool static compile_assignment_statement
	( min::gen left_side,
          min::gen right_side )
{
    min::phrase_position_vec left_ppv =
	min::get ( left_side, min::dot_position );
    min::obj_vec_ptr left_vp = left_side;
    MIN_REQUIRE ( left_vp != min::NULL_STUB );
    min::uns32 s = min::size_of ( left_vp );
    bool next_present =
	( s >= 1 && left_vp[0] == ::next );
    min::uns32 i = next_present;
    if ( i >= s )
    {
	mexcom::compile_error
	    ( left_ppv->position,
              "variable name to right of `=' operator"
              " is empty" );
        return false;
    }
    min::locatable_gen var_name
	( PRIM::scan_var_name ( left_vp, i ) );
    if ( i < s )
    {
	mexcom::compile_error
	    ( left_ppv->position,
              "right side of `=' operator does not"
              " have the form `next VARIABLE-NAME'" );
        return false;
    }


    min::phrase_position_vec right_ppv =
	min::get ( right_side, min::dot_position );
    if ( ! :: compile_expression
	    ( right_side, right_ppv->position ) )
	return false;
}

bool static compile_expression
	( min::gen expression,
          min::phrase_position pp,
	  min::gen name )
{
    if ( ! min::is_obj ( expression ) )
	return ::compile_constant
	    ( expression, pp, min::NONE(), name );

    min::obj_vec_ptr vp = expression;
    min::attr_ptr ap = vp;

    min::locate ( ap, min::dot_type );
    min::gen type = min::get ( ap );
    if ( type != min::NONE() )
	return ::compile_constant
	    ( expression, pp, type, name );

    min::locate ( ap, min::dot_initiator );
    min::gen initiator = min::get ( ap );
    if ( initiator == ::opening_quote )
	return ::compile_constant
	    ( expression, pp, type, name );
    

     // Non-constant expressions are not compiled yet.
     //
     return false;
}

bool static compile_constant
	( min::gen expression,
          min::phrase_position pp,
          min::gen type,
	  min::gen name )
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
    // TBD push name into compiler variable stack
    ++ mexstack::var_stack_length;
    mexstack::push_instr ( instr, pp, name );
    return true;
}
