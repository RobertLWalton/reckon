// Reckon Compiler
//
// File:	reckon_compiler.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Wed Oct 23 03:26:55 AM EDT 2024
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
static min::locatable_gen ZERO;
static min::locatable_gen ONE;
static min::locatable_gen IF;
static min::locatable_gen ELSE_IF;
static min::locatable_gen ELSE;
static min::locatable_gen TOP;

static min::locatable_gen DO;
static min::locatable_gen REPEAT;
static min::locatable_gen WHILE;
static min::locatable_gen UNTIL;
static min::locatable_gen EXACTLY;
static min::locatable_gen AT_MOST;
static min::locatable_gen TIMES;
static min::locatable_gen iteration_ops;

static min::uns32 jmp_counter = 1;

static min::locatable_var<PRIM::primary_pass>
	primary_pass;
static min::locatable_var<PAR::parser> parser;
static min::locatable_var<TAB::key_table> symbol_table;
static min::uns32 var_stack_length = 0;

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
    ::ZERO  = min::new_num_gen ( 0 );
    ::ONE  = min::new_num_gen ( 1 );
    ::IF  = min::new_str_gen ( "if" );
    ::ELSE_IF  = min::new_lab_gen ( "else", "if" );
    ::ELSE  = min::new_str_gen ( "else" );
    ::TOP  = min::new_str_gen ( "TOP" );

    ::DO       = min::new_str_gen ( "do" );
    ::REPEAT   = min::new_str_gen ( "repeat" );
    ::WHILE    = min::new_str_gen ( "while" );
    ::UNTIL    = min::new_str_gen ( "until" );
    ::EXACTLY   = min::new_str_gen ( "exactly" );
    ::AT_MOST  = min::new_lab_gen ( "at", "most" );
    ::TIMES  = min::new_str_gen ( "times" );
    min::gen labv[6] =
        { ::DO, ::REPEAT, ::WHILE, ::UNTIL,
	        ::EXACTLY, ::AT_MOST };
    ::iteration_ops = min::new_lab_gen ( labv, 6 );
}
static min::initializer initializer ( ::initialize );


// Helper Functions
// ------ ---------

// Get dot_position given obj_vec_ptr.
//
inline min::phrase_position_vec
	get_position ( min::obj_vec_ptr & vp )
{
    min::attr_ptr ap = vp;
    min::locate ( ap, min::dot_position );
    return min::get ( ap );
}

// Push variable into symbol table.
//
inline void push_var ( PRIM::var var )
{
    PRIM::push_var ( ::symbol_table, var );
    ++ ::var_stack_length;
}

void mexstack::pop_stacks ( void )
{
    // TBD: functions not implemented.

    MIN_REQUIRE
        (    ::var_stack_length
	  >= mexstack::var_stack_length );
    while (   ::var_stack_length
            > mexstack::var_stack_length )
    {
        TAB::pop ( ::symbol_table );
        -- ::var_stack_length;
    }
}

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

// Call mexstack::push_jmp_instr.  Adjust mexstack::
// var_stack_length BEFORE calling this function to
// be correct if the jump is taken.  Re-adjust AFTER
// the call to this function to be correct if the
// jump is NOT taken.
//
inline void jmp ( min::uns32 target,
                  const min::phrase_position pp,
		  min::uns8 op_code = mex::JMP,
		  min::uns32 immedB = 0,
		  min::gen immedD = ::ZERO )
{
    mex::instr i
        { op_code, 0, 0, 0, 0, immedB, 0, immedD };
    min::locatable_gen t
        ( min::new_num_gen ( target ) );
    mexstack::push_jmp_instr ( i, t, pp );
}

// Call mexstack::jmp_target.
//
inline void label ( min::uns32 target )
{
    min::locatable_gen t
        ( min::new_num_gen ( target ) );
    mexstack::print_label ( t );
    mexstack::jmp_target ( t );
}

struct block_struct
{
    min::gen name;

    min::uns32 if_next_jmp;
    min::uns32 if_finish_jmp;
	// If if_next_jmp is != 0 the previous statement
	// was an if or else if statement which jumps to
	// next_jmp if its condition is false, with
	// previous if and else if statements in a
	// sequence ending with the previous statement
	// jumping to if_finish_jmp if one of there
	// conditions is true provided if_finish_jmp is
	// != 0.

    min::phrase_position if_pp;
        // Phrase position of last IF or ELSE-IF.

    min::uns32 block_finish_jmp;
        // If non-zero, label to be output right after
	// END... .
};

typedef min::packed_vec_insptr<block_struct>
    block_stack_vec;
static min::locatable_var<::block_stack_vec>
    block_stack;
static block_struct block;
    // Copy of top of block stack.  ::block.name is
    // protected from gc because it is in block stack.

inline void push_block ( min::gen name )
{
    block_struct b =
        { name, 0, 0, min::MISSING_POSITION, 0 };
    min::push ( ::block_stack ) = b;
    ::block = b;
    min::unprotected::acc_write_update
        ( ::block_stack, name );
}

