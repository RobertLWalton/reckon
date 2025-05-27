// Reckon Interpreter/Compiler
//
// File:	reckon.cc
// Author:	Bob Walton (walton@acm.org)
// Date:	Tue May 27 05:46:53 PM EDT 2025
//
// The authors have placed this program in the public
// domain; they make no warranty and accept no liability
// for this program.

# include <reckon.h>
# include <ll_lexeme_test.h>
# include <ll_parser_standard.h>
# include <mex.h>
# include <mexcom.h>
# include <mexstack.h>
# define REC reckon
# define PAR ll::parser
# define PARLEX ll::parser::lexeme
# define TAB ll::parser::table
# define PARSTD ll::parser::standard
# define LEX ll::lexeme
# define LEXSTD ll::lexeme::standard

bool REC::warn_next_variable_promotion = false;

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

bool output_html = false;
bool raw_html = false;

bool compile = false;
bool run = false;
bool trace = false;

bool lexeme_test = false;

bool parser_test = false;
bool output_parse = false;
bool subexpression_parse = false;
bool detail_parse = false;
bool warn = false;

bool load = false;
bool load_OK = false;

static min::locatable_var<mex::process> process;

static min::printer computed_pgen
    ( min::printer printer,
      min::gen v, 
      const min::gen_format * gen_format,
      bool disable_mapping )
{
    if ( ! min::is_obj ( v )
         ||
	    min::get ( v, min::dot_initiator )
	 != PARLEX::left_square )
        gen_format = min::quote_gen_format;
    return ( * min::quote_gen_format->pgen )
        ( printer, v, gen_format, disable_mapping );
}

static min::gen_format computed_gen_format;
    // Same as min::quote_gen_format except for:
    // & ::computed_pgen           // pgen
    // & ::computed_obj_format     // obj_format
    //
static min::obj_format computed_obj_format;
    // Same as min::compact_obj_format except for:
    // & ::computed_gen_format     // element_format
    //
static void init_computed_formats ( void )
{
    ::computed_gen_format = * min::quote_gen_format;
    ::computed_gen_format.pgen = & ::computed_pgen;
    ::computed_gen_format.obj_format =
       & ::computed_obj_format;

    ::computed_obj_format = * min::compact_obj_format;
    ::computed_obj_format.element_format =
        & ::computed_gen_format;
}

static min::phrase_position last_position =
    { { 0, 0 }, { 0, 0 } };
static min::uns64 parse_error_count = 0;
static void remove_tokens
    ( PAR::parser parser,
      PAR::output output )
{
    MIN_REQUIRE
        ( parser->first->next->next != parser->first );
    MIN_REQUIRE ( parser->finished_tokens == 1 );

    bool parse_OK =
        ( parse_error_count == parser->error_count );
    parse_error_count = parser->error_count;

    if ( ! compile && ! load )
    {
	last_position.begin = last_position.end;
	last_position.end =
	    parser->first->next->position.end;
	if ( last_position.end.offset > 0 )
	    last_position.end =
		{ last_position.end.line + 1, 0 };
	min::print_phrase_lines
	    ( parser->printer,
	      parser->input_file,
	      last_position,
	      min::standard_line_format );
    }

    bool compile_OK = false;
    mexstack::compile_save_area area;
    TAB::root top;

    if ( ! parse_OK )
        /* do nothing */;
    else if ( load && ! load_OK )
        /* do nothing */;
    else if ( compile || run || load )
    {
	mexstack::save ( area );
	top = TAB::top ( reckon::symbol_table );
	compile_OK = REC::compile_statement
	    ( parser->first->next->value,
	      mex::ALL_RESULTS );
	bool r = mexstack::restore ( area );
	MIN_REQUIRE ( ! r == compile_OK );
	if ( ! compile_OK )
	    while (    TAB::top ( reckon::symbol_table )
	            != top )
	        TAB::pop ( reckon::symbol_table );
    }

    if ( load )
    {
	// Do nothing but note errors.
	//
        if ( ! parse_OK || ! compile_OK )
	    load_OK = false;
    }
    else if ( output_parse )
	 parser->printer
	    << parser->first->next->value
	    << min::eol;

    else if ( ! parse_OK )
        parser->printer
	    << "[no "
	    << ( compile && run ?
                    "compilation or run output" :
                 compile ?
                    "compilation" :
		    "output" )
	    << " due to above parse errors]"
	    << min::eol;

    else if ( run & ! compile_OK )
    {
        parser->printer
	    << "[no output due to compile error]"
	    << min::eol;
    }
    else if ( run )
    {
	min::uns32 length = ::process->length;
	mex::run_process ( ::process );
        if ( ::process->state != mex::MODULE_END )
	{
	    // Restore compiler and process state.
	    //
	    min::pop ( ::process,
	               ::process->length - length );
	    mexstack::restore ( area, true );
	    while (    TAB::top ( reckon::symbol_table )
	            != top )
	        TAB::pop ( reckon::symbol_table );

	    // Set pc to module end.
	    //
	    mex::pc pc = ::process->pc;
	    pc.index = mexcom::output_module->length;
	    mex::set_pc ( ::process, pc );
	    parser->printer
		<< "[no output due to run-time error]"
		<< min::eol;
	}
	if ( ! trace )
	    while ( length < ::process->length )
	    {
	        min::gen v = ::process[length++];
		    // Discard min::ref.
	        min::print_gen
		    ( parser->printer, v,
		      & ::computed_gen_format );
		parser->printer << min::eol;
	    }
    }

    PAR::remove ( parser,
                  parser->first->next,
		  parser->first->next->next );
    -- parser->finished_tokens;
}

