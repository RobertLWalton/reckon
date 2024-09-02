// Reckon Compiler
//
// File:	reckon_compiler.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Mon Sep  2 04:53:09 PM EDT 2024
//
// The authors have placed this program in the public
// domain; they make no warranty and accept no liability
// for this program.

// Table of Contents
//
//	Usage and Setup
//	Helper Functions
//	Init Compiler
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
# define PARLEX ll::parser::lexeme
# define TAB ll::parser::table
# define PRIM ll::parser::primary

static min::locatable_gen opening_quote;
static min::locatable_gen equal_sign;
static min::locatable_gen next;
static min::locatable_gen star;
static min::locatable_gen eq;
static min::locatable_gen neq;
static min::locatable_gen gt;
static min::locatable_gen geq;
static min::locatable_gen lt;
static min::locatable_gen leq;
static min::locatable_gen TRUE;
static min::locatable_gen FALSE;

static min::uns32 jmp_counter = 1;

static min::locatable_var<PRIM::primary_pass>
	primary_pass;
static min::locatable_var<TAB::key_table> symbol_table;
static min::locatable_var<PAR::parser> parser;

bool static compile_restricted_statement
	( min::gen statement,
	  min::obj_vec_ptr vp,
	  min::uns32 vsize );

bool inline compile_restricted_statement
	( min::gen statement )
{
    min::obj_vec_ptr vp = statement;
    min::uns32 vsize = min::size_of ( vp );
    return ::compile_restricted_statement
    	( statement, vp, vsize );
}

bool static compile_assignment_statement
	( min::gen left_side,
          min::gen right_side );

// Compile expression.  If true_jmp == 0, generate code
// that pushes the value of the expression to the stack.
// In this case, on success 1 is returned and var_stack_
// length is incremented, but on failure compile_error
// is called and 0 is returned.  Here `name' is only
// used in the trace_info of the compiled instruction
// that pushes the expression value, and a top level
// CALLER is expected the corresponding var into the
// symbol_table.
//
// If true_jmp != 0, generate code that computes a
// logical value and jumps to true_jmp if that value is
// true and false_jmp if the value is false.  The code
// may also fall through and not jump: if it does so
// when a true logical value is computed, the value
// of the true_jmp argument is returned, but if it does
// so when a false logical value is computed, the value
// of the false_jmp argument is returned.  0 is returned
// if compile_error has been called.
//
// Note that true/false_jmp are never 0.
//
min::uns32 static compile_expression
	( min::gen expression,
	  min::uns32 true_jmp = 0,
	  min::uns32 false_jmp = 0,
	  min::gen name = ::star );

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
    ::eq   = min::new_str_gen ( "==" );
    ::neq  = min::new_str_gen ( "!=" );
    ::gt   = min::new_str_gen ( ">" );
    ::geq  = min::new_str_gen ( ">=" );
    ::lt   = min::new_str_gen ( "<" );
    ::leq  = min::new_str_gen ( "<=" );
    ::TRUE  = min::new_str_gen ( "TRUE" );
    ::FALSE  = min::new_str_gen ( "FALSE" );
}
static min::initializer initializer ( ::initialize );


// Helper Functions
// ------ ---------

// Call mexstack::push_instr with a PUSHI instruction
// that pushes the indicated value and has name as
// its trace_info.  Increments var_stack_length.
//
inline void pushi ( min::gen value, 
                    const min::phrase_position pp,
	            min::gen name = ::star,
		    bool no_source = false )
{
    mex::instr instr =
	{ mex::PUSHI, 0, 0, 0, 0, 0, 0, value };
    ++ mexstack::var_stack_length;
    mexstack::push_instr ( instr, pp, name, no_source );
}

// Call mexstack::push_instr with a POPS instruction
// that just pops the top value.
//
inline void pop ( const min::phrase_position pp )
{
    mex::instr instr = { mex::POPS };
    -- mexstack::var_stack_length;
    mexstack::push_instr ( instr, pp );
}