inline void pop_block ( void )
{
    min::pop ( ::block_stack );
    ::block = ::block_stack[::block_stack->length - 1];
}

inline void end_if_sequence ( void )
{
    if ( block.if_next_jmp != 0 )
    {
        ::label ( block.if_next_jmp );
	block.if_next_jmp = 0;
    }
    if ( block.if_finish_jmp != 0 )
    {
        ::label ( block.if_finish_jmp );
	block.if_finish_jmp = 0;
    }
}

// Scan the name of an assignment left side variable
// and locate or create a var object for it.  Returns
// the var object found or created, or returns NULL_STUB
// if compile_error called.  If the var does NOT have
// the WRITABLE_VAR flag, the var has been created, and
// the caller of this function should assign the var's
// location and push the var into ::symbol_table at the
// appropriate time.  In this case, the returned var->
// location is NO_LOCATION unless the variable is a
// next variable, in which case it is the location of
// the predecessor of the variable.  In either case
// var->location must be overwritten by the caller.
//
// If the var returned has the WRITABLE_VAR flag, the
// var was found and the caller of this function should
// compile an instruction to move the appropriate
// runtime value to the var's location.
//
// The expession should be a MIN object whose elements
// are the variable name optionally preceeded by `next'.
//
// The returned var should be stored immediately in a
// min::locatable_var.
//
const min::uns32 NO_LOCATION = 0xFFFFFFFF;
PRIM::var scan_var ( min::gen expression )
{
    min::uns8 level = mexstack::lexical_level;
    min::uns32 depth = mexstack::depth[level];

    min::obj_vec_ptr vp = expression;
    MIN_REQUIRE ( vp != min::NULL_STUB );
    min::uns32 s = min::size_of ( vp );
    min::phrase_position_vec ppv =
        ::get_position ( vp );
    bool next_present = ( s >= 1 && vp[0] == ::next );
    min::uns32 i = next_present;
    if ( i >= s )
    {
	mexcom::compile_error
	    ( ppv->position,
              "variable name to left of `=' operator"
              " is empty" );
        return min::NULL_STUB;
    }
    min::locatable_gen var_name
	( PRIM::scan_var_name ( vp, i ) );
    if ( i < s )
    {
	mexcom::compile_error
	    ( ppv->position,
              "left side of `=' operator does not"
              " have the form `next? VARIABLE-NAME'" );
        return min::NULL_STUB;
    }

    min::locatable_var<PRIM::var> var
        ( (PRIM::var) TAB::find
	      ( var_name,
	        PRIM::VAR,
	        PAR::ALL_SELECTORS,
	        ::symbol_table ) );

    // At most one of the following _OK's can be true;
    //
    bool non_next_OK =
	( var == min::NULL_STUB
	  ||
	  ( var->block_level >> 16 ) != level );
    bool next_OK =
	( ! non_next_OK
          &&
	  ( var->block_level & 0xFFFF ) == depth );
    bool writable_OK =
	( ! non_next_OK
          &&
	  ( var->block_level & 0xFFFF ) != depth
	  &&
	  ( var->flags & PRIM::WRITABLE_VAR ) );

    bool write = ( writable_OK
                   &&
		      (bool)
		      ( var->flags & PRIM::NEXT_VAR )
                   == next_present );

    if ( write );
    else if ( next_present && ! next_OK )
    {
	mexcom::compile_error
	    ( ppv->position,
              "next variable `",
	      min::pgen_name ( var_name ),
	      "' has no predecessor of the same"
	      " variable name and lexical level"
	      " and depth" );
        return min::NULL_STUB;
    }
    else if ( ! next_present && ! non_next_OK )
    {
	mexcom::compile_error
	    ( ppv->position,
              "NON-next variable `",
	      min::pgen_name ( var_name ),
	      "' has a predecessor of the same"
	      " variable name and lexical level" );
        return min::NULL_STUB;
    }
    else
    {
	var = PRIM::create_var
	    ( var_name,
              PAR::ALL_SELECTORS,
              ppv->position,
              level, depth,
              next_present ? PRIM::NEXT_VAR : 0,
              next_present ?
	          var->location : NO_LOCATION,
              min::new_stub_gen
	          ( mexcom:: output_module ) );
    }

    return var;
}

// Return var->label with `next' prepended if it is a
// NEXT_VAR.
//
min::gen static full_var_name ( PRIM::var var )
{
    if ( ( var->flags & PRIM::NEXT_VAR ) == 0 )
        return var->label;
    min::uns32 lablen =
        min::is_lab ( var->label ) ?
	min::lablen ( var->label ) : 1;
    min::gen labv[lablen+1] = { ::next };
    if ( min::is_lab ( var->label ) )
	min::labncpy ( labv + 1, var->label, lablen );
    else
    	labv[1] = var->label;
    min::locatable_gen var_name
	( min::new_lab_gen ( labv, lablen + 1 ) );
    return var_name;
}

