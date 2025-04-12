// Reckon Compiler
//
// File:	reckon_compiler.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Sat Apr 12 03:49:02 AM EDT 2025
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
# define LEXSTD ll::lexeme::standard
# define PAR ll::parser
# define PARLEX ll::parser::lexeme
# define TAB ll::parser::table
# define PRIM ll::parser::primary

min::locatable_var<TAB::key_table>
    reckon::symbol_table;

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
static min::locatable_gen NONE;
static min::locatable_gen ZERO;
static min::locatable_gen ONE;
static min::locatable_gen IF;
static min::locatable_gen ELSE_IF;
static min::locatable_gen ELSE;
static min::locatable_gen EXIT;
static min::locatable_gen CONTINUE;
static min::locatable_gen VARIABLES;

static min::locatable_gen DO;
static min::locatable_gen REPEAT;
static min::locatable_gen WHILE;
static min::locatable_gen UNTIL;
static min::locatable_gen EXACTLY;
static min::locatable_gen AT_MOST;
static min::locatable_gen TIMES;
static min::locatable_gen iteration_ops;
static min::locatable_gen HIDDEN_COUNTER;

static min::locatable_gen A;
static min::locatable_gen AN;

static min::locatable_gen left_quote;

static min::uns32 jmp_counter = 1;

static min::locatable_var<PRIM::primary_pass>
	primary_pass;
static min::locatable_var<PAR::parser> parser;
static min::gen modifying_ops;

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
    ::TRUE  = min::new_str_gen ( "*TRUE*" );
    ::FALSE  = min::new_str_gen ( "*FALSE*" );
    ::NONE  = min::new_str_gen ( "*NONE*" );
    ::ZERO  = min::new_num_gen ( 0 );
    ::ONE  = min::new_num_gen ( 1 );
    ::IF  = min::new_str_gen ( "if" );
    ::ELSE_IF  = min::new_lab_gen ( "else", "if" );
    ::ELSE  = min::new_str_gen ( "else" );
    ::EXIT  = min::new_str_gen ( "exit" );
    ::CONTINUE  = min::new_str_gen ( "continue" );
    ::VARIABLES  = min::new_str_gen ( "*VARIABLES*" );

    ::DO       = min::new_str_gen ( "do" );
    ::REPEAT   = min::new_str_gen ( "repeat" );
    ::WHILE    = min::new_str_gen ( "while" );
    ::UNTIL    = min::new_str_gen ( "until" );
    ::EXACTLY  = min::new_str_gen ( "exactly" );
    ::AT_MOST  = min::new_lab_gen ( "at", "most" );
    ::TIMES    = min::new_str_gen ( "times" );

    ::A        = min::new_str_gen ( "a" );
    ::AN       = min::new_str_gen ( "an" );

    ::left_quote = min::new_str_gen ( "`" );

    min::gen labv[6] =
        { ::DO, ::REPEAT, ::WHILE, ::UNTIL,
	        ::EXACTLY, ::AT_MOST };
    ::iteration_ops = min::new_lab_gen ( labv, 6 );
    ::HIDDEN_COUNTER =
        min::new_str_gen ( "*HIDDEN*COUNTER*" );
}
static min::initializer initializer ( ::initialize );


// Helper Functions
// ------ ---------

// Dump parameters.
//
void dump ( const char * header = NULL )
{
    min::printer printer = mex::default_printer;
    printer << min::bom << "DUMP";
    if ( header != NULL )
        printer << " " << header ;
    printer << ": " << min::place_indent ( 0 )
            << "RUN_STACK_LENGTH = "
	    << mexstack::stack_length
	    << " RUN_STACK_LIMIT = "
	    << mexstack::stack_limit
	    << min::indent
	    << "LEXICAL LEVEL = "
	    << mexstack::lexical_level
	    << " DEPTH = "
	    << mexstack::depth[mexstack::lexical_level]
	    << min::eom;
}


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
    PRIM::push_var ( reckon::symbol_table, var );
}

void mexstack::pop_stacks ( void )
{
    min::uns32 level = mexstack::lexical_level;
    min::uns32 depth = mexstack::depth[level];
    min::uns32 level_and_depth =
    	( level << 16 ) + depth;

    while ( true )
    {
        TAB::root r =
	    TAB::top ( reckon::symbol_table );
	if ( r == min::NULL_STUB ) break;
	if ( r->block_level <= level_and_depth )
	    break;
        TAB::pop ( reckon::symbol_table );
    }
}

// Call mexstack::push_instr with a PUSHI instruction
// that pushes the indicated value and has name as
// its trace_info.  Increments run_stack_length.
//
inline void pushi ( min::gen value, 
                    const min::phrase_position pp,
	            min::gen name = ::star,
		    bool no_source = false )
{
    mex::instr instr =
	{ mex::PUSHI, 0, 0, 0, 0, 0, 0, value };
    ++ mexstack::stack_length;
    mexstack::push_instr ( instr, pp, name, no_source );
}

// Call mexstack::push_instr with a POPS instruction
// that just pops the top value.
//
inline void pop ( const min::phrase_position pp )
{
    mex::instr instr = { mex::POPS };
    -- mexstack::stack_length;
    mexstack::push_instr ( instr, pp );
}

// Call mexstack::push_instr with a DEL instruction
// to make mexstack::stack_length = stack_length.
//
inline void del ( min::uns32 stack_length,
                  const min::phrase_position pp )
{
    mex::instr instr = { mex::DEL };
    instr.immedC = mexstack::stack_length
                 - stack_length;
    mexstack::stack_length -= instr.immedC;
    mexstack::push_instr ( instr, pp );
}

// Call mexstack::push_jmp_instr.  Adjust mexstack::
// run_stack_length BEFORE calling this function to
// be correct if the jump is taken.  Re-adjust AFTER
// the call to this function to be correct if the
// jump is NOT taken.
//
inline void jmp ( min::uns32 target,
                  const min::phrase_position pp,
		  min::uns8 op_code = mex::JMP,
		  min::uns32 immedB = 0,
		  min::gen immedD = ::ZERO,
		  min::int32 success_stack_offset = 0 )
{
    mex::instr i
        { op_code, 0, 0, 0, 0, immedB, 0, immedD };
    min::locatable_gen t
        ( min::new_num_gen ( target ) );
    mexstack::push_jmp_instr
        ( i, t, pp, false, success_stack_offset );
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
	// END... .  All actual blocks has this
	// non-zero.  The bottom of the stack represents
	// top level and has this zero.

    bool is_loop;
        // True for loop block, false for non-loop
	// block.
};

typedef min::packed_vec_insptr<block_struct>
    block_stack_vec;
static min::locatable_var<::block_stack_vec>
    block_stack;
static block_struct block;
    // Top of block stack.  ::block.name is protected
    // from gc because it is equal to ::block_name.
static min::locatable_gen block_name;

inline void push_block
	( min::gen name, bool is_loop = false )
{
    min::push ( ::block_stack ) = ::block;
    min::unprotected::acc_write_update
        ( ::block_stack, ::block.name );

    ::block =
        { name, 0, 0, min::MISSING_POSITION, 0 };
	// is_loop cannot be set by this statement;
	// might be a C++ compiler bug.
    ::block.is_loop = is_loop;
    ::block.block_finish_jmp = ::jmp_counter ++;
        // C++ does not correctly compile
	// ::jmp_counter ++ as part of { ... }.
    ::block_name = ::block.name;
}

