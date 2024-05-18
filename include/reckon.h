// Reckon Programming Language
//
// File:	reckon.h
// Author:	Bob Walton (walton@acm.org)
// Date:	Sat May 18 03:42:56 EDT 2024
//
// The authors have placed this program in the public
// domain; they make no warranty and accept no liability
// for this program.

# ifndef RECKON_H
# define RECKON_H

# include <ll_parser.h>

namespace reckon {

    namespace lexeme {
        extern min::locatable_gen
	    reckon;
    }

    // Initialize parser by adding RECKON special parser
    // table entries.  Call after ll::parser::init is
    // called and before ll::parser::parse is called.
    //
    void init_parser ( ll::parser::parser parser );

    // Call to init compiler.
    //
    void init_compiler ( ll::parser::parser parser );

    // Number of compile errors and warnings so far.
    //
    extern min::uns64 compile_errors, compile_warnings;

    // Call to compile right-side subexpression and
    // leave results in the stack.
    //
    void compile_right_subexpression
	( min::gen expression );
}

# endif // RECKON_H
