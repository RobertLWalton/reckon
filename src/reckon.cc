// Reckon Interpreter/Compiler
//
// File:	reckon.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Sat Sep 30 06:49:26 EDT 2023
//
// The authors have placed this program in the public
// domain; they make no warranty and accept no liability
// for this program.

# include <reckon.h>
# include <ll_lexeme_test.h>
# include <ll_parser_standard.h>
# define REC reckon
# define PAR ll::parser
# define TAB ll::parser::table
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
bool parse = false;
bool output_parse = false;
bool subexpression_parse = false;
bool parse_detail = false;
bool output_html = false;

static void remove_tokens
    ( PAR::parser parser,
      PAR::output output )
{
    MIN_REQUIRE ( parser->finished_tokens == 1 );
    min::print_phrase_lines
        ( parser->printer,
	  parser->input_file,
	  parser->first->position );
    parser->printer << parser->first->value << min::eol;
    MIN_REQUIRE
        ( parser->first->next != parser->first );
    PAR::remove ( parser,
                  parser->first,
		  parser->first->next );
    -- parser->finished_tokens;
}

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
	TEST ( "--parse", parse )
	TEST ( "--output-parse", output_parse )
	TEST ( "--subexpression-parse",
	       subexpression_parse )
	TEST ( "--parse-detail", parse_detail )
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

    if ( subexpression_parse )
        output_parse = true;
    if ( output_parse )
        parse = true;

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
	min::printer printer =
	    PAR::default_parser->printer;
	printer << min::output_html;
	for ( const char ** p = html_prefix; * p; ++ p )
	    min::tag(printer) << * p << std::endl;
	min::tag(printer) << "<pre>";
	    // Without std::endl.
    }

    if ( parse )
    {

	PAR::default_parser->trace_flags |=
	    PAR::TRACE_PARSER_COMMANDS
	    +
	    PAR::TRACE_SUBEXPRESSION_LINES;

	if ( output_parse )
	    PAR::default_parser->trace_flags |=
		PAR::TRACE_PARSER_OUTPUT
		+
		PAR::TRACE_SUBEXPRESSION_ELEMENTS;

	int trace_index;
	min::locatable_gen bracketed_subexpressions_name
	    ( min::new_lab_gen
		  ( "bracketed", "subexpressions" ) );
	trace_index = TAB::get_index
	      ( PAR::default_parser->
	             trace_flag_name_table,
		bracketed_subexpressions_name );
	MIN_ASSERT ( trace_index >= 0,
		     "no bracketed subexpressions"
		     " trace flag" );
	TAB::flags bracketed_subexpressions =
	    1ull << trace_index;

	min::locatable_gen operator_subexpressions_name
	    ( min::new_lab_gen
		  ( "operator", "subexpressions" ) );
	trace_index = TAB::get_index
	      ( PAR::default_parser->
	             trace_flag_name_table,
		operator_subexpressions_name );
	MIN_ASSERT ( trace_index >= 0,
		     "no operator subexpressions"
		     " trace flag" );
	TAB::flags operator_subexpressions =
	    1ull << trace_index;

	if ( subexpression_parse )
	    PAR::default_parser->trace_flags |=
		bracketed_subexpressions
		+
		operator_subexpressions;

	if ( parse_detail )
	    PAR::default_parser->trace_flags |=
		PAR::TRACE_SUBEXPRESSION_DETAILS;

	PAR::parse();

    }
    else
    {
        min::locatable_var<PAR::output> output;
	PAR::init ( output, ::remove_tokens, NULL );
	PAR::output_ref ( PAR::default_parser ) =
	    output;
	PAR::parse();
    }

    if ( output_html )
    {
	min::printer printer =
	    PAR::default_parser->printer;

	for ( const char ** p = html_postfix; * p;
	                                      ++ p )
	    min::tag(printer) << * p << std::endl;
    }

    return 0;
}