inline void pop_block ( void )
{
    ::block = min::pop ( ::block_stack );
    ::block_name = ::block.name;
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

struct set_data {
    min::uns32 nlabels;	// Number of labels at end of
    			// refexp.
    min::gen refexp;	// Reference expression.
    min::phrase_position_vec refppv;
    			// ppv of refexp.
    min::uns32 base;	// Location of base object
    			// in mexstack variable stack.
    min::uns32 label;	// Location of label in mexstack
    			// variable stack, or
			// NO_LOCATION if label is
			// immedD or operation is VPUSH.
    min::gen immedD;    // Label if it is in immedD,
    			// NONE if operation is VPUSH.
    min::uns32 value;	// Location of value in variable
    			// stack.
};

// Scan a primary on the left side of '='.
// and locate or create a var object for it.  Returns
// the var object found or created, or returns NULL_STUB
// if compile_error called.  If no error, also returns
// information in set_data.
//
// The expression should be a MIN object whose elements
// are the variable name optionally preceeded by `next',
// and if not preceeded by `next', optionally followed
// by [] bracketed label expressions.
//
// If there is no error, a PRIM::var is returned.  This
// should be stored immediately in a min::locatable_var.
// If there is an error, NULL_STUB is returned.
//
// If the variable name beginning the reference
// expression is followed by [] bracketed label
// expressions, then if data == NULL compile_error is
// called and NULL_STUB is returned.  Otherwise data->
// nlabels is set to the number of these label
// expressions, data->refexp is set to the expression,
// and data->refppv is set to its .position.
//
// If there are no following [] bracketed label
// expressions and data != NULL, data->nlabels is
// set to 0.
//
// If nlabels > 0 and there is no error, the var
// returned is for the readable variable whose value is
// the base of the first label index subexpression.
//
// Otherwise, if the var returned has the WRITABLE_VAR
// flag, the var was found and the caller of this
// function should compile an instruction to move the
// appropriate runtime value to the var's location.
//
// But if data == NULL or data->nlabels == 0 and the var
// returned has no WRITABLE_VAR flag, the var has been
// created, and the caller of this function should
// assign the var's location and push the var into
// symbol_table at the appropriate time.  In this case,
// the returned var->location is NO_LOCATION unless
// the variable is a next variable, in which case it
// is the location of the predecessor of the variable.
// In either case var->location must be overwritten by
// the caller.
//
const min::uns32 NO_LOCATION = 0xFFFFFFFF;
PRIM::var scan_var
	( min::gen expression,
	  ::set_data * data = NULL )
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
        if ( data == NULL )
	{
	    mexcom::compile_error
		( ppv->position,
		  "left side of `=' operator does not"
		  " have the form"
		  " `next? VARIABLE-NAME'" );
	    return min::NULL_STUB;
	}
	data->nlabels = s - i;
	while ( i < s )
	{
	    if (    min::get
	                ( vp[i], min::dot_initiator )
		 != PARLEX::left_square )
	        break;
	    ++ i;
	}
	if ( i < s )
	{
	    mexcom::compile_error
		( ppv->position,
		  "left side of `=' operator does not"
		  " have the form `next? VARIABLE-NAME'"
		  " or VARIABLE-NAME[LABEL]..." );
	    return min::NULL_STUB;
	}
	if ( next_present )
	{
	    mexcom::compile_error
		( ppv->position,
		  "primary with labels"
		  " cannot begin with `next'" );
	    return min::NULL_STUB;
	}
	data->refexp = expression;
	data->refppv = ppv;
    }
    else if ( data != NULL )
        data->nlabels = 0;
        

    min::locatable_var<PRIM::var> var
        ( (PRIM::var) TAB::find
	      ( var_name,
	        PRIM::VAR,
	        PAR::ALL_SELECTORS,
	        reckon::symbol_table ) );

    if ( data != NULL && data->nlabels > 0 )
    {
        if ( var == min::NULL_STUB )
	{
	    mexcom::compile_error
		( ppv->position,
		  "variable name in a primary with"
		  " labels is undefined" );
	    return min::NULL_STUB;
	}
	if ( var->flags & PRIM::WRITABLE_VAR )
	{
	    if ( var->flags & PRIM::NEXT_VAR )
	    {
		var = (PRIM::var) TAB::find_next
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
		return min::NULL_STUB;
	    }
	}

	return var;
    }

    // At most one of the following _OK's can be true;
    //
    if ( var == min::NULL_STUB
	 ||
	 ( var->block_level >> 16 ) != level )
    {
	if ( next_present )
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
    }
    else if ( var->flags & PRIM::WRITABLE_VAR )
    {
        if (    var->flags & PRIM::NEXT_VAR
             && ! next_present )
	{
	    mexcom::compile_error
		( ppv->position,
		  "you must put `next' before"
		  " variable `",
		  min::pgen_name ( var_name ),
		  "'" );
	    return min::NULL_STUB;
	}
        else if (    ! ( var->flags & PRIM::NEXT_VAR )
                  && next_present )
	{
	    mexcom::compile_error
		( ppv->position,
		  "you CANNOT put `next' before"
		  " variable `",
		  min::pgen_name ( var_name ),
		  "'" );
	    return min::NULL_STUB;
	}
	else
	    return var;
    }
    else if ( ! next_present )
    {
	mexcom::compile_error
	    ( ppv->position,
              "NON-next variable `",
	      min::pgen_name ( var_name ),
	      "' has a predecessor of the same"
	      " variable name and lexical level" );
        return min::NULL_STUB;
    }
    else if ( ( var->block_level & 0xFFFF ) != depth )
    {
	mexcom::compile_error
	    ( ppv->position,
              "next variable `next ",
	      min::pgen_name ( var_name ),
	      "' has a predecessor of the same"
	      " variable name and lexical level"
	      " but less depth" );
        return min::NULL_STUB;
    }

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
	        reckon::symbol_table ) );

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
	      mexstack::stack_length,
              min::new_stub_gen
	          ( mexcom:: output_module ) );

    ++ mexstack::stack_length;
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
    else if ( s == 3
    	      &&
	      min::is_name ( vp[1] )
              &&
	         min::get ( ::modifying_ops, vp[1] )
	      != min::NONE() )
    {
	min::obj_vec_ptr vp0 = vp[0];
	min::uns32 s0 = min::size_of ( vp0 );
	min::phrase_position_vec ppv0 =
	    ::get_position ( vp0 );

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
        // Maybe we should just
	// ::search_block ( vp[s-1] ) ?
	//
        if ( s == 3 && vp[0] == ::IF )
	    ::search_block ( vp[2] );
        else if ( s == 3 && vp[0] == ::ELSE_IF )
	    ::search_block ( vp[2] );
        else if ( s == 3 && vp[1] == ::equal_sign )
	    ::search_block ( vp[2] );
        else if ( s == 2 && vp[0] == ::ELSE )
	    ::search_block ( vp[1] );
	else if ( s == 2 )
	    ::search_block ( vp[1] );
	    // For loops and `do:'.
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
// 	vp[0] "=" vp[2]
//
bool static compile_expression_assignment_statement
	( min::obj_vec_ptr & vp,
	  bool is_restricted = false );

// Compile statement of form
// 	vp[0] OP vp[2]
// where OP is the mex opcode of the instruction to
// perform the modification (e.g., mex::DIV).
//
bool static compile_modifying_statement
	( min::obj_vec_ptr & vp, min::gen op );

bool static compile_exit_statement
	( min::obj_vec_ptr & vp, min::uns32 s );

bool static compile_continue_statement
	( min::obj_vec_ptr & vp, min::uns32 s );

// Compile set data base and label in preparation
// for a SET instruction.  Data->base and data->label
// are set after the appropriate subexpressions
// are compiled.  All data->nlabel subexpressions
// are compiled.  It is up to the caller of this
// function to set the data->value field after reserving
// a stack place for the value, or instead to compile
// the value and execute the SET.
//
bool static compile_set_data
	( PRIM::var var, ::set_data * data );

// Compile a *VARIABLES* or *FUNCTIONS* statement.
//
bool static compile_symbols_statement
	( min::obj_vec_ptr & vp, min::uns32 s );

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

bool static compile_description
	( min::gen left_side,
          min::gen type_name,
          min::gen block );

bool static compile_object_block
	( min::gen type,
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
// In this case, on success 1 is returned and run_stack_
// length is incremented, but on failure compile_error
// is called and 0 is returned.  Here `name' is only
// used in the trace_info of the compiled instruction
// that pushes the expression value, and a top level
// CALLER is expected to push the corresponding var into
// the symbol_table.
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
// Any .initiator of the expression equal to initiator_
// to_be_ignored is treated as '('.
//
min::uns32 static compile_expression
	( min::gen expression,
	  min::uns32 true_jmp = 0,
	  min::uns32 false_jmp = 0,
	  min::gen name = ::star,
	  min::gen initiator_to_be_ignored =
	      min::MISSING() );

inline min::uns32 static compile_label
	( min::gen expression )
{
    return ::compile_expression
        ( expression, 0, 0, ::star,
	  PARLEX::left_square );
}


// If an expression can be evaluated at compile time,
// return its value.  Otherwise return min::FAILURE().
//
// An expression with a .type can be evaluated at
// compile time if the .type is "<Q>" or if
// publish_object says so.  Similarly an expression
// with an .initiator other than ( can be evaluated
// at compile time if publish_object says so.
//
min::gen static evaluate_expression
	( min::gen expression );

// Does the work of evaluate_expression for an object
// if that object does not contain any [...] subobjects
// anywhere within its elements or attributes.  For
// such expressions, returns the object.  Otherwise
// returns min::FAILURE().  An object is made public
// if it is returned, and any subobjects not containing
// any [...] subobjects are made public.
//
min::gen static publish_object
	( min::obj_vec_ptr vp,
	  min::uns32 max_attrs = 16 );

// Does the work of computing an object when that
// or one of its subobjects contains [...].  Must NOT
// be called when object's type is "<Q>".
//
bool static compile_object
	( min::obj_vec_ptr vp,
	  min::phrase_position_vec ppv,
	  min::gen name = ::star,
	  min::uns32 max_attrs = 16 );

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

// The following does the work of compile_expression in
// the case that the expression has an initiator other
// then NONE, "(", or LOGICAL_LINE.  Pushes a value into
// the stack and returns true, or outputs an error
// message and returns false.
//
bool static compile_bracketed_expression
	( min::obj_vec_ptr & vp,
	  min::phrase_position_vec ppv,
	  min::gen initiator,
	  min::gen name = ::star );

// The following does the work of compile_expression in
// the case that the expression has initiator NONE and
// a type NOT equal to NONE.  Pushes a value into the
// stack and returns true, or outputs an error message
// and returns false.
//
bool static compile_typed_expression
	( min::obj_vec_ptr & vp,
	  min::phrase_position_vec ppv,
	  min::gen initiator,
	  min::gen name = ::star );

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
    reckon::symbol_table =
        ::primary_pass->primary_table;
    ::modifying_ops = ::primary_pass->modifying_ops;

    min::locatable_var<PRIM::var> var;
    ::pushi ( mex::FALSE, PAR::top_level_position,
              ::FALSE, true );
    var = PRIM::create_var
        ( ::FALSE,
          PAR::ALL_SELECTORS,
          PAR::top_level_position,
          0, 0, 0,
          mexstack::stack_length - 1,
          min::new_stub_gen ( mexcom::output_module ) );
    ::push_var ( var );
    ::pushi ( mex::TRUE, PAR::top_level_position,
              ::TRUE, true );
    var = PRIM::create_var
        ( ::TRUE,
          PAR::ALL_SELECTORS,
          PAR::top_level_position,
          0, 0, 0,
          mexstack::stack_length - 1,
          min::new_stub_gen ( mexcom::output_module ) );
    ::push_var ( var );
    ::pushi ( min::NONE(), PAR::top_level_position,
              ::NONE, true );
    var = PRIM::create_var
        ( ::NONE,
          PAR::ALL_SELECTORS,
          PAR::top_level_position,
          0, 0, 0,
          mexstack::stack_length - 1,
          min::new_stub_gen ( mexcom::output_module ) );
    ::push_var ( var );

    ::block_stack = ::block_stack_type.new_stub ( 64 );
    ::block  = { min::MISSING(), 0, 0,
                 min::MISSING_POSITION, 0 };
    ::block_name = ::block.name;

    min::locatable_gen length_name
        ( min::new_str_gen ( "length" ) );

    PRIM::push_builtin_func
        ( length_name, reckon::symbol_table,
	  mex::VSIZE );

    min::locatable_gen copy_name
        ( min::new_str_gen ( "copy" ) );

    PRIM::push_builtin_func
        ( copy_name, reckon::symbol_table,
	  mex::COPY );
}


// Compile Statement
// ------- ---------

bool REC::compile_statement ( min::gen statement )
{
    min::obj_vec_ptr vp = statement;
    min::uns32 s = min::size_of ( vp );
    min::attr_ptr ap = vp;
    min::locate ( ap, min::dot_separator );
    if ( min::get ( ap ) == PARLEX::comma )
    {
	bool OK = true;
        for ( min::uns32 i = 0; i < s; ++ i )
	{
	    if ( ! ::compile_expression
	               ( vp[i] ) )
		OK = false;
	}
	return OK;
    }

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
	    min::gen right_side = min::NONE();
	    if ( s0 == 3 )
	    {
	        right_side = vp0[2];
		min::obj_vec_ptr right_vp = right_side;
		if (    min::size_of ( right_vp ) == 2
		     && (    right_vp[0] == ::A
		          || right_vp[0] == ::AN ) )
		{
		    return ::compile_description
			( vp0[0], right_vp[1], vp[1] );
		}
	    }
	    return ::compile_block_assignment_statement
	        ( vp0[0], right_side, vp[1] );
	}

	if (  vp0[0] == ::A || vp0[0] == ::AN )
	{
	    return ::compile_description
	        ( min::NONE(), vp0[1], vp[1] );
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

    if ( s == 3 )
    {
        if ( vp[1] == ::equal_sign )
	    return
	      ::compile_expression_assignment_statement
	          ( vp );
	if ( min::is_name ( vp[1] ) )
	{
	    min::gen op = min::get
		( ::modifying_ops, vp[1] );
	    if ( op != min::NONE() )
		return
		   ::compile_modifying_statement
		       ( vp, op );
	}
    }

    if ( vp[0] == ::EXIT )
        return ::compile_exit_statement( vp, s );
    if ( vp[0] == ::CONTINUE )
        return ::compile_continue_statement( vp, s );
    if ( vp[0] == ::VARIABLES )
        return ::compile_symbols_statement( vp, s );

    vp = min::NULL_STUB;
    return compile_expression ( statement );

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

    if ( s == 3 )
    {
	if ( vp[1] == ::equal_sign )
	    return
	      ::compile_expression_assignment_statement
	          ( vp, true );
	min::gen op =
	    min::get ( ::modifying_ops, vp[1] );
	if ( op != min::NONE() )
	    return
	       ::compile_modifying_statement
	           ( vp, op );
    }

    if ( vp[0] == ::EXIT )
        return ::compile_exit_statement( vp, s );
    if ( vp[0] == ::CONTINUE )
        return ::compile_continue_statement( vp, s );

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

    min::obj_vec_ptr left_vp = vp[0];
    min::attr_ptr left_ap = left_vp;
    min::locate ( left_ap, min::dot_initiator );
    min::gen left_initiator = min::get ( left_ap );
    min::locate ( left_ap, min::dot_separator );
    min::gen left_separator = min::get ( left_ap );
    if ( left_initiator != min::NONE() )
    {
        min::phrase_position_vec left_ppv =
	    ::get_position ( left_vp );
	mexcom::compile_error
	    ( left_ppv->position,
	      "cannot understand left side of =;"
	      "statement ignored" );
	return false;
    }

    min::uns32 left_n = 1;
    if ( left_separator == PARLEX::comma )
    {
	left_n = min::size_of ( left_vp );
	MIN_ASSERT
	    ( left_n > 1,
	      "expression with , separator has less"
	      " than 2 elements" );
    }

    min::obj_vec_ptr right_vp = vp[2];
    min::attr_ptr right_ap = right_vp;
    min::locate ( right_ap, min::dot_initiator );
    min::gen right_initiator = min::get ( right_ap );
    min::locate ( right_ap, min::dot_separator );
    min::gen right_separator = min::get ( right_ap );

    bool right_is_list =
        ( right_initiator == min::NONE()
	  &&
	  right_separator == PARLEX::comma );

    min::uns32 right_n = 1;
    if ( right_is_list )
    {
	right_n = min::size_of ( right_vp );
	MIN_ASSERT
	    ( right_n > 1,
	      "expression with , separator has less"
	      " than 2 elements" );
    }

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
    min::gen exps[right_n];
    ::set_data data[left_n];
    bool OK = true;
    if ( left_n == 1 )
    {
	left_vp = min::NULL_STUB;
	right_vp = min::NULL_STUB;
	vars[0] = ::scan_var ( vp[0], data + 0 );
	if ( vars[0] == min::NULL_STUB )
	    OK = false;
	exps[0] = vp[2];
    }
    else for ( min::uns32 i = 0; i < left_n; ++ i )
    {
	vars[i] = ::scan_var ( left_vp[i], data + i );
	if ( vars[i] == min::NULL_STUB )
	    OK = false;
	exps[i] = right_vp[i];
    }

    min::uns32 stack_length =
        mexstack::stack_length;

    for ( min::uns32 i = 0; i < left_n; ++ i )
    {
	if ( vars[i] == min::NULL_STUB ) continue;
	::set_data * d = data +  i;
        if ( d->nlabels == 0 ) continue;
	if ( ! ::compile_set_data ( vars[i], d ) )
	{
	    OK = false;
	    vars[i] = min::NULL_STUB;
	    continue;
	}
	::pushi ( ::ZERO, d->refppv->position );
	d->value = mexstack::stack_length - 1;
    }
    min::uns32 set_data_length =
        mexstack::stack_length - stack_length;
    stack_length = mexstack::stack_length;

    for ( min::uns32 i = 0; i < left_n; ++ i )
    {
        PRIM::var var = vars[i];
	min::gen exp = exps[i];
	::set_data & d = data[i];
	min::locatable_gen var_name
	    ( var == min::NULL_STUB ? ::star :
	      d.nlabels > 0 ? ::star :
	      ::full_var_name ( var ) );

	if ( ! :: compile_expression
		( exp, 0, 0, var_name ) )
	{
	    OK = false;
	    min::phrase_position_vec ppv = min::get
		( exp, min::dot_position );
	    pushi ( min::UNDEFINED(),
		    ppv->position,
		    var_name );
	}

	if ( var == min::NULL_STUB )
	{
	    min::phrase_position_vec ppv = min::get
		( exp, min::dot_position );
	    ::pop ( ppv->position );
	    continue;
	}

	if ( d.nlabels > 0 )
	{
	    mex::instr pop_instr = { mex::POPS };
	    pop_instr.immedA =
	        mexstack::stack_length
		- d.value - 1;
	    -- mexstack::stack_length;
	    mexstack::push_instr
		( pop_instr, d.refppv->position );
	}
	else if ( var->flags & PRIM::WRITABLE_VAR )
	{
	    min::gen labv[2] = { ::star, var_name };
	    min::locatable_gen trace_info
		( min::new_lab_gen ( labv, 2 ) );
	    mex::instr instr =
		{ mex::POPS, 0, 0, 0,
		    mexstack::stack_length - 1
		  - var->location };
	    -- mexstack::stack_length;
	    mexstack::push_instr
		( instr, var->position, trace_info );
	}
	else if ( is_restricted )
	{
	    mexcom::compile_error
		( var->position,
		  "cannot allocate variable (",
		  min::pgen_name ( var_name ),
		  ") in a RESTRICTED statement;"
		  " variable ignored" );
	    ::pop ( var->position );
	    vars[i] = min::NULL_STUB;
	    OK = false;
	}
	// else: var->location assigned below.
    }
    min::uns32 allocated_var_length =
        mexstack::stack_length - stack_length;

    if ( set_data_length > 0 )
    {
	mex::instr push_instr = { mex::PUSHS };
	mex::instr set_instr = { mex::SET };
	mex::instr vpush_instr = { mex::VPUSH };
	for ( min::uns32 i = 0; i < left_n; ++ i )
	{
	    if ( vars[i] == min::NULL_STUB ) continue;
	    ::set_data & d = data[i];
	    if ( d.nlabels == 0 ) continue;


	    min::gen labv[2] =
	        { ::star, vars[i]->label };
	    min::locatable_gen trace_info
	        ( min::new_lab_gen ( labv, 2 ) );
	    if ( d.label != ::NO_LOCATION )
	    {
		push_instr.immedA =
		      mexstack::stack_length
		    - d.label - 1;
		++ mexstack::stack_length;
		mexstack::push_instr
		    ( push_instr, d.refppv->position );
		push_instr.immedA =
		      mexstack::stack_length
		    - d.value - 1;
		++ mexstack::stack_length;
		mexstack::push_instr
		    ( push_instr, d.refppv->position );
		set_instr.immedA =
		      mexstack::stack_length
		    - d.base - 1;
		mexstack::stack_length -= 2;
		mexstack::push_instr
		    ( set_instr, d.refppv->position,
				 trace_info );
	    }
	    else
	    {
		push_instr.immedA =
		      mexstack::stack_length
		    - d.value - 1;
		++ mexstack::stack_length;
		mexstack::push_instr
		( push_instr, d.refppv->position );
		vpush_instr.immedA =
		      mexstack::stack_length
		    - d.base - 1;
		-- mexstack::stack_length;
		mexstack::push_instr
		    ( vpush_instr, d.refppv->position,
				   trace_info );
	    }
	}

	min::phrase_position_vec ppv =
	    ::get_position ( vp );
	mex::instr del_instr = { mex::DEL };
	del_instr.immedA = allocated_var_length;
	del_instr.immedC = set_data_length;
	mexstack::stack_length -= set_data_length;
	mexstack::push_instr ( del_instr, ppv[2] );
    }

    min::uns32 location = mexstack::stack_length
                        - allocated_var_length;
    for ( min::uns32 i = 0; i < left_n; ++ i )
    {
        PRIM::var var = vars[i];
	if ( var != min::NULL_STUB
	     &&
	     data[i].nlabels == 0
	     &&
	     ! is_restricted
	     &&
	     ! ( var->flags & PRIM::WRITABLE_VAR ) )
	{
	    var->location = location ++;
	    ::push_var ( var );
	}
    }

    return OK;
}

bool static compile_modifying_statement
	( min::obj_vec_ptr & vp, min::gen op )
{
    ::set_data data[1];
    PRIM::var var = ::scan_var ( vp[0], data );
    min::locatable_gen var_name;
    min::uns32 stack_length =
        mexstack::stack_length;
    if ( var == min::NULL_STUB )
        /* do nothing */;
    else if ( data->nlabels == 0 )
    {
	var_name = ::full_var_name ( var );
	++ mexstack::stack_length;
	mexstack::push_push_instr
	    ( ::star, var_name, var->location,
	      var->position );
    }
    else // if data->nlabels > 0
    {
        if ( ! ::compile_set_data ( var, data ) )
	{
	    ::del ( stack_length,
	            data->refppv->position );
	    return false;
	}
	min::gen labv[2] =
	    { data->nlabels == 1 ? var->label : ::star,
	      ::star };
	min::locatable_gen trace_info
	    ( min::new_lab_gen ( labv, 2 ) );
	if ( data->label != ::NO_LOCATION )
	{
	    mex::instr push_instr =
		{ mex::PUSHS, 0, 0, 0,
		    mexstack::stack_length - 1
		  - data->label };
	    ++ mexstack::stack_length;
	    mexstack::push_instr
		( push_instr, data->refppv->position );
	    mex::instr get_instr =
		{ mex::GET, 0, 0, 0,
		    mexstack::stack_length - 1
		  - data->base };
	    mexstack::push_instr
		( get_instr, data->refppv->position,
		             trace_info );
	}
	else
	{
	    mex::instr instr =
		{ mex::VPOP, 0, 0, 0,
		    mexstack::stack_length - 1
		  - data->base };
	    ++ mexstack::stack_length;
	    mexstack::push_instr
		( instr, data->refppv->position,
			 trace_info );
	}

    }
    if ( ! ::compile_expression ( vp[2] ) )
    {
        if ( var != min::NULL_STUB )
	    ::pop ( var->position );
	return false;
    }
    else if ( var == min::NULL_STUB )
    {
        min::phrase_position_vec ppv =
            min::get ( vp[2], min::dot_position );
	::pop ( ppv->position );
	return false;
    }
    else if ( data->nlabels > 0 )
        /* do nothing */;
    else if ( (   var->flags
                & PRIM::WRITABLE_VAR ) == 0 )
    {
	mexcom::compile_error
	    ( var->position,
	      "cannot modify NON-write-only"
	      " variable" );
        min::phrase_position_vec ppv =
            min::get ( vp[2], min::dot_position );
        ::pop ( ppv->position );
	::pop ( var->position );
	return false;
    }
    mex::instr instr =
        { (min::uns8) min::direct_float_of ( op ) };
    -- mexstack::stack_length;
    mexstack::push_instr
        ( instr, ::get_position(vp)->position );

    if ( data->nlabels == 0 )
    {
	min::gen labv[2] = { ::star, var_name };
	min::locatable_gen trace_info
	    ( min::new_lab_gen ( labv, 2 ) );
	instr =
	    { mex::POPS, 0, 0, 0,
		mexstack::stack_length - 1
	      - var->location };
	-- mexstack::stack_length;
	mexstack::push_instr
	    ( instr, var->position, trace_info );
    }
    else // data->nlabels > 0
    {
	min::gen labv[2] =
	    { ::star, data->nlabels == 1 ?
	                  var->label : ::star };
	min::locatable_gen trace_info
	    ( min::new_lab_gen ( labv, 2 ) );
	if ( data->label != ::NO_LOCATION )
	{
	    MIN_REQUIRE
		(    data->label
		  == mexstack::stack_length - 2 );
	    mex::instr set_instr =
		{ mex::SET, 0, 0, 0,
		    mexstack::stack_length - 1
		  - data->base };
	    mexstack::stack_length -= 2;
	    mexstack::push_instr
		( set_instr, data->refppv->position,
			     trace_info );
	}
	else
	{
	    mex::instr instr =
		{ mex::VPUSH, 0, 0, 0,
		    mexstack::stack_length - 1
		  - data->base,
                  0, 0, min::MISSING() };
	    mexstack::stack_length -= 1;
	    mexstack::push_instr
		( instr, data->refppv->position,
			 trace_info );
	}

	switch (   mexstack::stack_length
		 - stack_length )
	{
	case 0:
	    /* do nothing */;
	    break;
	case 1:
	    ::pop ( data->refppv->position );
	    break;
	default:
	    ::del ( stack_length,
	            data->refppv->position );
	    break;
	}
    }
    return true;
}

bool static compile_exit_statement
	( min::obj_vec_ptr & vp, min::uns32 s )
{
    min::phrase_position_vec ppv =
	::get_position ( vp );
    const char * message = NULL;
    min::phrase_position pp = ppv->position;;

    if ( s == 1 )
    {
    	if ( ::block.block_finish_jmp != 0 )
	{
	    ::jmp ( ::block.block_finish_jmp,
	            ppv->position );
	    return true;
	}
	else
	    message = "exit not inside block";
    }

    else if ( s == 2 )
    {
	min::obj_vec_ptr vp1 = vp[1];
	pp = ppv[1];
	min::locatable_gen name;
	min::uns32 jmp = 0;
	if ( vp1 == min::NULL_STUB )
	    message = "not a block name";
	else
	{
	    min::uns32 i1 = 0;
	    name = PRIM::scan_var_name ( vp1, i1 );
	    if ( name == min::NONE()
	         ||
		 i1 != min::size_of ( vp1 ))
	        message = "not a block name";
	    else
	    {
		min::uns32 j = ::block_stack->length;
		if ( j == 0 )
		{
		    message = "exit statement is"
			      " not in a block";
		    pp = ppv->position;
		}
		else if ( name == ::block.name )
		    jmp = ::block.block_finish_jmp;
		else while ( j != 0 )
		{
		    min::ptr<::block_struct> p =
			::block_stack + -- j;
		    if ( p->name != name )
			continue;
		    jmp = p->block_finish_jmp;
		    break;
		}
		if ( jmp == 0 )
		{
		    message = "exit statement is"
			      " not in a block"
			      " with the given name";
		    pp = ppv->position;
		}
	    }
	}
	if ( message == NULL )
	{
	    ::jmp ( jmp, ppv->position );
	    return true;
	}
    }
    else
    {
        message = "extra stuff after";
	pp = ppv[1];
    }

    mexcom::compile_error
	( pp, message, min::pnop,
	  "; statement ignored" );
    return false;
}

bool static compile_continue_statement
	( min::obj_vec_ptr & vp, min::uns32 s )
{
    min::phrase_position_vec ppv =
	::get_position ( vp );
    const char * message = NULL;
    min::phrase_position pp = ppv->position;;

    min::uns32 loop_depth;
    mex::instr instr = { mex::CONT };

    if ( s == 1 )
    {
        if ( mexstack::cont
	         ( instr, 1, 0,
	           min::MISSING(), ppv->position ) )
	    return true;
	message = "continue statement is not in a loop";
    }

    else if ( s == 2 )
    {
	min::obj_vec_ptr vp1 = vp[1];
	pp = ppv[1];
	min::locatable_gen name;
	if ( vp1 == min::NULL_STUB )
	    message = "not a block name";
	else
	{
	    min::uns32 i1 = 0;
	    name = PRIM::scan_var_name ( vp1, i1 );
	    if ( name == min::NONE()
	         ||
		 i1 != min::size_of ( vp1 ))
	        message = "not a block name";
	    else if ( name == ::block.name )
	    {
		if ( ! ::block.is_loop )
		    message =
		        "named block is not loop block";
		else
		    loop_depth = 1;
	    }
	    else
	    {
	        loop_depth = ::block.is_loop;
		min::uns32 j = ::block_stack->length;
		message =
		    "block with given name"
		    " does not exist";
		while ( j != 0 )
		{
		    min::ptr<::block_struct> p =
			::block_stack + -- j;
		    if ( p->is_loop ) ++ loop_depth;
		    if ( p->name != name )
			continue;
		    if ( p->is_loop )
		        message = NULL;
		    else
		        message =
			    "named block is not"
			    " a loop block";
		    break;
		}
	    }
	}

	if ( message == NULL )
	{
	    MIN_ASSERT
	        ( mexstack::cont
	              ( instr, loop_depth, 0,
		        min::MISSING(), ppv->position ),
		  "mexstack::cont failed;"
		  " mismatched block stacks" );
	    return true;
	}
    }
    else
    {
        message = "extra stuff after";
	pp = ppv[1];
    }

    mexcom::compile_error
	( pp, message, min::pnop,
	  "; statement ignored" );
    return false;
}

bool static compile_set_data
	( PRIM::var var, ::set_data * data )
{
    min::obj_vec_ptr vp = data->refexp;
    min::uns32 s = min::size_of ( vp );
    min::phrase_position_vec ppv = data->refppv;
    min::phrase_position pp = ppv->position;
    min::uns32 i = s - data->nlabels;
    if (    var->location
         >= mexstack::fp[mexstack::lexical_level] )
	data->base = var->location;
    else
    {
	pp.end = (& ppv[i-1])->end;
	data->base = mexstack::stack_length ++;
        mexstack::push_push_instr
	    ( var->label, var->label, var->location,
	      pp );
    }
    bool OK = true;
    min::gen from = var->label;
    mex::instr get_instr = { mex::GET };
    mex::instr vpop_instr = { mex::VPOP };
    while ( i < s )
    {
	min::obj_vec_ptr lvp = vp[i];
        min::uns32 labsize = min::size_of ( lvp );
	if ( labsize == 0 )
	{
	    if ( ++ i == s )
	    {
	        data->label = ::NO_LOCATION;
		data->immedD = min::NONE();
		return OK;
	    }
	}
	else // if labsize > 0
	{
	    lvp = min::NULL_STUB;
	    if ( ! ::compile_label ( vp[i] ) )
	    {
		OK = false;
		::pushi ( ::ZERO, ppv[i] );
	    }
	    if ( ++ i == s ) break;
	}

	min::gen labv[2] = { from, ::star };
	min::locatable_gen trace_info
	    ( min::new_lab_gen ( labv, 2 ) );
	pp.end = (& ppv[i-1])->end;
	
	if ( labsize == 0 )
	{
	    vpop_instr.immedA =
		mexstack::stack_length - data->base - 1;
	    mexstack::push_instr
		( vpop_instr, pp, trace_info );
	}
	else // if labsize > 0
	{
	    get_instr.immedA =
		mexstack::stack_length - data->base - 1;
	    mexstack::push_instr
		( get_instr, pp, trace_info );
	}

	data->base = mexstack::stack_length - 1;
	from = ::star;
    }
    data->label = mexstack::stack_length - 1;
    return OK;
}

bool static compile_symbols_statement
	( min::obj_vec_ptr & vp, min::uns32 s )
{
    min::printer printer = mex::default_printer;
    printer << min::bom
            << "*VARIABLES*: "
	    << min::place_indent ( 0 );

    for ( TAB::root symbol =
	      TAB::top ( reckon::symbol_table );
          symbol != min::NULL_STUB;
	  symbol = TAB::previous ( symbol ) )
    {
	min::uns32 level_and_depth =
	    symbol->block_level;
	min::locatable_gen name;
	    ( symbol->label );

	PRIM::var var = (PRIM::var) symbol;
	if ( var == min::NULL_STUB )
	    continue;

	name = ::full_var_name ( var );

	printer << min::indent
	        << ( level_and_depth >> 16 )
		<< "."
		<< ( level_and_depth & 0xFFFF )
		<< " "
		<< name;
	if ( var != min::NULL_STUB
	     &&
	     var->flags & PRIM::WRITABLE_VAR )
	    printer << " [WO]";
    }

    printer << min::eom;

    return true;
}

bool static compile_block
        ( min::gen block,
	  min::gen label,
	  min::uns32 nnext,
	  iteration * iterations )
{
    min::phrase_position_vec ppv =
        min::get ( block, min::dot_position );
    mex::instr beg =
        { iterations == NULL ?
	  mex::BEG : mex::BEGL };
    mexstack::begx
        ( beg, nnext, 0,
	  min::MISSING(), ppv->position );
    push_block ( label, iterations != NULL );

    if ( iterations != NULL )
    {
        PRIM::var vars[nnext];

	TAB::root r = TAB::top ( reckon::symbol_table );
	for ( min::uns32 i = nnext; 0 < i; -- i )
	{
	    vars[i-1] = (PRIM::var) r;
	    MIN_REQUIRE ( vars[i-1] != min::NULL_STUB );
	    r = TAB::previous ( r );
	}
	min::uns32 level = mexstack::lexical_level;
	for ( min::uns32 i = 0; i < nnext; ++ i )
	{
	    PRIM::var var = PRIM::create_var
		( vars[i]->label,
		  PAR::ALL_SELECTORS,
		  vars[i]->position,
		  level,
		  mexstack::depth[level],
		  PRIM::NEXT_VAR + PRIM::WRITABLE_VAR,
		  mexstack::stack_length ++,
		  min::new_stub_gen
		      ( mexcom:: output_module ) );
	    vars[i]->flags &= ~ PRIM::WRITABLE_VAR;
	    ::push_var ( var );
	}
    }

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
		      mexstack::stack_length
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
    min::locatable_gen type ( min::NONE() );
    if ( right_side != min::NONE() )
    {
        right_vp = right_side;
	min::uns32 right_s = min::size_of ( right_vp );
	number_of_iterations = right_s;
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
		++ i;
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
		       mexstack::stack_length };
		if ( ! ::compile_expression
		          ( right_vp[i], 0, 0,
			    ::HIDDEN_COUNTER ) )
		    OK = false;
		i += 2;
		continue;
	    }
	}
    }

    iter[number_of_iterations].type =
	min::MISSING();

    // Process left side.
    //
    min::uns32 initial_run_stack_length =
    	mexstack::stack_length;
    min::uns32 initial_next_run_stack_length =
	mexstack::stack_length;
	// In case left_size == NONE.

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
		    ( ::ZERO, var->position,
		              var->label );
		var->location =
		    mexstack::stack_length - 1;
		var->flags |= PRIM::WRITABLE_VAR;
		::push_var ( var );
	    }
	}

	initial_next_run_stack_length =
	    mexstack::stack_length;

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

	    ++ mexstack::stack_length;
	    mexstack::push_push_instr
		( var->label, var->label,
		  var->location, var->position );
	    var->location =
	        mexstack::stack_length - 1;
	    var->flags |= PRIM::WRITABLE_VAR;
	    ::push_var ( var );
	}
    }

    ::search_block ( block );

    min::uns32 number_of_vars =
          mexstack::stack_length
	- initial_run_stack_length;
    min::uns32 nnext =
          mexstack::stack_length
	- initial_next_run_stack_length;

    OK = ::compile_block
        ( block, label, nnext, iterations );

    TAB::root r = TAB::top ( reckon::symbol_table );
    for ( min::uns32 i = 0; i < number_of_vars; ++ i )
    {
        PRIM::var var = (PRIM::var) r;
	MIN_REQUIRE ( var != min::NULL_STUB );
	var->flags &= ~ PRIM::WRITABLE_VAR;
	r = TAB::previous ( r );
    }

    return OK;
}

