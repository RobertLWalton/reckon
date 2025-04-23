// Reckon Parser
//
// File:	reckon_parser.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Wed Apr 23 02:01:05 AM EDT 2025
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
# include <ll_parser_primary.h>
# define REC reckon
# define RLEX reckon::lexeme
# define PAR ll::parser
# define PARLEX ll::parser::lexeme
# define BRA ll::parser::bracketed
# define TAB ll::parser::table
# define OP ll::parser::oper
# define PRIM ll::parser::primary


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

    PRIM::primary_pass primary_pass = min::NULL_STUB;
    pass = parser->pass_stack;
    while (    primary_pass == min::NULL_STUB
            && pass != min::NULL_STUB )
    {
        primary_pass = (PRIM::primary_pass) pass;
	pass = pass->next;
    }
    MIN_REQUIRE ( primary_pass != min::NULL_STUB );

    min::locatable_gen label_name
        ( min::new_str_gen ( "label" ) );
    min::locatable_gen code_name
        ( min::new_str_gen ( "code" ) );
    min::locatable_gen math_name
        ( min::new_str_gen ( "math" ) );
    min::locatable_gen text_name
        ( min::new_str_gen ( "text" ) );

    TAB::flags label =
        1ull << TAB::find_name
            ( parser->selector_name_table, label_name );
    TAB::flags code =
        1ull << TAB::find_name
            ( parser->selector_name_table, code_name );
    TAB::flags math =
        1ull << TAB::find_name
            ( parser->selector_name_table, math_name );
    TAB::flags text =
        1ull << TAB::find_name
            ( parser->selector_name_table, text_name );

    BRA::push_brackets
        ( PARLEX::left_square,
	  PARLEX::right_square,
	  label + text + math,
	  block_level, PAR::top_level_position,
	  TAB::new_flags
	      ( code, label + text + math, 0 ),
	  min::NULL_STUB, min::MISSING(),
	  bracketed_pass->bracket_table );

    min::locatable_gen stars_colon
        ( min::new_lab_gen ( "***", ":" ) );

    min::locatable_var<min::phrase_position_vec_insptr>
	pos;
    min::init ( pos, parser->input_file,
		PAR::top_level_position, 0 );

    min::uns32 paragraph_check = PAR::MISSING_MASTER;
    min::uns32 line_check = PAR::MISSING_MASTER;
    {
	min::locatable_gen table_check_name
	    ( min::new_str_gen ( "TABLE-CHECK" ) );
	paragraph_check =
	    PAR::get_lexical_master
		( table_check_name, parser );
    }

    min::locatable_gen implied_p_header
	( min::new_obj_gen ( 10, 1 ) );
    min::locatable_gen p
	( min::new_str_gen ( "p" ) );
    {
	min::obj_vec_insptr vp ( implied_p_header );
	min::attr_insptr ap ( vp );
	min::locate ( ap, min::dot_type );
	min::set ( ap, p );
	min::locate ( ap, min::dot_position );
	min::set ( ap, min::new_stub_gen ( pos ) );
	min::set_flag
	    ( ap, min::standard_attr_hide_flag );
    }

    BRA::push_indentation_mark
	( stars_colon,
	  min::MISSING(),
	  code,
	  block_level, PAR::top_level_position,
	  PAR::parsing_selectors ( text ),
	  implied_p_header,
	  paragraph_check,
	  line_check,
	  bracketed_pass->bracket_table );

    BRA::push_bracket_type
	( code_name,
	    PAR::TOP_LEVEL_SELECTOR
	  + code + text,
	  block_level, PAR::top_level_position,
	  TAB::new_flags ( 0, 0, 0 ),
	  PARLEX::RESET,
	  min::MISSING(),
	  min::MISSING(),
	  PAR::MISSING_MASTER,
	  min::NULL_STUB,
	  min::MISSING(),
	  bracketed_pass->bracket_type_table );

    min::locatable_gen equal_number_sign
        ( min::new_str_gen ( "=#" ) );
    min::locatable_gen is_a_name
        ( min::new_lab_gen ( "is", "a" ) );
    min::locatable_gen arrow
        ( min::new_str_gen ( "<--" ) );

    min::locatable_gen exit_name
        ( min::new_str_gen ( "exit" ) );
    min::locatable_gen on_name
        ( min::new_str_gen ( "on" ) );
    min::locatable_gen for_every
        ( min::new_lab_gen ( "for", "every" ) );

    min::locatable_gen every_name
        ( min::new_str_gen ( "every" ) );
    min::locatable_gen in_name
        ( min::new_str_gen ( "in" ) );
    min::locatable_gen with_name
        ( min::new_str_gen ( "with" ) );
    min::locatable_gen and_name
        ( min::new_str_gen ( "and" ) );
    min::locatable_gen the_name
        ( min::new_str_gen ( "the" ) );
    min::locatable_gen of_the_name
        ( min::new_lab_gen ( "of", "the" ) );

    min::locatable_gen declare
        ( min::new_str_gen ( "declare" ) );

    PAR::reformatter declare_reformatter =
        PAR::find_reformatter
	    ( declare, OP::reformatter_stack );

    min::locatable_gen declare_arguments
	    ( min::new_obj_gen ( 1 ) );
    min::obj_vec_insptr davp ( declare_arguments );
    min::attr_push ( davp ) = PARLEX::colon;

    min::locatable_gen assignment
        ( min::new_str_gen ( "assignment" ) );

    PAR::reformatter assignment_reformatter =
        PAR::find_reformatter
	    ( assignment,
	      OP::reformatter_stack );

    min::locatable_gen unary_prefix
        ( min::new_lab_gen ( "unary", "prefix" ) );

    PAR::reformatter unary_prefix_reformatter =
        PAR::find_reformatter
	    ( unary_prefix, OP::reformatter_stack );

    OP::push_oper
        ( equal_number_sign,
	  min::MISSING(),
	  code + math,
	  block_level, PAR::top_level_position,
	  OP::LEFT + OP::LINE,
	  1000, assignment_reformatter,
	  min::MISSING(),
	  oper_pass->oper_table );
    OP::push_oper
        ( is_a_name,
	  min::MISSING(),
	  code,
	  block_level, PAR::top_level_position,
	  OP::INFIX,
	  12000, min::NULL_STUB,
	  min::MISSING(),
	  oper_pass->oper_table );
    OP::push_oper
        ( with_name,
	  min::MISSING(),
	  code,
	  block_level, PAR::top_level_position,
	  OP::INFIX +OP::AFIX,
	  12000, min::NULL_STUB,
	  min::MISSING(),
	  oper_pass->oper_table );
    OP::push_oper
        ( and_name,
	  min::MISSING(),
	  code,
	  block_level, PAR::top_level_position,
	  OP::INFIX +OP::AFIX,
	  12000, min::NULL_STUB,
	  min::MISSING(),
	  oper_pass->oper_table );

    OP::push_oper
        ( for_every,
	  min::MISSING(),
	  code,
	  block_level, PAR::top_level_position,
	  OP::PREFIX + OP::LINE,
	  3000, unary_prefix_reformatter,
	  min::MISSING(),
	  oper_pass->oper_table );

    OP::push_oper
        ( exit_name,
	  min::MISSING(),
	  code,
	  block_level, PAR::top_level_position,
	  OP::INITIAL + OP::LINE,
	  0, min::NULL_STUB,
	  min::MISSING(),
	  oper_pass->oper_table );

    OP::push_oper
        ( on_name,
	  min::MISSING(),
	  code,
	  block_level, PAR::top_level_position,
	  OP::PREFIX + OP::LINE,
	  0, min::NULL_STUB,
	  min::MISSING(),
	  oper_pass->oper_table );

    OP::push_oper
        ( arrow,
	  min::MISSING(),
	  code,
	  block_level, PAR::top_level_position,
	  OP::NOFIX + OP::LINE,
	  0, declare_reformatter,
	  declare_arguments,
	  oper_pass->oper_table );

    min::gen in_buf[1] = { every_name };
    min::locatable_gen in_following
        ( min::new_lab_gen ( in_buf, 1 ) );
    min::gen with_buf[3] =
        { the_name, of_the_name, every_name };
    min::locatable_gen with_following
        ( min::new_lab_gen ( with_buf, 3 ) );

    PRIM::push_separator
        ( every_name,
	  code,
	  block_level, PAR::top_level_position,
	  min::MISSING(),
	  primary_pass->separator_table );
    PRIM::push_separator
        ( in_name,
	  code,
	  block_level, PAR::top_level_position,
	  in_following,
	  primary_pass->separator_table );
    PRIM::push_separator
        ( with_name,
	  code,
	  block_level, PAR::top_level_position,
	  with_following,
	  primary_pass->separator_table );
}