// Call mexstack::push_jmp_instr.
//
inline void jmp ( min::uns32 target,
                  const min::phrase_position pp,
		  min::uns8 op_code = mex::JMP,
		  min::uns32 immedB = 0 )
{
    mex::instr i { op_code };
    i.immedB = immedB;
    min::locatable_gen t
        ( min::new_num_gen ( target ) );
    if ( op_code == mex::JMPF
         ||
	 op_code == mex::JMPT )
	-- mexstack::var_stack_length;
    else if ( op_code != mex::JMP )
	mexstack::var_stack_length -= 2;
    mexstack::push_jmp_instr ( i, t, pp );
    mexstack::var_stack_length += immedB;
}

// Call mexstack::jmp_target.
//
inline void label ( min::uns32 target )
{
    min::locatable_gen t
        ( min::new_num_gen ( target ) );
    mexstack::jmp_target ( t );
}

// The following does the work of compile_expression in
// the case of a LOGICAL_OPERATOR with a logical value
// requested.
//
min::uns32 static compile_logical
	( min::obj_vec_ptr vp,
	  min::phrase_position_vec ppv,
	  PRIM::func func,
	  min::uns32 true_jmp,
	  min::uns32 false_jmp );

// The following does the work of compile_expression in
// the case of a LOGICAL_OPERATOR with IF op code and
// a non-logical value requested.
//
min::uns32 static compile_if_to_value
	( min::obj_vec_ptr vp,
	  min::phrase_position_vec ppv,
	  PRIM::func func,
	  min::gen name );

// Init Compiler
// ---- --------

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

    ::pushi ( mex::FALSE, PAR::top_level_position,
              ::FALSE, true );
    PRIM::push_var ( ::FALSE,
		     PAR::ALL_SELECTORS,
		     PAR::top_level_position,
		     0, 0, 0,
		     mexstack::var_stack_length - 1,
		     min::new_stub_gen
		       ( mexcom::output_module ),
		     ::symbol_table );
    ::pushi ( mex::TRUE, PAR::top_level_position,
              ::TRUE, true );
    PRIM::push_var ( ::TRUE,
		     PAR::ALL_SELECTORS,
		     PAR::top_level_position,
		     0, 0, 0,
		     mexstack::var_stack_length - 1,
		     min::new_stub_gen
		       ( mexcom::output_module ),
		     ::symbol_table );
}


// Compile Statement
// ------- ---------

bool REC::compile_statement ( min::gen statement )
{
    min::obj_vec_ptr vp = statement;
    min::uns32 vsize = min::size_of ( vp );

    if ( vsize == 2
         &&
	    min::get ( vp[1], min::dot_terminator )
	 == min::INDENTED_PARAGRAPH() )
    {
        MIN_REQUIRE
	    ( min::get ( vp[1], min::dot_initiator )
	      ==
	      PARLEX::colon );
        min::obj_vec_ptr vp0 = vp[0];
	if ( vp0 == min::NULL_STUB )
	    goto COMPILE_RESTRICTED;
	min::uns32 vsize0 = min::size_of ( vp0 );
	if ( vsize0 == 2
	     &&
	     vp0[1] == PARLEX::equal )
	{
	    min::gen separator =
	        min::get ( vp0[0], min::dot_separator );
	    vp0 = vp0[0];

	    // TBD
	}
    }

COMPILE_RESTRICTED:
    return ::compile_restricted_statement
        ( statement, vp, vsize );
}