// Find all the expression assignment statement left-
// side next variables in a statement or block
// (including subblocks in both cases).  Results
// are added to the symbol_table.  A variable V is
// added as a WRITEABLE_VAR only if the symbol_table
// already contains a variable V with the same level
// and depth and without WRITABLE_VAR.
//
static void search_block ( min::gen statement );
static void search_statement
    ( min::gen statement, bool is_restricted = false );
inline void add_next_variable
    ( min::gen var_name, min::phrase_position pp )
{

    min::locatable_var<PRIM::var> var
        ( (PRIM::var) TAB::find
	      ( var_name,
	        PRIM::VAR,
	        PAR::ALL_SELECTORS,
	        ::symbol_table ) );

    min::uns8 level = mexstack::lexical_level;
    min::uns32 depth = mexstack::depth[level];

    if ( var == min::NULL_STUB
	 ||
	 ( var->block_level >> 16 ) != level
	 ||
	 ( var->block_level & 0xFFFF ) != depth
         ||
	 ( var->flags & PRIM::WRITABLE_VAR ) )
        return;

    min::locatable_var<PRIM::var> next_var =
	PRIM::create_var
	    ( var_name,
              PAR::ALL_SELECTORS,
              pp,
              level, depth,
              PRIM::NEXT_VAR | PRIM::WRITABLE_VAR,
	      mexstack::var_stack_length,
              min::new_stub_gen
	          ( mexcom:: output_module ) );

    ++ mexstack::var_stack_length;
    mexstack::push_push_instr
	( next_var->label, var->label,
	  var->location, pp );
    ::push_var ( next_var );

    if ( REC::warn_next_variable_promotion )
        mexcom::compile_warn
	    ( pp,
	      "next variable promoted to depth ",
	      min::puns ( depth, "%u" ) );
}
    
static void search_statement
    ( min::gen statement, bool is_restricted )
{
    min::obj_vec_ptr vp = statement;
    min::uns32 s = min::size_of ( vp );

    if ( s == 3
         &&
	 vp[1] == ::equal_sign )
    {
        min::gen separator =
	    min::get ( vp[0], min::dot_separator );
	min::obj_vec_ptr vp0 = vp[0];
	min::uns32 s0 = min::size_of ( vp0 );
	min::phrase_position_vec ppv0 =
	    ::get_position ( vp0 );

	if ( separator  == PARLEX::comma )
	{
	    for ( min::uns32 i = 0; i < s0; ++ i )
	    {
	        min::obj_vec_ptr vpi = vp0[i];
		min::uns32 si = min::size_of ( vpi );
		if ( si < 2 || vpi[0] != ::next )
		    continue;
		min::uns32 j = 1;
		min::locatable_gen var_name
		    ( PRIM::scan_var_name ( vpi, j ) );
		if ( j < si ) continue;
		add_next_variable ( var_name, ppv0[i] );
	    }
	    return;
	}
	else
	{
	    if ( s0 < 2 || vp0[0] != ::next )
		return;
	    min::uns32 j = 1;
	    min::locatable_gen var_name
		( PRIM::scan_var_name ( vp0, j ) );
	    if ( j < s0 ) return;
	    add_next_variable
	        ( var_name, ppv0->position );
	    return;
	}
    }

    if ( is_restricted ) return;

    if ( s == 4
         &&
	 ( vp[0] == ::IF || vp[0] == ::ELSE_IF )
	 &&
	 vp[2] == PARLEX::colon )
        ::search_statement ( vp[3], true );
    else if ( s == 3
              &&
	      vp[0] == ::ELSE 
	      &&
	      vp[1] == PARLEX::colon )
        ::search_statement ( vp[2], true );
    else if (    min::get
                     ( vp[s-1], min::dot_terminator )
	      == min::INDENTED_PARAGRAPH() )
    {
        if ( s == 3 && vp[0] == ::IF )
	    ::search_block ( vp[2] );
        else if ( s == 3 && vp[0] == ::ELSE_IF )
	    ::search_block ( vp[2] );
        else if ( s == 2 && vp[0] == ::ELSE )
	    ::search_block ( vp[1] );
	else if ( s == 2 )
	{
	    min::obj_vec_ptr vp0 = vp[0];
	    min::uns32 s0 = min::size_of (vp0 );
	    if ( s0 >= 2 && vp0[1] == ::equal_sign )
		::search_block ( vp[1] );
	}
    }
}

static void search_block ( min::gen block )
{
    min::obj_vec_ptr vp = block;
    min::uns32 s = min::size_of ( vp );
    for ( min::uns32 i = 0; i < s; ++ i )
        ::search_statement ( vp[i] );
}

// Compile Functions
// ------- ---------

bool static compile_restricted_statement
	( min::gen statement );

// Compile statement of form
// 	vp[0] "=" vp[1]
//
bool static compile_expression_assignment_statement
	( min::obj_vec_ptr & vp,
	  bool is_restricted = false );

