// Reckon Compiler
//
// File:	reckon_compiler.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Sun Aug 25 03:20:28 AM EDT 2024
//
// The authors have placed this program in the public
// domain; they make no warranty and accept no liability
// for this program.

// Table of Contents
//
//	Usage and Setup
//	Helper Functions
//	Compile Statement

// Usage and Setup
// ----- --- -----

# include <reckon.h>
# include <ll_parser_primary.h>
# include <mex.h>
# include <mexcom.h>
# include <mexstack.h>
# define REC reckon
# define PAR ll::parser
# define TAB ll::parser::table
# define PRIM ll::parser::primary

static min::locatable_gen opening_quote;
static min::locatable_gen equal_sign;
static min::locatable_gen next;
static min::locatable_gen star;

static min::uns32 jmp_counter = 1;

static min::locatable_var<PRIM::primary_pass>
	primary_pass;
static min::locatable_var<TAB::key_table> symbol_table;
static min::locatable_var<PAR::parser> parser;
static min::locatable_gen FALSE
    ( min::new_num_gen ( 0 ) );
static min::locatable_gen TRUE
    ( min::new_num_gen ( 1 ) );

bool static compile_restricted_statement
	( min::gen statement );

bool static compile_assignment_statement
	( min::gen left_side,
          min::gen right_side );

bool static compile_expression
	( min::gen expression,
	  min::gen name = ::star );

min::uns32 static compile_logical
	( min::obj_vec_ptr vp,
	  min::phrase_position_vec ppv,
	  PRIM::func func,
	  min::uns32 true_jmp,
	  min::uns32 false_jmp );

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
    ::star = min::new_str_gen ( "*" );
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

    ::primary_pass = PRIM::init_primary ( parser );
    ::symbol_table = ::primary_pass->primary_table;


}


// Helper Functions
// ------ ---------

inline void pushi ( min::gen value, 
                    const min::phrase_position pp,
	            min::gen name = ::star )
{
    mex::instr instr =
	{ mex::PUSHI, 0, 0, 0, 0, 0, 0, value };
    ++ mexstack::var_stack_length;
    mexstack::push_instr ( instr, pp, name );
}

inline void jmp ( min::uns32 target,
                  const min::phrase_position pp,
	          min::uns8 op_code = mex::JMP )
{
    mex::instr i = { op_code, mex::T_JMPS };
    min::locatable_gen t
        ( min::new_num_gen ( target ) );
    mexstack::push_jmp_instr ( i, t, pp );
}

inline void label ( min::uns32 target )
{
    min::locatable_gen t
        ( min::new_num_gen ( target ) );
    mexstack::jmp_target ( t );
}


// Compile Statement
// ------- ---------

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
    min::obj_vec_ptr vp = statement;

    min::uns32 vsize = min::size_of ( vp );

    if ( vsize == 3
         &&
	 vp[1] == ::equal_sign )
	return ::compile_assignment_statement
	     ( vp[0], vp[2] );
    else
    {
	vp = min::NULL_STUB;
	return ::compile_expression ( statement );
    }

    return false;
}