bool static compile_restricted_statement
	( min::gen statement,
	  min::obj_vec_ptr vp,
	  min::uns32 vsize )
{
    if ( vsize == 3
         &&
	 vp[1] == ::equal_sign )
	return ::compile_assignment_statement
	     ( vp[0], vp[2] );
    else
    {
	vp = min::NULL_STUB;
	min::phrase_position_vec ppv =
	    min::get ( statement, min::dot_position );
	min::uns8 level = mexstack::lexical_level;
	min::uns32 depth = mexstack::depth[level];
	if ( ::compile_expression ( statement ) )
	{
	    PRIM::push_var ( ::star,
			     PAR::ALL_SELECTORS,
			     ppv->position,
			     level, depth,
			     0,
			     mexstack::var_stack_length
			       - 1,
			     min::new_stub_gen
			       ( mexcom::
			           output_module ),
			     ::symbol_table );
	    return true;
	}
	else
	    return false;
    }
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
	    ( right_side, 0, 0,
	      write ? ::star : var_name ) )
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

min::uns32 static compile_expression
	( min::gen expression,
	  min::uns32 true_jmp,
	  min::uns32 false_jmp,
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
        return 0;
    }

    if ( s >= 1 && vp[0] == ::next )
    {
	mexcom::compile_error
	    ( ppv->position,
              "expression cannot begin with `next'" );
        return 0;
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
		    MIN_REQUIRE
			( ! (   var->flags
			      & PRIM::WRITABLE_VAR ) );
		}
		else
		{
		    mexcom::compile_error
			( ppv->position,
			  "cannot read write-only"
			  " variable" );
		    return 0;
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

	    goto RETURN_VALUE;
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
		if ( ! OK ) return 0;

		min::uns8 op_code =
		    (min::uns8) ( func->flags >> 24 );
		mex::instr instr =
		    { op_code, mex::T_AOP };
		mexstack::var_stack_length -= jend;;
		++ mexstack::var_stack_length;
		mexstack::push_instr
		    ( instr, ppv->position, ::star );
		goto RETURN_VALUE;
	    }
	    else if
	        ( func->flags & PRIM::LOGICAL_OPERATOR )
	    {
	        bool value_requested =
		    ( true_jmp == 0 );
		if ( value_requested
		     &&
		     (    ( func->flags >> 24 )
		       == PRIM::IF ) )
		    return ::compile_if_to_value
		        ( vp, ppv, func, name );

		if ( value_requested )
		{
		    true_jmp = ::jmp_counter ++;
		    false_jmp = ::jmp_counter ++;
		    ::pushi
		        ( mex::FALSE, ppv->position );
		}
		min::uns32 jmp =
		    ::compile_logical
		        ( vp, ppv, func,
			  true_jmp, false_jmp );
		if ( jmp == 0 )
		    return 0;
		else if ( ! value_requested )
		    return jmp;
		else if ( jmp == false_jmp )
		    ::jmp ( false_jmp, ppv->position );

		::label ( true_jmp );
		::pop ( ppv->position );
		::pushi ( mex::TRUE, ppv->position );
		::label ( false_jmp );

		return 1;
	    }
	    else if
	        ( func->flags & PRIM::OPERATOR_CALL )
	    {
	        i -= 2;
		min::uns8 op_code_1 =
		    (min::uns8) ( func->flags >> 24 );
		min::uns8 op_code_2 =
		    (min::uns8) ( func->flags >> 16 );
		min::gen op_1 = vp[1];
		bool OK =
		    ::compile_expression ( vp[i++] );
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
		if ( ! OK ) return 0;
		else goto RETURN_VALUE;
	    }
	}
    }

    if ( s == 1 )
    {
	bool OK;
	if ( ! min::is_obj ( vp[0] ) )
	    OK = ::compile_constant
		( vp[0], ppv[0],
                  min::NONE(), name );
	else
	{
	    min::obj_vec_ptr vp0 = vp[0];
	    min::attr_ptr ap0 = vp0;

	    min::locate ( ap0, min::dot_type );
	    min::gen type = min::get ( ap0 );
	    if ( type != min::NONE() )
		OK = ::compile_constant
		    ( vp[0], ppv[0], type, name );
	    else
	    {
		min::locate ( ap0, min::dot_initiator );
		min::gen initiator = min::get ( ap0 );
		if ( initiator == ::opening_quote )
		    OK = ::compile_constant
			( vp[0], ppv[0], type, name );
	    }
	}
	if ( ! OK ) return 0;
	else goto RETURN_VALUE;
    }

    mexcom::compile_error
	( ppv->position,
          "cannot understand expression" );
    return 0;