struct iteration
{
    min::gen type;	  // iteration type:
    			  //     WHILE, UNTIL, TIMES;
			  //     MISSING for last
			  //     element
    min::gen exp;	  // expression to compile
    min::phrase_position pp; // position of exp
    min::uns32 location;  // stack location of counter
    			  // for times
};


bool static compile_block
        ( min::gen block,
	  min::gen label = min::MISSING(),
	      // do label
	  min::uns32 nnext = 0,
	      // number `next' variables for loop
	  iteration * iterations = NULL
	      // NULL if not loop
	);

bool static compile_block_assignment_statement
	( min::gen left_side,
          min::gen right_side,
          min::gen block );

// If ::block.if_next_jmp == 0, compiles IF statement.
// Otherwise if condition != NONE, compiles ELSE_IF
// statement.  Otherwise if condition == NONE, compiles
// ELSE statement.
//
bool static compile_if_statement
	( min::gen condition,
          min::gen block,
	  bool is_restricted = false );

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
// Note that in this case no label for true_jmp or
// false_jmp is output; the caller must do this.
//
// Note that the value of true/false_jmp is never 0, so
// a returned value of 0 always indicates an error.
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

static min::uns32 block_stack_gen_disp[] = {
    min::DISP ( & ::block_struct::name ),
    min::DISP_END };

static min::packed_vec<::block_struct>
    block_stack_type
        ( "block_stack_type",
	  ::block_stack_gen_disp );

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

    min::locatable_var<PRIM::var> var;
    ::pushi ( mex::FALSE, PAR::top_level_position,
              ::FALSE, true );
    var = PRIM::create_var
        ( ::FALSE,
          PAR::ALL_SELECTORS,
          PAR::top_level_position,
          0, 0, 0,
          mexstack::var_stack_length - 1,
          min::new_stub_gen ( mexcom::output_module ) );
    ::push_var ( var );
    ::pushi ( mex::TRUE, PAR::top_level_position,
              ::TRUE, true );
    var = PRIM::create_var
        ( ::TRUE,
          PAR::ALL_SELECTORS,
          PAR::top_level_position,
          0, 0, 0,
          mexstack::var_stack_length - 1,
          min::new_stub_gen ( mexcom::output_module ) );
    ::push_var ( var );

    ::block_stack = ::block_stack_type.new_stub ( 64 );
    ::push_block ( ::TOP );
}


// Compile Statement
// ------- ---------

bool REC::compile_statement ( min::gen statement )
{
    min::obj_vec_ptr vp = statement;
    min::uns32 s = min::size_of ( vp );

    // Else-if/else statements must be first so
    // end_if_sequence can be called for all other
    // statements.

    if ( s == 4
         &&
	 vp[0] == ::ELSE_IF
	 &&
	 vp[2] == PARLEX::colon )
    {
        if ( ::block.if_next_jmp == 0 )
	{
	    mexcom::compile_error
	        ( ::get_position(vp)->position,
		  " else if statement does NOT follow"
		  " if or else if statement" );
	    return false;
	}
	return ::compile_if_statement
	    ( vp[1], vp[3], true );
    }

    if ( s == 3
         &&
	 vp[0] == ::ELSE
	 &&
	 vp[1] == PARLEX::colon )
    {
        if ( ::block.if_next_jmp == 0 )
	{
	    mexcom::compile_error
	        ( ::get_position(vp)->position,
		  " else statement does NOT follow"
		  " if or else if statement" );
	    return false;
	}
        return ::compile_if_statement
	    ( min::NONE(), vp[2], true );
    }

    if (     min::get ( vp[s-1], min::dot_terminator )
	 == min::INDENTED_PARAGRAPH() )
    {
        MIN_REQUIRE
	    ( min::get ( vp[s-1], min::dot_initiator )
	      ==
	      PARLEX::colon );

	// : paragraph is postfix so s >= 2.

	// Else-if/else statements must be first so
	// end_if_sequence can be called for all other
	// statements.

	if ( s == 3
	     &&
	     vp[0] == ::ELSE_IF )
	{
	    if ( ::block.if_next_jmp == 0 )
	    {
		mexcom::compile_error
		    ( ::get_position(vp)->position,
		      " else if statement does NOT"
		      " follow if or else if"
		      " statement" );
		return false;
	    }
	    return ::compile_if_statement
		( vp[1], vp[2] );
	}

	if ( s == 2
	     &&
	     vp[0] == ::ELSE )
	{
	    if ( ::block.if_next_jmp == 0 )
	    {
		mexcom::compile_error
		    ( ::get_position(vp)->position,
		      " else statement does NOT follow"
		      " if or else if statement" );
		return false;
	    }
	    return ::compile_if_statement
		( min::NONE(), vp[1] );
	}

        ::end_if_sequence();

	if ( s == 3
	     &&
	     vp[0] == ::IF )
	{
	    return ::compile_if_statement
		( vp[1], vp[2] );
	}

	if ( s != 2 ) goto NOT_LEGAL_STATEMENT;

        min::obj_vec_ptr vp0 = vp[0];
	if ( vp0 == min::NULL_STUB )
	    goto NOT_LEGAL_STATEMENT;
	min::uns32 s0 = min::size_of ( vp0 );
	if ( s0 == 0 ) goto NOT_LEGAL_STATEMENT;

	if ( s0 >= 2 && vp0[1] == PARLEX::equal )
	{
	    MIN_REQUIRE ( s0 <= 3 );
	    min::gen right_side =
	        ( s0 == 3 ? vp0[2] : min::NONE() );
	    return ::compile_block_assignment_statement
	        ( vp0[0], right_side, vp[1] );
	}

	if (    min::labfind ( vp0[0], ::iteration_ops )
	     != -1 )
	{
	    vp0 = min::NULL_STUB;
	    return ::compile_block_assignment_statement
	        ( min::NONE(), vp[0], vp[1] );
	}

    }
    else
        ::end_if_sequence();

    if ( s == 4
         &&
	 vp[0] == ::IF
	 &&
	 vp[2] == PARLEX::colon )
        return ::compile_if_statement
	    ( vp[1], vp[3], true );

    if ( s == 3
         &&
	 vp[1] == ::equal_sign )
	return ::compile_expression_assignment_statement
	     ( vp );

    else
    {
	min::phrase_position_vec ppv =
	    ::get_position ( vp );
	min::uns8 level = mexstack::lexical_level;
	min::uns32 depth = mexstack::depth[level];
	vp = min::NULL_STUB;
	if ( ::compile_expression ( statement ) )
	{
	    min::locatable_var<PRIM::var> var;
	    var = PRIM::create_var
	        ( ::star,
	          PAR::ALL_SELECTORS,
	          ppv->position,
	          level, depth,
	          0,
	          mexstack::var_stack_length - 1,
	          min::new_stub_gen
	            ( mexcom::output_module ) );
	    ::push_var ( var );
	    return true;
	}
	else
	    return false;
    }

NOT_LEGAL_STATEMENT:

    mexcom::compile_error
	( ::get_position(vp)->position,
	  "not a legal statement;"
	  " statement ignored" );
    return false;
}

