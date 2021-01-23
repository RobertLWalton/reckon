// Reckon Parser
//
// File:	reckon_parser.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Sat Jan 23 05:49:36 EST 2021
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

    min::locatable_gen code_name
        ( min::new_str_gen ( "code" ) );
    min::locatable_gen math_name
        ( min::new_str_gen ( "math" ) );
    min::locatable_gen text_name
        ( min::new_str_gen ( "text" ) );
    min::locatable_gen data_name
        ( min::new_str_gen ( "data" ) );

    TAB::flags code =
        1ull << TAB::find_name
            ( parser->selector_name_table, code_name );
    TAB::flags math =
        1ull << TAB::find_name
            ( parser->selector_name_table, math_name );
    TAB::flags text =
        1ull << TAB::find_name
            ( parser->selector_name_table, text_name );
    TAB::flags data =
        1ull << TAB::find_name
            ( parser->selector_name_table, data_name );

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
	  code + math + text,
	  block_level, PAR::top_level_position,
	  TAB::new_flags ( text, code + math + data, 0 ),
	  min::NULL_STUB, min::NULL_STUB,
	  bracketed_pass->bracket_table );

    BRA::push_brackets
        ( opening_square,
	  closing_square,
	  text + code,
	  block_level, PAR::top_level_position,
	  TAB::new_flags ( math, text + code, 0 ),
	  min::NULL_STUB, min::NULL_STUB,
	  bracketed_pass->bracket_table );

}

# ifdef TBD


    BRA::push_brackets
        ( opening_quote,
	  closing_quote,
	  atom,
	  block_level, PAR::top_level_position,
	  TAB::new_flags ( 0, 0, 0 ),
	  min::NULL_STUB, min::NULL_STUB,
	  bracketed_pass->bracket_table );

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