RETURN_VALUE:
    if ( true_jmp != 0 )
    {
	::jmp ( false_jmp, ppv->position,
		mex::JMPF );
	return true_jmp;
    }
    else
	return 1;
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
    min::uns32 s = min::size_of ( vp );
    MIN_REQUIRE ( s >= 2 );
    bool OK = true;
    switch ( func->flags >> 24 )
    {
    case PRIM::COMPARE:
    {
	MIN_REQUIRE ( ( s & 1 ) == 1 );
	if ( ! ::compile_expression ( vp[0] ) )
	    OK = false;
	min::uns32 immedB = 1;
        for ( min::uns32 i = 1; i < s; i += 2 )
	{
	    if ( ! ::compile_expression ( vp[i+1] ) )
	        OK = false;
	    min::gen op = vp[i];
	    min::uns8 op_code =
	        ( op == ::eq   ? mex::JMPNEQ :
	          op == ::neq  ? mex::JMPEQ :
	          op == ::gt   ? mex::JMPLEQ :
	          op == ::geq  ? mex::JMPLT :
	          op == ::lt   ? mex::JMPGEQ :
	          op == ::leq  ? mex::JMPGT :
		  0 );
	    MIN_REQUIRE ( op_code != 0 );
	    if ( i + 2 >= s ) immedB = 0;
	    ::jmp
	        ( false_jmp, ppv[i], op_code, immedB );
	}
	if ( OK ) return true_jmp;
	else return 0;
    }
    case PRIM::AND:
    {
	MIN_REQUIRE ( ( s & 1 ) == 1 );
	min::uns32 next_jmp = ::jmp_counter ++;
	min::uns32 fallthru_jmp =
	    ::compile_expression
		( vp[0], next_jmp, false_jmp );
	if ( fallthru_jmp == 0 ) OK = false;
        for ( min::uns32 i = 1; i < s; i += 2 )
	{
	    if ( fallthru_jmp == false_jmp )
		::jmp ( false_jmp, ppv[i-1] );
	    :: label ( next_jmp );
	    if ( i + 2 >= s )
	        next_jmp = true_jmp;
	    else
	        next_jmp = ::jmp_counter ++;
	    fallthru_jmp = compile_expression
		     ( vp[i+1], next_jmp, false_jmp );
	    if ( fallthru_jmp == 0 )
	        OK = false;
	}
	if ( OK ) return fallthru_jmp;
	else return 0;
    }
    case PRIM::OR:
    {
	MIN_REQUIRE ( ( s & 1 ) == 1 );
	min::uns32 next_jmp = ::jmp_counter ++;
	min::uns32 fallthru_jmp =
	    ::compile_expression
		( vp[0], true_jmp, next_jmp );
	if ( fallthru_jmp == 0 ) OK = false;
        for ( min::uns32 i = 1; i < s; i += 2 )
	{
	    if ( fallthru_jmp == true_jmp )
		::jmp ( true_jmp, ppv[i-1] );
	    :: label ( next_jmp );
	    if ( i + 2 >= s )
	        next_jmp = false_jmp;
	    else
	        next_jmp = ::jmp_counter ++;
	    fallthru_jmp = compile_expression
		     ( vp[i+1], true_jmp, next_jmp );
	    if ( fallthru_jmp == 0 ) OK = false;
	}
	if ( OK ) return fallthru_jmp;
	else return 0;
    }
    case PRIM::NOT:
    {
	MIN_REQUIRE ( s == 2 );
	return ::compile_expression
		     ( vp[1], false_jmp, true_jmp );
    }
    case PRIM::BUT_NOT:
    {
	MIN_REQUIRE ( s == 3 );
	min::uns32 next_jmp = ::jmp_counter ++;
	min::uns32 fallthru_jmp =
	    ::compile_expression
		( vp[0], next_jmp, false_jmp );
	if ( fallthru_jmp == 0 ) OK = false;
	if ( fallthru_jmp == false_jmp )
	    ::jmp ( false_jmp, ppv[0] );
	::label ( next_jmp );
	fallthru_jmp = ::compile_expression
		     ( vp[2], false_jmp, true_jmp );
	if ( fallthru_jmp == 0 ) OK = false;
	if ( OK ) return fallthru_jmp;
	else return 0;
    }
    case PRIM::IF:
    {
	MIN_REQUIRE ( s >= 5 );
	MIN_REQUIRE ( s % 4 == 1 );
	for ( min::uns32 i = 1; i < s; i += 4 )
	{
	    min::uns32 jmp0 = ::jmp_counter ++;
	    min::uns32 jmp1 = ::jmp_counter ++;
	    min::uns32 fallthru_jmp =
	        ::compile_expression
		     ( vp[i+1], jmp1, jmp0 );
	    if ( fallthru_jmp == 0 ) OK = false;
	    else if ( fallthru_jmp == jmp0 )
	        ::jmp ( jmp0, ppv[i+1] );
	    ::label ( jmp1 );
	    fallthru_jmp =
	        ::compile_expression
		     ( vp[i-1], true_jmp, false_jmp );
	    if ( fallthru_jmp == 0 ) OK = false;
	    else ::jmp ( fallthru_jmp, ppv[i-1] );
	    ::label ( jmp0 );
	}
	min::uns32 fallthru_jmp =
	    ::compile_expression
		 ( vp[s-1], true_jmp, false_jmp );
	if ( fallthru_jmp == 0 ) OK = false;
	if ( ! OK ) return 0;
	else return fallthru_jmp;
    }
    default:
    {
	mexcom::compile_error
	    ( ppv->position,
	      "cannot understand expression" );
	return 0;
    }
    }
}