bool static compile_restricted_statement
	( min::gen statement )
{
    min::obj_vec_ptr vp = statement;
    min::uns32 s = min::size_of ( vp );

    if ( s == 3
         &&
	 vp[1] == ::equal_sign )
	return ::compile_expression_assignment_statement
	     ( vp, true );

    mexcom::compile_error
	( ::get_position(vp)->position,
	  "not a legal restricted statement;"
	  " statement ignored" );
    return false;
}

bool static compile_expression_assignment_statement
	( min::obj_vec_ptr & vp,
	  bool is_restricted )
{

    min::gen left_separator =
	min::get ( vp[0], min::dot_separator );
    min::obj_vec_ptr left_vp = vp[0];
    min::uns32 left_n = 1;
    if ( left_separator == PARLEX::comma )
	left_n = min::size_of ( left_vp );

    min::gen right_separator =
	min::get ( vp[2], min::dot_separator );
    min::obj_vec_ptr right_vp = vp[2];
    min::uns32 right_n = 1;
    if ( right_separator == PARLEX::comma )
	right_n = min::size_of ( right_vp );

    if ( left_n != right_n )
    {
        mexcom::compile_error
	    ( ::get_position(vp)->position,
	      left_n < right_n ?
	      "too few variables, too many expressions"
	      " in expression assigment statement;" :
	      "too many variables, too few expressions"
	      " in expression assigment statement;",
	      min::pnop,
	      " statement ignored" );
	return false;
    }

    min::locatable_var<PRIM::var> vars[left_n];
    bool OK = true;
    if ( left_separator != PARLEX::comma )
    {
	left_vp = min::NULL_STUB;
	vars[0] = ::scan_var ( vp[0] );
	if ( vars[0] == min::NULL_STUB )
	    OK = false;
    }
    else for ( min::uns32 i = 0; i < left_n; ++ i )
    {
	vars[i] = ::scan_var ( left_vp[i] );
	if ( vars[i] == min::NULL_STUB )
	    OK = false;
    }
    if ( ! OK )
    {
        mexcom::compile_error
	    ( ::get_position ( vp )->position,
	      "statement ignored due to previous"
	      " errors" );
	return false;
    }

    for ( min::uns32 i = 0; i < left_n; ++ i )
    {
        PRIM::var var = vars[i];
	if ( var->flags & PRIM::WRITABLE_VAR )
	    /* Do nothing */;
	else if ( is_restricted )
	{
	    min::locatable_gen var_name
		( ::full_var_name ( var ) );
	    mexcom::compile_error
		( var->position,
		  "cannot allocate variable (",
		  min::pgen_name ( var_name ),
		  ") in a RESTRICTED statement;"
		  " variable ignored" );
	}
	else
	{
	    min::locatable_gen var_name
		( ::full_var_name ( var ) );
	    ::pushi
	        ( ::ZERO, var->position, var_name );
	    var->location =
	        mexstack::var_stack_length - 1;
	}
    }


    // OK == true
    if ( right_separator != PARLEX::comma )
    {
	right_vp = min::NULL_STUB;
	if ( ! :: compile_expression ( vp[2] ) )
	{
	    OK = false;
	    pushi
	        ( min::UNDEFINED(),
		  ::get_position(right_vp)->position );
	}
    }
    else
    for ( min::uns32 i = 0; i < right_n; ++ i )
    {
	if ( ! :: compile_expression ( right_vp[i] ) )
	{
	    OK = false;
	    pushi ( min::UNDEFINED(),
		    ::get_position(right_vp)[i] );
	}
    }

    for ( min::uns32 i = left_n; 0 < i; )
    {
        PRIM::var var = vars[--i];
	if ( is_restricted
	     &&
	     ! ( var->flags & PRIM::WRITABLE_VAR ) )
	    ::pop ( var->position );
	else
	{
	    min::locatable_gen var_name
		( ::full_var_name ( var ) );
	    min::gen labv[2] = { ::star, var_name };
	    min::locatable_gen trace_info
		( min::new_lab_gen ( labv, 2 ) );
	    mex::instr instr =
		{ mex::POPS, mex::T_POP, 0, 0,
		    mexstack::var_stack_length - 1
		  - var->location };
	    -- mexstack::var_stack_length;
	    mexstack::push_instr
		( instr, var->position,
		  trace_info );
	}
    }

    for ( min::uns32 i = 0; i < left_n; ++ i )
    {
        PRIM::var var = vars[i];
	if ( ! is_restricted
	     &&
	     ! ( var->flags & PRIM::WRITABLE_VAR ) )
	    ::push_var ( var );
    }

    return OK;
}