bool static compile_description
	( min::gen left_side,
          min::gen type_name,
          min::gen block )
{
    bool OK = true;

    min::obj_vec_ptr tvp = type_name;
    min::uns32 ts = min::size_of ( tvp );
    min::phrase_position_vec tppv =
	::get_position ( tvp );
    min::uns32 j = 0;
    min::locatable_gen type
	( PRIM::scan_var_name ( tvp, j ) );
    if ( type == min::NONE() || j < ts )
    {
	mexcom::compile_error
	    ( tppv->position,
	      "`a' or `an'"
	      " not followed by description"
	      " type name; type name ignored" );
	OK = false;
    }
    ::set_data data;
    min::locatable_var<PRIM::var> var
        ( min::NULL_STUB );
    min::locatable_gen var_name ( ::star );
    min::locatable_gen trace_info ( min::MISSING() );
    min::uns32 stack_length = mexstack::stack_length;
    if ( left_side != min::NONE() )
    {
	var = ::scan_var ( left_side, & data );
	if ( var == min::NULL_STUB ) OK = false;
	else
	{
	    var_name = ::full_var_name ( var );
	    min::gen labv[2] = { ::star, var_name };
	    trace_info = min::new_lab_gen ( labv, 2 );
	    if ( data.nlabels > 0 )
	    {
		if ( ! ::compile_set_data
		           ( var, & data ) )
		{
		    OK = false;
		    var = min::NULL_STUB;
		}
		else
		    data.value = mexstack::stack_length;
	    }
	}
    }

    if ( ! ::compile_object_block ( type, block ) )
    {
	OK = false;
	min::phrase_position_vec ppv =
	    min::get ( block, min::dot_position );
	::pushi ( ::ZERO, ppv->position );
    }

    if ( var == min::NULL_STUB )
	/* Do Nothing */;
    else if ( data.nlabels > 0 )
    {
	if ( data.label != ::NO_LOCATION )
	{
	    MIN_REQUIRE
		(    data.label
		  == mexstack::stack_length - 2 );
	    mex::instr set_instr =
		{ mex::SET, 0, 0, 0,
		    mexstack::stack_length - 1
		  - data.base };
	    mexstack::stack_length -= 2;
	    mexstack::push_instr
		( set_instr, data.refppv->position,
			     trace_info );
	}
	else
	{
	    mex::instr instr =
		{ mex::VPUSH, 0, 0, 0,
		    mexstack::stack_length - 1
		  - data.base,
                  0, 0, min::MISSING() };
	    mexstack::stack_length -= 1;
	    mexstack::push_instr
		( instr, data.refppv->position,
			 trace_info );
	}

	switch (   mexstack::stack_length
		 - stack_length )
	{
	case 0:
	    /* do nothing */;
	    break;
	case 1:
	    ::pop ( data.refppv->position );
	    break;
	default:
	    ::del ( stack_length,
	            data.refppv->position );
	    break;
	}
    }
    else if ( var->flags & PRIM::WRITABLE_VAR )
    {
	mex::instr instr =
	    { mex::POPS, 0, 0, 0,
		mexstack::stack_length - 1
	      - var->location };
	-- mexstack::stack_length;
	mexstack::push_instr
	    ( instr, var->position, trace_info );
    }
    else
    {
	var->location = mexstack::stack_length - 1;
	::push_var ( var );
    }

    return OK;
}

