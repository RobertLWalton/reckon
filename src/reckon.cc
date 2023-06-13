// Reckon Interpreter/Compiler
//
// File:	reckon.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Tue Jun 13 16:42:58 EDT 2023
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

const char * html_prefix[] = {
    "<!DOCTYPE html>",
    "<html>",
    "<head>",
    "<style>",
    "table.MIN-LINE-TABLE {",
    "border: dashed 2px;",
    "background-color: lightgoldenrodyellow;",
    "}",
    "table.MIN-LINE-TABLE td.MIN-LINE-NUMBER {",
    "background-color: palegreen;",
    "}",
    "table.MIN-LINE-TABLE td span.MIN-LINE-MARK {",
    "background-color: thistle;",
    "}",
    "</style>",
    "</head>",
    "<body>",
    NULL };
const char * html_postfix[] = {
    "</pre>",
    "</body>",
    "</html>",
    NULL };

bool lexeme_test = false;
bool output_parse = false;
bool subexpression_parse = false;
bool subexpression_detail = false;
bool output_html = false;

int main ( int argc, const char * argv[] )
{
    min::initialize();

    bool found_error = false;
    for ( int i = 1; i < argc; ++ i )
    {
#	define TEST(x,y) \
	    if ( strcmp ( argv[i], x ) == 0 ) \
	        y = true; \
	    else
	TEST ( "--lexeme-test", lexeme_test )
	TEST ( "--output-parse", output_parse )
	TEST ( "--subexpression-parse",
	       subexpression_parse )
	TEST ( "--subexpression-detail",
	       subexpression_detail )
	TEST ( "--output-html", output_html )
	{
	    std::cerr
	        << "ERROR: unrecognized argument "
	        << argv[i] << std::endl;
	    found_error = true;
	}
#	undef TEST
    }
    if ( found_error ) exit ( 1 );

    if ( lexeme_test )
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

    if ( output_html )
    {
	min::printer printer = PAR::default_parser->printer;
	printer << min::output_html;
	for ( const char ** p = html_prefix; * p; ++ p )
	    min::tag(printer) << * p << std::endl;
	min::tag(printer) << "<pre>";
	    // Without std::endl.

	PAR::default_parser->trace_flags |=
	    PAR::TRACE_PARSER_COMMANDS;
	PAR::parse();

	for ( const char ** p = html_postfix; * p; ++ p )
	    min::tag(printer) << * p << std::endl;
	                 
	return 0;
    }

    PAR::default_parser->trace_flags |=
	PAR::TRACE_PARSER_COMMANDS;
    PAR::parse();

    return 0;
}