min::uns32 static compile_if_to_value
	( min::obj_vec_ptr vp,
	  min::phrase_position_vec ppv,
	  PRIM::func func,
	  min::gen name )
{
    min::uns32 s = min::size_of ( vp );
    MIN_REQUIRE ( s >= 5 );
    MIN_REQUIRE ( s % 4 == 1 );
    bool OK = true;
    pushi ( min::UNDEFINED(),
            ppv->position, name );
        // Allocate place for result.
    min::uns32 done = ::jmp_counter ++;
    for ( min::uns32 i = 1; i < s; i += 4 )
    {
	min::uns32 jmp0 = ::jmp_counter ++;
	min::uns32 jmp1 = ::jmp_counter ++;
	min::uns32 fallthru_jmp =
	    ::compile_expression
		 ( vp[i+1], jmp1, jmp0 );
	if ( fallthru_jmp == 0 ) OK = false;
	else if ( fallthru_jmp == jmp0 )
	    ::jmp ( jmp0, ppv[i+1] );
	::label ( jmp1 );
	::pop ( ppv[i-1]);
	if ( ! ::compile_expression
		 ( vp[i-1], 0, 0, name ) )
	{
	    pushi ( min::UNDEFINED(),
		    ppv->position, name );
	    OK = false;
	}
	jmp ( done, ppv[i-1] );
	::label ( jmp0 );
    }
    ::pop ( ppv[s-1]);
    if ( ! ::compile_expression
	     ( vp[s-1], 0, 0, name ) )
    {
	pushi ( min::UNDEFINED(),
		ppv->position, name );
	OK = false;
    }
    ::label ( done );
    return OK;
}
