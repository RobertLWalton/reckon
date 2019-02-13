// Reckon Interpreter/Compiler
//
// File:	reckon.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Tue Feb 12 22:23:32 EST 2019
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

// # include <ll_lexeme.h>
# include <ll_lexeme_test.h>
// # include <ll_lexeme_standard.h>
// # include <iostream>
// # include <cassert>

int main ( int argc, const char * argv[] )
{
    min::initialize();
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

    if (    argc > 1 
         && strcmp ( argv[1], "--lexeme-test" ) == 0 )
	LEX::test_input ( LEXSTD::end_of_file_t );

    return 0;
}