bool static compile_object_block
	( min::gen type,
          min::gen block )
{
    bool OK = true;
    min::obj_vec_ptr bvp = block;
    min::uns32 bs = min::size_of ( bvp );
    min::phrase_position_vec ppv =
        ::get_position ( bvp );
    min::locatable_gen obj
        ( min::new_obj_gen ( 3 * bs, bs ) );
    if ( type != min::NONE() )
        min::set ( obj, min::dot_type, type );
    mex::instr seti_instr = { mex::SETI, 0, 0, 0, 1 };
    min::locatable_gen labels[bs];
    min::gen exps[bs];
    min::uns32 n = 0;

    for ( min::uns32 i = 0; i < bs; ++ i )
    {
	min::obj_vec_ptr lvp = bvp[i];
	min::uns32 ls = min::size_of ( lvp );
	if ( ls != 3 || lvp[1] != ::equal_sign )
	{
	    mexcom::compile_error
		( ppv[i],
		  "cannot understand;"
		  " line ignored" );
	    OK = false;
	    continue;
	}
	min::obj_vec_ptr labvp = lvp[0];
	min::uns32 li = 0;
	min::locatable_gen label
	    ( PRIM::scan_var_name ( labvp, li ) );
	if ( label == min::NONE()
	     ||
	     li < min::size_of ( labvp ) )
	{
	    min::phrase_position_vec labppv =
		::get_position ( labvp );
	    mexcom::compile_error
		( labppv->position,
		  "not a label name;"
		  " line ignored" );
	    OK = false;
	    continue;
	}

	min::locatable_gen value
	    ( ::evaluate_expression ( lvp[2] ) );
	if  ( value == min::FAILURE() )
	{
	    labels[n] = label;
	    exps[n] = lvp[2];
	    ++ n;
	    continue;
	}

	min::set ( obj, label, value );
    }

    min::obj_vec_insptr ivp = obj;
    min::set_public_flag_of ( ivp );
    mex::instr copyi_instr = { mex::COPYI };
    copyi_instr.immedD = obj;
    ++ mexstack::stack_length;
    mexstack::push_instr
	( copyi_instr, ppv->position );

    for ( min::uns32 i = 0; i < n; ++ i )
    {

	if ( ! ::compile_label ( exps[i] ) )
	{
	    ::pushi ( ::ZERO, ppv[i] );
	    OK = false;
	}
	seti_instr.immedD = labels[i];
	-- mexstack::stack_length;
	mexstack::push_instr
	    ( seti_instr, ppv[i] );
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


inline min::uns32 return_value
    ( min::phrase_position pp,
      min::uns32 true_jmp,
      min::uns32 false_jmp )
{
    if ( true_jmp != 0 )
    {
	-- mexstack::stack_length;
	::jmp ( false_jmp, pp, mex::JMPFALSE );
	return true_jmp;
    }
    else
	return 1;
}

min::uns32 static compile_expression
	( min::gen expression,
	  min::uns32 true_jmp,
	  min::uns32 false_jmp,
	  min::gen name,
	  min::gen initiator_to_be_ignored )
{
    min::obj_vec_ptr vp = expression;
    min::phrase_position_vec ppv =
        ::get_position ( vp );
    min::uns32 s = min::size_of ( vp );
    min::uns8 level = mexstack::lexical_level;

    min::attr_ptr ap = vp;
    min::locate ( ap, min::dot_initiator );
    min::gen initiator = min::get ( ap );
    if ( initiator != initiator_to_be_ignored
         &&
	 initiator != min::NONE()
         &&
	 initiator != PARLEX::left_parenthesis
         &&
	 initiator != min::LOGICAL_LINE() )
    {
        bool OK = ::compile_bracketed_expression
	    ( vp, ppv, initiator, name );
	if ( ! OK ) return 0;
	else return return_value
	    ( ppv->position, true_jmp, false_jmp );
    }

    min::locate ( ap, min::dot_type );
    min::gen type = min::get ( ap );
    if ( type != min::NONE() )
    {
        MIN_REQUIRE ( type != min::doublequote );
	    // Quoted string has no .position.
        bool OK = ::compile_typed_expression
	    ( vp, ppv, type, name );
	if ( ! OK ) return 0;
	else return return_value
	    ( ppv->position, true_jmp, false_jmp );
    }
        

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
	      reckon::symbol_table ) )
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

	    if ( argument_vector->length == 0 )
	    {
		++ mexstack::stack_length;
		mexstack::push_push_instr
		    ( name, var->label, var->location,
		      ppv->position );
	    }
	    else
	    {
		min::uns32 stack_length =
		    mexstack::stack_length;
		min::uns32 offset =
		      min::size_of ( vp )
		    - argument_vector->length;
		min::phrase_position pp = ppv->position;
		min::uns32 location = var->location;

		if (    var->location
		     < mexstack::fp [mexstack::
		                     lexical_level] )
		{
		    pp.end = (& ppv[offset-1])->end;
		    location =
		        mexstack::stack_length ++;
		    mexstack::push_push_instr
			( var->label, var->label,
			  var->location, pp );
		}

	        bool OK = true;
		min::gen from = var->label;
		min::gen to =
		    ( argument_vector->length == 1 ?
		      name : ::star );
		min::gen labv[2] = { from, to };
		min::locatable_gen trace_info
		    ( min::new_lab_gen ( labv, 2 ) );
		pp.end = (& ppv[offset+0])->end;
		mex::instr get_instr = { mex::GET };
		mex::instr vpop_instr = { mex::VPOP };
		{
		    min::obj_vec_ptr avp =
		        (min::gen )
			( argument_vector[0] );
		    if ( min::size_of ( avp ) == 0 )
		    {
			vpop_instr.immedA =
			    mexstack::stack_length
			  - location - 1;
			++ mexstack::stack_length;
			mexstack::push_instr
			    ( vpop_instr, pp,
			      trace_info );
		    }
		    else
		    {
		        avp = min::NULL_STUB;
			if ( ! ::compile_label
				( argument_vector[0] ) )
			{
			    ::pushi ( min::MISSING(),
				      ppv[offset] );
			    OK = false;
			}
			get_instr.immedA =
			    mexstack::stack_length
			  - location - 1;
			mexstack::push_instr
			    ( get_instr, pp,
			      trace_info );
		    }
		}

		for ( min::uns32 i = 1;
		      i < argument_vector->length;
		      ++ i )
		{
		    if ( i ==   argument_vector->length
		              - 1 )
		        to = name;
		    min::gen labv[2] = { ::star, to };
		    min::locatable_gen trace_info
			( min::new_lab_gen
			      ( labv, 2 ) );
		    pp.end = (& ppv[offset+i])->end;
		    min::obj_vec_ptr avp =
		        (min::gen )
			( argument_vector[i] );
		    if ( min::size_of ( avp ) == 0 )
		    {
			vpop_instr.immedA = 0;
			++ mexstack::stack_length;
			mexstack::push_instr
			    ( vpop_instr, pp,
			      trace_info );
		    }
		    else
		    {
		        avp = min::NULL_STUB;
			if ( ! ::compile_label
			         ( argument_vector[i] )
			   )
			{
			    ::pushi ( min::MISSING(),
				      ppv[offset+i] );
			    OK = false;
			}
			get_instr.immedA = 1;
			mexstack::push_instr
			    ( get_instr,
			      pp, trace_info );
		    }
		}
		min::uns32 C = mexstack::stack_length
		             - stack_length - 1;
		if ( C > 0 )
		{
		    mex::instr del_instr =
			{ mex::DEL, 0, 0, 0, 1, 0, C };
		    min::gen labv[2] = { name, name };
		    min::locatable_gen trace_info
			( min::new_lab_gen
			      ( labv, 2 ) );
		    mexstack::stack_length -= C;
		    mexstack::push_instr
			( del_instr, pp, trace_info );
		}
		if ( ! OK ) return 0;
	    }

	    return ::return_value
		( ppv->position, true_jmp, false_jmp );
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
		    { op_code, mex::T_AOP, 0, 0,
                      0, 0, 0, ::ZERO };
		mexstack::stack_length -= jend;;
		++ mexstack::stack_length;
		mexstack::push_instr
		    ( instr, ppv->position, name );
		return ::return_value
		    ( ppv->position,
		      true_jmp, false_jmp );
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
		min::uns8 op_code_1 =
		    (min::uns8) ( func->flags >> 24 );
		min::uns8 op_code_2 =
		    (min::uns8) ( func->flags >> 16 );

	        i = 0;
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
		    -- mexstack::stack_length;
		    mexstack::push_instr
			( instr, ppv->position,
			  i >= s ? name : ::star );
		}
		if ( ! OK ) return 0;
		return ::return_value
		    ( ppv->position,
		      true_jmp, false_jmp );
	    }
	}
    }

    if ( s == 1 )
    {
	min::obj_vec_ptr vp0 = vp[0];
	if ( vp0 == min::NULL_STUB )
	{
	    if ( min::is_num ( vp[0] ) )
		::pushi ( vp[0], ppv[0], name );
	    else goto CANNOT_UNDERSTAND;
	}
	else
	{
	    min::attr_ptr ap0 = vp0;
	    min::locate ( ap0, min::dot_type );
	    if ( min::get ( ap0 ) == min::doublequote )
		::pushi ( vp0[0], ppv[0], name );
	    else
	    {
	        vp0 = min::NULL_STUB;
		if ( ! ::compile_expression
		           ( vp[0], true_jmp, false_jmp,
			            name ) )
		    goto CANNOT_UNDERSTAND;
	    }
	}

	return ::return_value
	    ( ppv->position, true_jmp, false_jmp );

CANNOT_UNDERSTAND:

	mexcom::compile_error
	    ( ppv[0],
	      "cannot understand expression" );
	return 0;
    }

    mexcom::compile_error
	( ppv->position,
          "cannot understand expression" );
    return 0;
}