bool static compile_block
        ( min::gen block,
	  min::gen label,
	  min::uns32 nnext,
	  iteration * iterations )
{
    if ( iterations != NULL )
    {
        PRIM::var vars[nnext];

	TAB::root r = TAB::top ( ::symbol_table );
	for ( min::uns32 i = nnext; 0 < i; -- i )
	{
	    vars[i-1] = (PRIM::var) r;
	    MIN_REQUIRE ( vars[i-1] != min::NULL_STUB );
	    r = TAB::previous ( r );
	}
	for ( min::uns32 i = 0; i < nnext; ++ i )
	{
	    PRIM::var var = PRIM::create_var
		( vars[i]->label,
		  PAR::ALL_SELECTORS,
		  vars[i]->position,
		  vars[i]->block_level >> 16,
		  vars[i]->block_level & 0xFFFF,
		  PRIM::NEXT_VAR + PRIM::WRITABLE_VAR,
		  mexstack::var_stack_length ++,
		  min::new_stub_gen
		      ( mexcom:: output_module ) );
	    vars[i]->flags &= ~ PRIM::WRITABLE_VAR;
	    ::push_var ( var );
	}
    }

    min::phrase_position_vec ppv =
        min::get ( block, min::dot_position );
    mex::instr beg =
        { iterations == NULL ?
	  mex::BEG : mex::BEGL };
    mexstack::begx
        ( beg, nnext, 0,
	  min::MISSING(), ppv->position );
    push_block ( label );
    ::block.block_finish_jmp = ::jmp_counter ++;

    bool OK = true;

    if ( iterations != NULL )
        while ( iterations->type != min::MISSING() )
    {
        if ( iterations->type == ::WHILE )
	{
	    min::uns32 true_jmp = ::jmp_counter ++;
	    min::uns32 fallthru_jmp =
	        ::compile_expression
		    ( iterations->exp,
		      true_jmp,
		      ::block.block_finish_jmp );
	    if ( fallthru_jmp == 0 ) OK = false;
	    else if ( fallthru_jmp != true_jmp ) 
		::jmp ( ::block.block_finish_jmp,
		        iterations->pp );
	    ::label ( true_jmp );
	}
        else if ( iterations->type == ::UNTIL )
	{
	    min::uns32 false_jmp = ::jmp_counter ++;
	    min::uns32 fallthru_jmp =
	        ::compile_expression
		    ( iterations->exp,
		      ::block.block_finish_jmp,
		      false_jmp );
	    if ( fallthru_jmp == 0 ) OK = false;
	    else if ( fallthru_jmp != false_jmp ) 
		::jmp ( ::block.block_finish_jmp,
		        iterations->pp );
	    ::label ( false_jmp );
	}
        else if ( iterations->type == ::TIMES )
	{
	    ::jmp ( ::block.block_finish_jmp,
	            iterations->pp, mex::JMPCNT,
		      mexstack::var_stack_length
		    - iterations->location - 1,
		    ::ONE );
	}
        ++ iterations;
    }

    min::obj_vec_ptr vp = block;
    min::uns32 s = min::size_of ( vp );
    for ( min::uns32 i = 0; i < s; ++ i )
    {
        if ( ! REC::compile_statement ( vp[i] ) )
	    OK = false;
    }

    ::end_if_sequence();

    mex::instr end =
        { iterations != NULL ? mex::ENDL : mex::END };
    mexstack::endx
        ( end, 0, min::MISSING(), ppv->position );
    if ( ::block.block_finish_jmp != 0 )
        ::label ( ::block.block_finish_jmp );
    pop_block();

    return OK;
}