bool static compile_assignment_statement
	( min::gen left_side,
          min::gen right_side )
{
    min::uns8 level = mexstack::lexical_level;
    min::uns32 depth = mexstack::depth[level];

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
              "variable name to left of `=' operator"
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
              " have the form `next? VARIABLE-NAME'" );
        return false;
    }

    PRIM::var var = (PRIM::var) TAB::find
	( var_name,
	  PRIM::VAR,
	  PAR::ALL_SELECTORS,
	  ::symbol_table );  

    // At most one of the following _OK's can be true;
    //
    bool non_next_OK =
	( var == min::NULL_STUB
	  ||
	  ( var->block_level >> 16 ) != level );
    bool next_OK =
	( var != min::NULL_STUB
	  &&
	  ( var->block_level >> 16 ) == level
          &&
	  ( var->block_level & 0xFFFF ) == depth );
    bool writable_OK =
	( var != min::NULL_STUB
	  &&
	  ( var->block_level >> 16 ) == level
          &&
	  ( var->block_level & 0xFFFF ) != depth
	  &&
	  ( var->flags & PRIM::WRITABLE_VAR ) );

    bool write = ( writable_OK
                   &&
		       ( var->flags & PRIM::NEXT_VAR )
                   == next_present );

    if ( write );
    else if ( next_present && ! next_OK )
    {
	mexcom::compile_error
	    ( left_ppv->position,
              "next variable `",
	      min::pgen_name ( var_name ),
	      "' has no predecessor of the same"
	      " variable name and lexical level"
	      " and depth" );
        return false;
    }
    else if ( ! next_present && ! non_next_OK )
    {
	mexcom::compile_error
	    ( left_ppv->position,
              "NON-next variable `",
	      min::pgen_name ( var_name ),
	      "' has a predecessor of the same"
	      " variable name and lexical level" );
        return false;
    }

    if ( ! :: compile_expression
	    ( right_side, var_name ) )
	return false;

    // If compile_expression returns true, it has
    // incremented var_stack_length.

    if ( write )
    {
	mex::instr instr =
	    { mex::POPS, mex::T_POP, 0, 0,
                mexstack::var_stack_length - 1
              - var->location };
	min::gen labv[2] = { ::star, var_name };
	min::locatable_gen trace_info
	    ( min::new_lab_gen ( labv, 2 ) );
	-- mexstack::var_stack_length;
	mexstack::push_instr
	    ( instr, left_ppv->position,
	      trace_info );
    }
    else
	PRIM::push_var ( var_name,
			 PAR::ALL_SELECTORS,
			 left_ppv->position,
			 level, depth,
			 next_present ?
			     PRIM::NEXT_VAR : 0,
			 mexstack::var_stack_length - 1,
			 min::new_stub_gen
			   ( mexcom::output_module ),
			 ::symbol_table );
    return true;
}