min::gen static publish_object
	( min::obj_vec_ptr vp, min::uns32 max_attrs )
{
    const min::stub * stub = (const min::stub *) vp;

    struct min::attr_info infos[max_attrs];

    min::uns32 n = min::attr_info_of
        ( infos, max_attrs, vp, false, true );
    if ( n > max_attrs )
	return ::publish_object ( vp, n );

    for ( min::uns32 i = 0; i < n; ++ i )
    {
        // Do not recurse into [...] value.
	//
	if ( infos[i].name == min::dot_initiator )
	{
	    if ( infos[i].value == PARLEX::left_square )
		return min::FAILURE();
	    break;
	}
    }
    for ( min::uns32 i = 0; i < n; ++ i )
    {
	MIN_REQUIRE ( infos[i].value_count == 1 );
	    // Because this is parser output.
        min::gen v = infos[i].value;
	vp = v;
	if ( vp == min::NULL_STUB ) continue;
	if (    ::publish_object ( vp )
	     == min::FAILURE() )
	    return min::FAILURE();
    }

    vp = min::NULL_STUB;

    min::obj_vec_insptr ivp = stub;
    if ( ivp != min::NULL_STUB )
        // Expression is not already public.
	min::set_public_flag_of ( ivp );

    return min::new_stub_gen ( stub );
}