int main ( int argc, const char * argv[] )
{
    min::initialize();
    ::init_computed_formats();

    bool found_error = false;
    int i = 1;
    for ( ; i < argc; ++ i )
    {
#	define TEST(x,y) \
	    if ( strcmp ( argv[i], x ) == 0 ) \
	        y = true; \
	    else
	TEST ( "--output-html", output_html )
	TEST ( "--raw-html", raw_html )

	TEST ( "--compile", compile )
	TEST ( "--run", run )
	TEST ( "--trace", trace )

	TEST ( "--lexeme-test", lexeme_test )
	TEST ( "--output-parse", output_parse )
	TEST ( "--subexpression-parse",
	       subexpression_parse )
	TEST ( "--detail-parse", detail_parse )
	TEST ( "--warn", warn )
	    // ends with `else'
#	undef TEST

	if ( strcmp ( argv[i], "-" ) == 0 )
	    break;
	else
	if ( strncmp ( argv[i], "-", 1 ) != 0 )
	    break;
	else
	{
	    std::cerr
	        << "ERROR: unrecognized argument "
	        << argv[i] << std::endl;
	    found_error = true;
	}
    }
    if ( found_error ) exit ( 1 );

    if ( subexpression_parse || detail_parse )
    {
        output_parse = true;
        parser_test = true;
    }
    if ( trace )
    {
	run = true;
	mex::run_trace_flags = 0xFFFFFFFF;
    }
    if ( output_parse && ( compile || run ) )
    {
	std::cerr
	    << "ERROR: --compile and --run incompatible"
               " with --*-parse" << std::endl;
	exit ( 1 );
    }
    if (    lexeme_test
         && ( output_parse || compile || run ) )
    {
	std::cerr
	    << "ERROR: --*-parse, --compile, and --run"
               " incompatible with --lexeme-test"
            << std::endl;
	exit ( 1 );
    }
    if ( run && compile )
    {
	std::cerr
	    << "ERROR: --run incompatible "
               "  with --compile"
            << std::endl;
	exit ( 1 );
    }

    if ( warn )
    {
	REC::warn_next_variable_promotion = true;
    }

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
		- PARSTD::SENTENCE
		- PARSTD::BITWISE_OPERATORS );

    REC::init_parser ( PAR::default_parser );
    PAR::init_printer_ostream
        ( PAR::default_parser, std::cout );
    mexcom::printer = PAR::default_parser->printer;
    min::init ( PAR::input_file_ref
                    ( PAR::default_parser ) );
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
    else if ( raw_html )
	PAR::default_parser->printer
	    << min::output_html;

    if ( parser_test )
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

	if ( detail_parse )
	    PAR::default_parser->trace_flags |=
		PAR::TRACE_SUBEXPRESSION_DETAILS;

    }
    else
    {
	if ( compile || run )
	    REC::init_compiler
		( PAR::default_parser,
		  compile ? mexstack::PRINT_WITH_SOURCE
			  : mexstack::NO_PRINT );
	if ( run )
	{
	    ::process = mex::init_process
		( mexcom::output_module );
	    mex::run_process ( ::process );
	        // Creates builtin variables, e.g.,
		// TRUE and FALSE.
	}

        min::locatable_var<PAR::output> output;
	PAR::init ( output, ::remove_tokens, NULL );
	PAR::output_ref ( PAR::default_parser ) =
	    output;

    }

    if ( i == argc )
    {
	PAR::init_input_stream
	    ( PAR::default_parser, std::cin,
	      min::marked_line_format );
	PAR::parse();
    }
    else
    {
        // TBD: Loop to load files goes here.
	//
	std::cerr
	    << "ERROR: unrecognized argument "
	    << argv[i] << std::endl;
	found_error = true;
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