bool static compile_block_assignment_statement
	( min::gen left_side,
          min::gen right_side,
          min::gen block )
{
    bool OK = true;

    // Process right side.  Do this first to get context
    // of `xxx times' subexpressions correct.
    //
    min::uns32 number_of_iterations = 0;
    min::obj_vec_ptr right_vp = min::NULL_STUB;
    if ( right_side != min::NONE() )
    {
        right_vp = right_side;
	number_of_iterations =
	    min::size_of ( right_vp );
		// Overestimate
    }
    iteration iter[number_of_iterations];

    min::locatable_gen label ( min::MISSING() );
    iteration * iterations = NULL;
    if ( number_of_iterations > 0 )
    {
	min::uns32 s = min::size_of ( right_vp );
	min::phrase_position_vec right_ppv =
	    ::get_position ( right_vp );
	number_of_iterations = 0;
	for ( min::uns32 i = 0; i < s; )
	{
	    min::gen op = right_vp[i++];
	    if ( op == ::DO || op == ::REPEAT )
	    {
	        if ( i != 1 )
		    mexcom::compile_warn
			( right_ppv[i-1], "",
			  min::pgen_name
			      ( right_vp[i-1] ),
			  " should be at beginning of"
			  " iterations list" );
		if (    i + 1 < s
		     && right_vp[i+1] == ::TIMES )
		    goto TIMES_FOUND;
		if ( op == ::REPEAT )
		    iterations = iter;
		if (    i >= s
		     || ! min::is_obj ( right_vp[i] ) )
		    continue;
		min::obj_vec_ptr labvp = right_vp[i];
		min::uns32 j = 0;
		label =
		    PAR::scan_simple_name ( labvp, j );
		if ( j < min::size_of ( labvp )
		     ||
		     label == min::NONE() )
		{
		    mexcom::compile_error
			( right_ppv[i],
			  "expression following ",
			  min::pgen_name ( op ),
			  " is not a block label" );
		    goto ERROR_SKIP;
		}
		++ i;
	    }
	    else if ( op == ::WHILE || op == ::UNTIL )
	    {
	        if ( i >= s )
		{
		    mexcom::compile_error
			( right_ppv[i-1],
			  "no expression after ",
			  min::pgen_name ( op ) );
		    OK = false;
		    continue;
		}

	        iterations = iter;
		iteration & it =
		    iterations[number_of_iterations++];
		it = { op, right_vp[i], right_ppv[i] };
		i += 2;
	    }
	    else if (    op == ::EXACTLY
	              || op == ::AT_MOST )
	    {
	        if ( i + 1 >= s
		     ||
		     right_vp[i+1] != ::TIMES )
		{
		    mexcom::compile_error
			( right_ppv[i-1],
			  "no `times' after ",
			  min::pgen_name ( op ) );
		    goto ERROR_SKIP;
		}
		goto TIMES_FOUND;
	    }
	    else
	    {
		mexcom::compile_error
		    ( right_ppv[i-1],
		      "cannot understand ",
		      min::pgen_name ( op ) );
		goto ERROR_SKIP;
	    }

	    continue;

	ERROR_SKIP:
	    OK = false;
	    while ( i < s
	            &&
	               min::labfind
		           ( right_vp[i],
			     ::iteration_ops )
		    == -1 ) ++ i;
	    continue;


        TIMES_FOUND:
	    {
	        iterations = iter;
		iteration & it =
		    iterations[number_of_iterations++];
		it = { ::TIMES,
		       right_vp[i], right_ppv[i],
		       mexstack::var_stack_length };
		if ( ! ::compile_expression
		          ( right_vp[i] ) )
		    OK = false;
		i += 2;
		continue;
	    }
	}

	iter[number_of_iterations].type =
	    min::MISSING();
    }

    // Process left side.
    //
    min::uns32 initial_var_stack_length =
    	::var_stack_length;
    min::uns32 initial_next_var_stack_length =
        ::var_stack_length;

    if ( left_side != min::NONE() )
    {
	min::gen separator =
	    min::get ( left_side, min::dot_separator );
	min::obj_vec_ptr left_vp = left_side;
	min::uns32 n = 1;
	if ( separator == PARLEX::comma )
	    n = min::size_of ( left_vp );
	min::locatable_var<PRIM::var> vars[n];
	if ( separator != PARLEX::comma )
	{
	    left_vp = min::NULL_STUB;
	    vars[0] = ::scan_var ( left_side );
	    if ( vars[0] == min::NULL_STUB )
		OK = false;
	}
	else for ( min::uns32 i = 0; i < n; ++ i )
	{
	    vars[i] = ::scan_var ( left_vp[i] );
	    if ( vars[i] == min::NULL_STUB )
		OK = false;
	}

	for ( min::uns32 i = 0; i < n; ++ i )
	{
	    // Process non-next vars.
	    //
	    PRIM::var var = vars[i];
	    if ( var == min::NULL_STUB ) continue;

	    if ( var->flags & PRIM::WRITABLE_VAR )
	    {
		mexcom::compile_warn
		    ( var->position,
		      "this output variable hides"
		      " previous output variable;"
		      " this variable; this variable"
		      " is ignored" );
		vars[i] = min::NULL_STUB;
		    // To prevent next var check.
	    }
	    else if ( var->location == ::NO_LOCATION )
	    {
		::pushi
		    ( ZERO, var->position, var->label );
		var->location =
		    mexstack::var_stack_length - 1;
		var->flags |= PRIM::WRITABLE_VAR;
		::push_var ( var );
	    }
	}

	initial_next_var_stack_length =
	    ::var_stack_length;

	for ( min::uns32 i = 0; i < n; ++ i )
	{
	    // Process next vars, so they are last in
	    // the list of processed vars.
	    //
	    PRIM::var var = vars[i];
	    if ( var == min::NULL_STUB
		 ||
		 var->flags & PRIM::WRITABLE_VAR )
		continue;

	    ++ mexstack::var_stack_length;
	    mexstack::push_push_instr
		( var->label, var->label,
		  var->location, var->position );
	    var->location =
	        mexstack::var_stack_length - 1;
	    var->flags |= PRIM::WRITABLE_VAR;
	    ::push_var ( var );
	}
    }

    ::search_block ( block );

    min::uns32 number_of_vars =
        ::var_stack_length - initial_var_stack_length;
    min::uns32 nnext =
          ::var_stack_length
	- initial_next_var_stack_length;

    OK = ::compile_block
        ( block, label, nnext, iterations );

    TAB::root r = TAB::top ( ::symbol_table );
    for ( min::uns32 i = 0; i < number_of_vars; ++ i )
    {
        PRIM::var var = (PRIM::var) r;
	MIN_REQUIRE ( var != min::NULL_STUB );
	var->flags &= ~ PRIM::WRITABLE_VAR;
	r = TAB::previous ( r );
    }

    return OK;
}

