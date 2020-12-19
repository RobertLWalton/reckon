// Reckon Parser
//
// File:	reckon_parser.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Sat Dec 19 07:33:56 EST 2020
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
# include <ll_parser_bracketed.h>
# define REC reckon
# define RLEX reckon::lexeme
# define PAR ll::parser
# define BRA ll::parser::bracketed
# define TAB ll::parser::table


// Reckon Parser
// ------ ------

min::locatable_gen RLEX::reckon;

static void initialize ( void )
{
    RLEX::reckon =
        min::new_str_gen ( "reckon" );
}
static min::initializer initializer ( ::initialize );

void REC::init_parser ( PAR::parser parser )
{
    min::gen result =
        PAR::begin_block
	    ( parser, RLEX::reckon,
	      PAR::top_level_position );

    MIN_REQUIRE ( result == min::SUCCESS() );

    min::uns32 block_level =
        PAR::block_level ( parser );

    BRA::bracketed_pass bracketed_pass =
        (BRA::bracketed_pass) parser->pass_stack;

    min::locatable_gen atom_name
        ( min::new_str_gen ( "atom" ) );
    min::locatable_gen code_name
        ( min::new_str_gen ( "code" ) );
    min::locatable_gen math_name
        ( min::new_str_gen ( "math" ) );

    TAB::flags atom =
        1ull << TAB::find_name
            ( parser->selector_name_table, atom_name );
    TAB::flags code =
        1ull << TAB::find_name
            ( parser->selector_name_table, code_name );
    TAB::flags math =
        1ull << TAB::find_name
            ( parser->selector_name_table, math_name );

    min::locatable_gen opening_quote
        ( min::new_str_gen ( "`" ) );
    min::locatable_gen closing_quote
        ( min::new_str_gen ( "'" ) );
    min::locatable_gen opening_square
        ( min::new_str_gen ( "[" ) );
    min::locatable_gen closing_square
        ( min::new_str_gen ( "]" ) );

    BRA::push_brackets
        ( opening_quote,
	  closing_quote,
	  atom,
	  block_level, PAR::top_level_position,
	  TAB::new_flags ( 0, 0, 0 ),
	  min::NULL_STUB, min::NULL_STUB,
	  bracketed_pass->bracket_table );

    BRA::push_brackets
        ( opening_square,
	  closing_square,
	  atom,
	  block_level, PAR::top_level_position,
	  TAB::new_flags ( code, atom, 0 ),
	  min::NULL_STUB, min::NULL_STUB,
	  bracketed_pass->bracket_table );
}

# ifdef TBD
    min::locatable_gen oper
        ( min::new_str_gen ( "operator" ) );

    PAR::pass pass = parser->pass_stack;
    while ( pass != min::NULL_STUB
            &&
	    pass->name != oper )
        pass = pass->next;
    MIN_REQUIRE ( pass != min::NULL_STUB );
    OP::oper_pass oper_pass = (OP::oper_pass) pass;

    min::locatable_gen binary
        ( min::new_str_gen ( "binary" ) );

    OP::push_oper
        ( LLEX::equal_at,
          min::MISSING(),
          code + math,
          block_level, PAR::top_level_position,
          OP::INFIX,
          9010,
          PAR::find_reformatter
              ( binary,
                OP::reformatter_stack ),
          min::NULL_STUB,
          oper_pass->oper_table );
#endif
