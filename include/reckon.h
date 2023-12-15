// Reckon Programming Language
//
// File:	reckon.h
// Author:	Bob Walton (walton@acm.org)
// Date:	Sat Jan 26 03:20:37 EST 2019
//
// The authors have placed this program in the public
// domain; they make no warranty and accept no liability
// for this program.

# ifndef RECKON_H
# define RECKON_H

# include <ll_parser.h>

namespace reckon {

    extern const char * log_directory;
    extern const char * page_directory;

    namespace lexeme {
        extern min::locatable_gen
	    reckon;
    }
    void init_parser
        ( ll::parser::parser parser );
}

# endif // RECKON_H
