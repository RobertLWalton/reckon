// Reckon Interpreter/Compiler
//
// File:	reckon.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Mon Jun  5 15:38:23 EDT 2023
//
// The authors have placed this program in the public
// domain; they make no warranty and accept no liability
// for this program.

# include <reckon.h>
# include <ll_lexeme_test.h>
# include <ll_parser_standard.h>
# define REC reckon
# define PAR ll::parser
# define PARSTD ll::parser::standard
# define LEX ll::lexeme
# define LEXSTD ll::lexeme::standard


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
	      min::eol_line_format );
	LEX::test_input ( LEXSTD::end_of_file_t );
	return 0;
    }

    PAR::init ( PAR::default_parser,
                  PARSTD::ALL
		- PARSTD::BITWISE_OPERATORS );
    PAR::init_ostream
        ( PAR::default_parser, std::cout );
    REC::init_parser ( PAR::default_parser );
    PAR::init_input_stream
        ( PAR::default_parser, std::cin,
	  min::marked_line_format );

    if (    argc > 1 
         && strcmp ( argv[1], "--html" ) == 0 )
    {
	min::printer printer = PAR::default_parser->printer;
	printer << min::output_html;
	min::tag(printer) << "<!DOCTYPE html>" << std::endl
	                  << "<html>" << std::endl
	                  << "<body>" << std::endl
	                  << "<pre>" << std::endl;

	PAR::default_parser->trace_flags |=
	    PAR::TRACE_PARSER_COMMANDS;
	PAR::parse();

	min::tag(printer) << "</pre>" << std::endl 
	                  << "</body>" << std::endl
	                  << "</html>" << std::endl;
	                 
	return 0;
    }

    PAR::default_parser->trace_flags |=
	PAR::TRACE_PARSER_COMMANDS;
    PAR::parse();

    return 0;
}
