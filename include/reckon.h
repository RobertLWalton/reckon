// Reckon Programming Language
//
// File:	reckon.h
// Author:	Bob Walton (walton@acm.org)
// Date:	Tue Jul 23 02:17:22 EDT 2024
//
// The authors have placed this program in the public
// domain; they make no warranty and accept no liability
// for this program.

# ifndef RECKON_H
# define RECKON_H

# include <mex.h>
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

    // Call to init compiler.  Parser->input_file must
    // be set before this function is called.
    //
    void init_compiler ( ll::parser::parser parser );
}

# endif // RECKON_H