bool static compile_if_statement
	( min::gen condition,
          min::gen block,
	  bool is_restricted )
{
    if ( ::block.if_next_jmp != 0 )
    {
        // Its an ELSE-IF or ELSE.
	//
        if ( ::block.if_finish_jmp == 0 )
	    ::block.if_finish_jmp = ::jmp_counter ++;
	::jmp ( ::block.if_finish_jmp,
	        ::block.if_pp );
        ::label ( ::block.if_next_jmp );
    }

    if ( condition != min::NONE() )
    {
        // Its an IF or ELSE-IF.
	//
	min::phrase_position_vec ppv = min::get
	    ( condition, min::dot_position );
	::block.if_pp = ppv->position;
	::block.if_next_jmp = ::jmp_counter ++;
	min::uns32 true_jmp = ::jmp_counter ++;
	min::uns32 fallthru_jmp = ::compile_expression
	    ( condition,
	      true_jmp, ::block.if_next_jmp );
	if ( fallthru_jmp == 0 ) return false;
	if ( fallthru_jmp == ::block.if_next_jmp )
	    ::jmp
	        ( ::block.if_next_jmp, ::block.if_pp );
	::label ( true_jmp );
    }
    else
        ::block.if_next_jmp = 0;

    if ( is_restricted )
	return ::compile_restricted_statement ( block );
    else
        return ::compile_block ( block );
}

min::uns32 static compile_expression
	( min::gen expression,
	  min::uns32 true_jmp,
	  min::uns32 false_jmp,
	  min::gen name )
{
    min::obj_vec_ptr vp = expression;
    min::phrase_position_vec ppv =
        ::get_position ( vp );
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
		    ( instr, ppv->position, name );
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
	        ( func->flags & PRIM::VALUE_OPERATOR )
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
			  i >= s ? name : ::star );
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
	-- mexstack::var_stack_length;
	::jmp ( false_jmp, ppv->position,
		mex::JMPFALSE );
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
	    mexstack::var_stack_length -= 2;
	    ::jmp
	        ( false_jmp, ppv[i], op_code, immedB );
	    if ( immedB != 0 )
		++ mexstack::var_stack_length;
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
    case PRIM::JMP:
    {
	MIN_REQUIRE ( s == 2 );
        if ( ! ::compile_expression ( vp[0] ) )
	    return 0;
        -- mexstack::var_stack_length;
	::jmp ( true_jmp, ppv->position );
	return false_jmp;
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
