// Reckon Programming Language
//
// File:	reckon.h
// Author:	Bob Walton (walton@acm.org)
// Date:	Tue Dec 24 07:40:10 PM EST 2024
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

    // Call to init compiler.  Parser->input_file must
    // be set before this function is called.
    //
    void init_compiler
	( ll::parser::parser parser,
          mexstack::print print_switch );

    // Set by init_complier to symbol table.
    //
    extern min::locatable_var
           <ll::parser::table::key_table>
	   symbol_table;

    // Call to compile next statement.  Return true if
    // no error, false if error (and print error
    // message including source lines).
    //
    bool compile_statement ( min::gen statement );
}

# endif // RECKON_H