bool static compile_object
	( min::obj_vec_ptr vp,
	  min::phrase_position_vec ppv,
	  min::gen name,
	  min::uns32 max_attrs )
{
    bool OK = true;

    struct min::attr_info infos[max_attrs];

    min::uns32 n = min::attr_info_of
        ( infos, max_attrs, vp, false, false );
    if ( n > max_attrs )
	return ::compile_object ( vp, ppv, name, n );
    min::uns32 s = min::size_of ( vp );

    for ( min::uns32 i = 0; i < n; ++ i )
    {
	if ( infos[i].name == min::dot_initiator )
	{
	    if ( infos[i].value == PARLEX::left_square )
	    {
		min::gen expression =
		    min::new_stub_gen ( vp );
	        vp = min::NULL_STUB;
		return    compile_label ( expression )
		       != 0;
	    }
	    else
	        break;
	}
    }

    min::locatable_gen obj
        ( min::new_obj_gen ( s + 3 * n, n ) );

    min::gen labels[n];
    min::gen exps[n];
    min::uns32 j = 0;
    for ( min::uns32 i = 0; i < n; ++ i )
    {
	if ( infos[i].name == min::dot_position )
	    continue;

        min::gen value = infos[i].value;
	if ( min::is_obj ( value ) )
	    value = ::evaluate_expression ( value );
	if ( value == min::FAILURE() )
	{
	    labels[j] = infos[i].name;
	    exps[j] = infos[i].value;
	    ++ j;
	}
	else
	    min::set ( obj, infos[i].name, value );
    }

    mex::instr copyi_instr = { mex::COPYI };
    mex::instr vpush_instr = { mex::VPUSH, 0, 0, 0, 1 };
    min::gen elements[s];
    min::uns32 k = 0;

    for ( min::uns32 i = 0; i < s; ++ i )
    {
        min::gen value = vp[i];
	if ( min::is_obj ( value ) )
	    value = ::publish_object ( value );
	if ( value == min::FAILURE() )
	{
	    if ( obj != min::NONE() )
	    {
		min::obj_vec_insptr ivp = obj;
		for ( min::uns32 i = 0; i < k; ++ i )
		    min::attr_push(ivp) = elements[i];
		min::set_public_flag_of ( ivp );
		k = 0;
	    }

	    if ( k > 1 )
	    {
		obj = min::new_obj_gen ( k, 0 );
		min::obj_vec_insptr ivp = obj;
		for ( min::uns32 i = 0; i < k; ++ i )
		    min::attr_push(ivp) = elements[i];
		min::set_public_flag_of ( ivp );
		vpush_instr.immedD = min::NONE();
	    }
	    else if ( k == 1 )
	    {
	        ::pushi ( elements[0], ppv->position );
		vpush_instr.immedD = min::MISSING();
	    }

	    if ( obj != min::NONE() )
	    {
		++ mexstack::stack_length;
		copyi_instr.immedD = obj;
		mexstack::push_instr
		    ( copyi_instr, ppv->position );
		obj = min::NONE();
	    }

	    if ( k > 0 )
	    {
		-- mexstack::stack_length;
		mexstack::push_instr
		    ( vpush_instr, ppv->position );
		k = 0;
	    }

	    min::obj_vec_ptr vpi = vp[i];

	    min::phrase_position_vec ppvi =
	        ::get_position ( vpi );

	    if ( ! compile_object ( vpi, ppvi ) )
	    {
		::pushi ( ::ZERO, ppvi->position );
		OK = false;
	    }

	    vpush_instr.immedD = ::left_quote;
	    -- mexstack::stack_length;
	    mexstack::push_instr
		( vpush_instr, ppv->position );
	}
	else
	    elements[k++] = value;
    }

    if ( obj != min::NONE() )
    {
        // All elements can be evaluated at compile
	// time.

        MIN_REQUIRE ( j > 0 );
	     // Must have attribute that CANNOT be
	     // evaluated at compile time.

	min::obj_vec_insptr ivp = obj;
	for ( min::uns32 i = 0; i < k; ++ i )
	    min::attr_push(ivp) = elements[i];
	min::set_public_flag_of ( ivp );
	++ mexstack::stack_length;
	copyi_instr.immedD = obj;
	mexstack::push_instr
	    ( copyi_instr, ppv->position );
    }

    mex::instr seti_instr = { mex::SETI, 0, 0, 0, 1 };

    for ( min::uns32 i = 0; i < j; ++ i )
    {
	if ( ! ::compile_label ( exps[i] ) )
	{
	    ::pushi ( ::ZERO, ppv[i] );
	    OK = false;
	}
	seti_instr.immedD = labels[i];
	-- mexstack::stack_length;
	mexstack::push_instr ( seti_instr, ppv[i] );
    }

    return OK;
}


