// Reckon Parser
//
// File:	reckon_parser.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Tue Feb  2 08:02:23 EST 2021
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
# include <ll_parser_oper.h>
# define REC reckon
# define RLEX reckon::lexeme
# define PAR ll::parser
# define BRA ll::parser::bracketed
# define TAB ll::parser::table
# define OP ll::parser::oper


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

    OP::oper_pass oper_pass = min::NULL_STUB;
    PAR::pass pass = parser->pass_stack;
    while (    oper_pass == min::NULL_STUB
            && pass != min::NULL_STUB )
    {
        oper_pass = (OP::oper_pass) pass;
	pass = pass->next;
    }
    MIN_REQUIRE ( oper_pass != min::NULL_STUB );

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
    min::locatable_gen equal_number_sign
        ( min::new_str_gen ( "=#" ) );
    min::locatable_gen if_name
        ( min::new_str_gen ( "if" ) );
    min::locatable_gen else_name
        ( min::new_str_gen ( "else" ) );
    min::locatable_gen while_name
        ( min::new_str_gen ( "while" ) );
    min::locatable_gen colon
        ( min::new_str_gen ( ":" ) );

    min::locatable_gen right_associative
        ( min::new_lab_gen ( "right", "associative" ) );

    PAR::reformatter right_associative_reformatter =
        PAR::find_reformatter
	    ( right_associative, OP::reformatter_stack );

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

    OP::push_oper
        ( equal_number_sign,
	  min::MISSING(),
	  code + math,
	  block_level, PAR::top_level_position,
	  OP::INFIX,
	  1000, right_associative_reformatter,
	  min::NULL_STUB,
	  oper_pass->oper_table );

    OP::push_oper
        ( if_name,
	  min::MISSING(),
	  code,
	  block_level, PAR::top_level_position,
	  OP::NOFIX,
	  0, min::NULL_STUB,
	  min::NULL_STUB,
	  oper_pass->oper_table );

    OP::push_oper
        ( else_name,
	  min::MISSING(),
	  code,
	  block_level, PAR::top_level_position,
	  OP::NOFIX,
	  0, min::NULL_STUB,
	  min::NULL_STUB,
	  oper_pass->oper_table );

    OP::push_oper
        ( while_name,
	  min::MISSING(),
	  code,
	  block_level, PAR::top_level_position,
	  OP::NOFIX,
	  0, min::NULL_STUB,
	  min::NULL_STUB,
	  oper_pass->oper_table );

    OP::push_oper
        ( colon,
	  min::MISSING(),
	  code,
	  block_level, PAR::top_level_position,
	  OP::NOFIX + OP::AFIX,
	  0,
	  min::NULL_STUB, min::NULL_STUB,
	  oper_pass->oper_table );

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
