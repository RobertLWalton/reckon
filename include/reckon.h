// Reckon Programming Language
//
// File:	reckon.h
// Author:	Bob Walton (walton@acm.org)
// Date:	Thu May 29 02:29:34 AM EDT 2025
//
// The authors have placed this program in the public
// domain; they make no warranty and accept no liability
// for this program.

# ifndef RECKON_H
# define RECKON_H

# include <mex.h>
# include <mexstack.h>
# include <ll_parser.h>

namespace reckon {

    namespace lexeme {
        extern min::locatable_gen
	    reckon;
    }

    // If true, printer warning message when a next
    // variable is promoted to a lesser depth.
    //
    extern bool warn_next_variable_promotion;

    // Compiler symbol table and modifying_ops table
    // taken from parser primary pass.
    //
    extern min::locatable_var
           <ll::parser::table::key_table>
	   symbol_table;
    extern min::locatable_gen
           modifying_ops;

    // Initialize parser by adding RECKON special parser
    // table entries.  Call after ll::parser::init is
    // called and before ll::parser::parse is called.
    //
    // The parser printer and mexcom::printer should be
    // set immediately after calling this function, and
    // are used by the functions below.
    //
    void init_parser ( ll::parser::parser parser );

    // Call to set symbol_table and modifying_ops above
    // and initialize compiler mexcom and mexstack
    // variables.  Does NOT initialize mexcom::output_
    // module, input_file, printer, and print_switch
    // variables.  Should be called just before each
    // module is compiled.
    //
    void init_compiler ( ll::parser::parser parser );

    // Create a module containing builtin variables and
    // operators (e.g., *TRUE* and *FALSE*), load
    // definitions of these into the symbol_table, and
    // return the module.  You must call `execute' below
    // to run and finish the module.
    //
    // This function begins by calling init_compiler.
    // This function cannot have any errors.
    //
    mex::module load_builtins
        ( ll::parser::parser parser );

    // Read file, create a module for that file, compile
    // the file, and return the module.  You must call
    // `execute' below to run and finish the module.
    //
    // Definitions of global variables and functions
    // defined by the file are loaded into the
    // symbol_table.
    //
    // If the load is successful, nothing is printed
    // and the module is returned.  Otherwise error or
    // warning messages are printed and NULL_STUB is
    // returned.
    //
    // This function begins by calling init_compiler.
    //
    mex::module load_file ( ll::parser::parser parse,
                            const char * file_name );

    // The load_file routine sets loading to true, and
    // then calls the parse function to parse and
    // compile (see remove_tokens in reckon.cc).  Errors
    // are detected by setting parser->error/warning_
    // count and mexcom::error/warning_count to zero
    // before calling the parse function and examining
    // them after the call.
    // 
    extern bool loading;

    // Execute and finish a module that was loaded by
    // a load_... function.  Return true if no errors,
    // in which case nothing is printed.  Return false
    // if there were errors, in which case error
    // messages are printed.
    //
    // This function creates a process to run the module
    // and copies the process stack to the module
    // globals during finishing, so the process is no
    // longer needed.
    //
    bool execute ( mex::module m );

    // Call to compile next statement.  Return true if
    // no error, false if error (and print error
    // message including source lines).
    //
    // Nresults is the number of results expected by
    // the call instruction that implements any
    // statement that is nothing but a single call
    // expression.  It is usually 0, but can be
    // mex::ALL_RESULTS for a statement compiled and
    // executed by the reckon program top level.
    //
    bool compile_statement ( min::gen statement,
                             min::uns32 nresults = 0 );
}

# endif // RECKON_H