min::gen static evaluate_expression
	( min::gen expression )
{
    min::obj_vec_ptr vp = expression;
    if ( vp == min::NULL_STUB )
    {
        if ( min::is_num ( expression ) )
	    return expression;
	else
	    return min::FAILURE();
    }
    min::uns32 s = min::size_of ( vp );
    min::attr_ptr ap = vp;
    min::locate ( ap, min::dot_type );
    min::gen type = min::get ( ap );
    if ( s == 1 && type == min::doublequote )
        return vp[0];
    min::locate ( ap, min::dot_initiator );
    min::gen initiator = min::get ( ap );
    if ( initiator == PARLEX::left_parenthesis
         ||
	 ( initiator == min::NONE()
	   &&
	   type == min::NONE() ) )
    {
        if ( s == 1 )
	    return evaluate_expression ( vp[0] );
	else
	    return min::FAILURE();
    }
    else
        return publish_object ( vp );
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
	min::int32 success_stack_offset = -1;
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
	    if ( i + 2 >= s )
	    {
	        immedB = success_stack_offset = 0;
		mexstack::stack_length -= 2;
	    }
	    else
		mexstack::stack_length -= 1;
	    ::jmp
	        ( false_jmp, ppv[i], op_code, immedB,
		  ::ZERO, success_stack_offset );
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
	min::uns8 jmp_op =
	    ( 0xFF & ( func->flags >> 16 ) );
        if ( ! ::compile_expression ( vp[0] ) )
	    return 0;
        -- mexstack::stack_length;
	::jmp ( true_jmp, ppv->position, jmp_op );
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

bool static compile_bracketed_expression
	( min::obj_vec_ptr & vp,
	  min::phrase_position_vec ppv,
	  min::gen initiator,
	  min::gen name )
{
    if ( initiator == PARLEX::left_square )
    {
        min::attr_ptr ap = vp;
	min::locate ( ap, min::dot_separator );
	min::gen separator = min::get ( ap );
	min::uns32 n = min::size_of ( vp );
	if ( n > 0 && separator != PARLEX::comma )
	    n = 1;

	// Create empty [] list with room for n vector
	// elements and .initiator and .terminator.
	//
	mex::instr i1 =
	    { mex::PUSHOBJ, 0, 0, 0, n+10, 0, 2 };
	++ mexstack::stack_length;
	mexstack::push_instr
	    ( i1, ppv->position, name );

	min::gen labbuf[2] = { ::star, name };
	min::locatable_gen trace_info
	    ( min::new_lab_gen ( labbuf, 2 ) );

	::pushi ( PARLEX::left_square,
	          ppv->position, ::star, true );
	mex::instr i2 =
	    { mex::SETI, 0, 0, 0, 1, 0, 0,
	      min::dot_initiator };
	-- mexstack::stack_length;
	mexstack::push_instr
	    ( i2, ppv->position, trace_info, true );
	::pushi ( PARLEX::right_square,
	          ppv->position, ::star, true );
	i2.immedD = min::dot_terminator;
	-- mexstack::stack_length;
	mexstack::push_instr
	    ( i2, ppv->position, trace_info, true );
	::pushi ( PARLEX::comma,
	          ppv->position, ::star, true );
	i2.immedD = min::dot_separator;
	-- mexstack::stack_length;
	mexstack::push_instr
	    ( i2, ppv->position, trace_info, true );

	if ( n == 0 ) return true;

	mex::instr i3 =
	    { mex::VPUSH, 0, 0, 0, 1, 0, 0,
	      PARLEX::left_square };

	if ( separator != PARLEX::comma )
	{
	    const min::stub * stub =
	        (const min::stub *) vp;
	    min::gen exp = min::new_stub_gen ( stub );
	    vp =  min::NULL_STUB;
	    if ( ! ::compile_label ( exp ) ) 
	        return false;
	    -- mexstack::stack_length;
	    mexstack::push_instr
		( i3, ppv->position, trace_info, true );

	    return true;
	}
	else
	{
	    min::uns32 s = min::size_of ( vp );
	    bool OK = true;
	    for ( min::uns32 i = 0; i < s; ++ i )
	    {
	        if ( ! ::compile_expression ( vp[i] ) )
		    OK = false;
		else
		{
		    -- mexstack::stack_length;
		    mexstack::push_instr
			( i3, ppv->position, trace_info,
			      true );
		}
	    }
	    return OK;
	}

    }

    min::gen value = ::publish_object ( vp );
    if ( value != min::FAILURE() )
    {
	min::locatable_gen immedD
	    ( min::new_stub_gen
		  ( (const min::stub *) vp ) );
	vp = min::NULL_STUB;
	::pushi ( immedD, ppv->position, name );
	return true;
    }
    else return ::compile_object ( vp, ppv, name );
}

bool static compile_typed_expression
	( min::obj_vec_ptr & vp,
	  min::phrase_position_vec ppv,
	  min::gen type,
	  min::gen name )
{
    min::gen value = ::publish_object ( vp );
    if ( value != min::FAILURE() )
    {
	min::locatable_gen immedD
	    ( min::new_stub_gen
		  ( (const min::stub *) vp ) );
	vp = min::NULL_STUB;
	::pushi ( immedD, ppv->position, name );
	return true;
    }
    else return ::compile_object ( vp, ppv, name );
}