# ifdef TBD

    min::locatable_gen left_quote
        ( min::new_str_gen ( "`" ) );
    min::locatable_gen right_quote
        ( min::new_str_gen ( "'" ) );

    BRA::push_brackets
        ( left_quote,
	  right_quote,
	  code + math + text,
	  block_level, PAR::top_level_position,
	  TAB::new_flags
	      ( text, code + math + data, 0 ),
	  min::NULL_STUB, min::MISSING(),
	  bracketed_pass->bracket_table );

    min::locatable_gen period
        ( min::new_str_gen ( "." ) );
    min::locatable_gen question
        ( min::new_str_gen ( "?" ) );
    min::locatable_gen exclamation
        ( min::new_str_gen ( "!" ) );
    min::locatable_gen s
        ( min::new_str_gen ( "s" ) );
    min::locatable_gen left_double_quote
        ( min::new_str_gen ( "``" ) );
    min::locatable_gen right_double_quote
        ( min::new_str_gen ( "''" ) );


    min::locatable_var
    	    <min::packed_vec_insptr<min::gen> >
        double_quote_arguments
	    ( min::gen_packed_vec_type.new_stub ( 6 ) );
    min::push ( double_quote_arguments ) = s;
    min::push ( double_quote_arguments ) = period;
    min::push ( double_quote_arguments ) = question;
    min::push ( double_quote_arguments ) = exclamation;
    min::push ( double_quote_arguments ) =
        PARLEX::colon;
    min::push ( double_quote_arguments ) =
        PARLEX::semicolon;

    PAR::reformatter text_reformatter =
        PAR::find_reformatter
	    ( text_name,
	      BRA::untyped_reformatter_stack );

    BRA::push_brackets
        ( left_double_quote,
          right_double_quote,
	  code + text,
	  block_level, PAR::top_level_position,
	  TAB::new_flags
	      ( text, code + math + data, 0 ),
	  text_reformatter, double_quote_arguments,
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
          min::MISSING(),
          oper_pass->oper_table );
#endif
