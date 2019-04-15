// Reckon Interpreter/Compiler
//
// File:	reckon.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Mon Apr 15 04:51:07 EDT 2019
//
// The authors have placed this program in the public
// domain; they make no warranty and accept no liability
// for this program.

// Table of Contents
//

// Usage and Setup
// ----- --- -----

# include <ll_parser_standard.h>
# define MUP min::unprotected
# define PAR ll::parser
# define LEX ll::lexeme
# define LEXSTD ll::lexeme::standard
# define PARSTD ll::parser::standard

// # include <ll_lexeme.h>
# include <ll_lexeme_test.h>
// # include <ll_lexeme_standard.h>
// # include <iostream>
// # include <cassert>

int main ( int argc, const char * argv[] )
{
    min::initialize();

    if (    argc > 1 
         && strcmp ( argv[1], "--lexeme-test" ) == 0 )
    {
        // We cannot use parser initialization here
	// because that sets up different scanner
	// erroneous atom handling.
	//
	LEX::init_ostream
		( LEX::default_scanner, std::cout );
	LEXSTD::init_standard_program();
	LEX::init_program ( LEX::default_scanner,
			    LEXSTD::default_program );
	LEX::init_input_stream
	    ( LEX::default_scanner,
	      std::cin,
		min::DISPLAY_PICTURE
	      + min::DISPLAY_NON_GRAPHIC
	      + min::DISPLAY_EOL );
	LEX::test_input ( LEXSTD::end_of_file_t );
	return 0;
    }

    PAR::init ( PAR::default_parser, true );
    PAR::init_input_stream
        ( PAR::default_parser, std::cin,
	    min::DISPLAY_PICTURE
	  + min::DISPLAY_NON_GRAPHIC
	  + min::DISPLAY_EOL );
    PAR::init_ostream
        ( PAR::default_parser, std::cout );
    PAR::init_line_display
	( PAR::default_parser,
	  min::DISPLAY_PICTURE );
    PAR::default_parser->trace_flags |=
	PAR::TRACE_PARSER_COMMANDS;
    PAR::parse();

    return 0;
}

# ifdef NONE_SUCH

    // Since subexpressions are not being parsed, we
    // change the gen_formats to not suppress spaces
    // between lexemes and not quote brackets.
    //
    min::gen_format altered_line_gen_format =
        * min::line_gen_format;
    min::obj_format altered_line_obj_format =
        * min::line_obj_format;
    min::gen_format altered_line_element_gen_format =
        * min::line_element_gen_format;
    min::obj_format altered_line_element_obj_format =
        * min::line_element_obj_format;

    altered_line_gen_format.obj_format =
        & altered_line_obj_format;
    altered_line_obj_format.top_element_format =
        & altered_line_element_gen_format;
    altered_line_element_gen_format.obj_format =
        & altered_line_element_obj_format;
    altered_line_element_gen_format.str_format =
        min::quote_non_graphic_str_format;
    altered_line_element_obj_format.obj_sep =
        (const min::ustring *) "\x01\x01" " ";

    PAR::default_parser->subexpression_gen_format =
        & altered_line_gen_format;

# endif // NONE_SUCH
