// Reckon Programming Language
//
// File:	reckon.h
// Author:	Bob Walton (walton@acm.org)
// Date:	Tue May 27 05:23:45 PM EDT 2025
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

    // Initialize parser by adding RECKON special parser
    // table entries.  Call after ll::parser::init is
    // called and before ll::parser::parse is called.
    //
    void init_parser ( ll::parser::parser parser );

    // Create a module containing builtin variables and
    // operators, load definitions of these into the
    // symbol_table below, and return the module.  Call
    // execute below to run and finish the module.
    //
    // This function must be called before any other
    // modules are compiled.
    //
    mex::module load_builtins
        ( ll::parser::parser parser );

    // Read file, create a module for that file, compile
    // the file, and return the module.  Call execute
    // below to run and finish the module.
    //
    // Definitions of global variables and functions
    // defined by the file are loaded into the
    // symbol_table below.
    //
    // If the load is successful, nothing is printed
    // and the module is returned.  Otherwise error or
    // warning messages are printed and NULL_STUB is
    // returned.
    //
    mex::module load_file ( ll::parser::parser parse,
                            const char * file_name );

    // Execute and finish a module that was loaded by
    // a load_... function.  Return true if no errors,
    // in which case nothing is printed.  Return false
    // if there were errors, in which case error
    // messages are printed.
    //
    bool execute ( mex::module m );

    // Call to init compiler.  Parser->input_file must
    // be set before this function is called.
    //
    void init_compiler
	( ll::parser::parser parser,
          mexstack::print print_switch );

    // Set by any load_builtins to the symbol table.
    //
    extern min::locatable_var
           <ll::parser::table::key_table>
	   symbol_table;

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