bool static compile_expression
	( min::gen expression,
	  min::gen name )
{
    min::obj_vec_ptr vp = expression;
    min::attr_ptr ap = vp;
    min::locate ( ap, min::dot_position );
    min::phrase_position_vec ppv = min::get ( ap );
    min::uns32 s = min::size_of ( vp );
    min::uns8 level = mexstack::lexical_level;

    if ( s == 0 )
    {
	mexcom::compile_error
	    ( ppv->position, "expression is empty" );
        return false;
    }

    if ( s >= 1 && vp[0] == ::next )
    {
	mexcom::compile_error
	    ( ppv->position,
              "expression cannot begin with `next'" );
        return false;
    }
    min::locatable_var<PRIM::argument_vector>
        argument_vector ( min::NULL_STUB );
    TAB::key_prefix key_prefix = min::NULL_STUB;

    min::uns32 i = 0;
    min::uns32 quoted_i;
    TAB::root root;

RETRY:

    if ( PRIM::scan_primary
	    ( vp, i, ppv, ::parser, PAR::ALL_SELECTORS,
	      key_prefix, root, quoted_i,
	      argument_vector,
	      ::symbol_table ) )
    {
        PRIM::var var = (PRIM::var) root;
	if ( var != min::NULL_STUB )
	{
	    if ( i < s ) goto RETRY;
	    if ( quoted_i < i ) goto RETRY;
	    if ( ( var->block_level >> 16 ) != level )
	        goto RETRY;

	    if ( var->flags & PRIM::WRITABLE_VAR )
	    {
		if ( var->flags & PRIM::NEXT_VAR )
		{
		    var = TAB::find_next
			( (TAB::root) var,
			  PRIM::VAR,
			  PAR::ALL_SELECTORS );
		    MIN_REQUIRE
			( var != min::NULL_STUB );
		}
		else
		{
		    mexcom::compile_error
			( ppv->position,
			  "cannot read write-only"
			  " variable" );
		    return false;
		}
	    }

	    mex::instr instr =
		{ mex::PUSHS, mex::T_PUSH, 0, 0,
		    mexstack::var_stack_length
		  - var->location - 1 };
	    min::gen labv[2] = { var->label, name };
	    min::locatable_gen trace_info
		( min::new_lab_gen ( labv, 2 ) );
	    ++ mexstack::var_stack_length;
	    mexstack::push_instr
		( instr, ppv->position, trace_info );
	    return true;
	}
        PRIM::func func = (PRIM::func) root;
	if ( func != min::NULL_STUB )
	{
	    if ( func->flags & PRIM::BUILTIN_FUNCTION )
	    {
	        min::uns32 jend =
		    argument_vector->length;
		bool OK = true;
		for ( min::uns32 j = 0; j < jend; ++ j )
		{
		    if ( ! ::compile_expression
		    		( argument_vector[j] ) )
		        OK = false;
		}
		if ( ! OK ) return false;

		min::uns8 op_code =
		    (min::uns8) ( func->flags >> 24 );
		mex::instr instr =
		    { op_code, mex::T_AOP };
		mexstack::var_stack_length -= jend;;
		++ mexstack::var_stack_length;
		mexstack::push_instr
		    ( instr, ppv->position, ::star );
		return true;
	    }
	    else if
	        ( func->flags & PRIM::LOGICAL_OPERATOR )
	    {
	        min::uns32 true_jmp = ::jmp_counter ++;
	        min::uns32 false_jmp = ::jmp_counter ++;
		min::uns32 jmp =
		    ::compile_logical
		        ( vp, ppv, func,
			  true_jmp, false_jmp );
		if ( jmp == true_jmp )
		{
		    ++ mexstack::var_stack_length;
		    ::label ( true_jmp );
		    ::pushi ( ::TRUE, ppv->position );
		    min::uns32 finish =
		        ::jmp_counter ++;
		    ::jmp ( finish, ppv->position );
		    ::label ( false_jmp );
		    ::pushi ( ::FALSE, ppv->position );
		    ::label ( finish );
		}
		else if ( jmp == false_jmp )
		{
		    ++ mexstack::var_stack_length;
		    ::label ( false_jmp );
		    ::pushi ( ::FALSE, ppv->position );
		    min::uns32 finish =
		        ::jmp_counter ++;
		    ::jmp ( finish, ppv->position );
		    ::label ( true_jmp );
		    ::pushi ( ::TRUE, ppv->position );
		    ::label ( finish );
		}
	    }
	    else if
	        ( func->flags & PRIM::OPERATOR_CALL )
	    {
	        i -= 2;
		min::uns32 s = min::size_of ( vp );
		min::uns8 op_code_1 =
		    (min::uns8) ( func->flags >> 24 );
		min::uns8 op_code_2 =
		    (min::uns8) ( func->flags >> 16 );
		min::gen op_1 = vp[1];
		bool OK = true;
		{
		    if ( ! ::compile_expression
		    		( vp[i++] ) )
		        OK = false;
		}
		while ( i < s )
		{
		    min::uns8 op_code =
		        ( vp[i++] == op_1 ?
			  op_code_1 :
			  op_code_2 );
		    if ( ! ::compile_expression
		    		( vp[i++] ) )
		        OK = false;
		    mex::instr instr =
			{ op_code, mex::T_AOP };
		    -- mexstack::var_stack_length;
		    mexstack::push_instr
			( instr, ppv->position,
			  ::star );
		}
		return OK;
	    }
	}
    }

    if ( s == 1 )
    {
	if ( ! min::is_obj ( vp[0] ) )
	    return ::compile_constant
		( vp[0], ppv[0],
                  min::NONE(), name );

	min::obj_vec_ptr vp0 = vp[0];
	min::attr_ptr ap0 = vp0;

	min::locate ( ap0, min::dot_type );
	min::gen type = min::get ( ap0 );
	if ( type != min::NONE() )
	    return ::compile_constant
		( vp[0], ppv[0], type, name );

	min::locate ( ap0, min::dot_initiator );
	min::gen initiator = min::get ( ap0 );
	if ( initiator == ::opening_quote )
	    return ::compile_constant
		( vp[0], ppv[0], type, name );
    }

    ++ mexstack::var_stack_length;
    mexcom::compile_error
	( ppv->position,
          "cannot understand expression" );
    return false;
}

bool static compile_constant
	( min::gen value,
          min::phrase_position pp,
          min::gen type,
	  min::gen name )
{
    if ( type == min::doublequote )
    {
	value =
	    PAR::quoted_string_value ( value );
        if ( value == min::NONE() )
	{
	    mexcom::compile_error
		( pp,
		  "cannot understand expression" );
	    return false;
	}
    }

    ::pushi ( value, pp, name );
    return true;
}

min::uns32 static compile_logical
	( min::obj_vec_ptr vp,
	  min::phrase_position_vec ppv,
	  PRIM::func func,
	  min::uns32 true_jmp,
	  min::uns32 false_jmp )
{
    // TBD
    return 0;
}
