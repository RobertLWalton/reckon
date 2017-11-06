# Reckon Makefile
#
# File:		Makefile
# Author:	Bob Walton (walton@acm.org)
# Date:		Mon Nov  6 01:56:11 EST 2017
#
# The authors have placed this program in the public
# domain; they make no warranty and accept no liability
# for this program.

# The following locates the MIN and LAYERED source code.
#
MIN = ~/min
LAYERED = ~/layered

# The following turns on MIN debugging.
#
GFLAGS = -DMIN_PROTECT=1
WFLAGS = -Wall -Wno-char-subscripts

.SUFFIXES:
.SUFFIXES: .out .test .diff

# O3 is needed to inline functions.
#
OPTIMIZE = -O3

RECKON_TESTS = reckon_context_test

TESTS = ${RECKON_TESTS}

MIN_H =	${MIN}/include/min.h \
        ${MIN}/include/min_parameters.h \
        ${MIN}/include/min_os.h \
        ${MIN}/include/min_unicode.h \
        ${MIN}/unicode/unicode_data.h \
        ${MIN}/unicode/unicode_data_support_sets.h


MIN_O =	min_assert.o min.o min_os.o min_acc.o \
        min_unicode.o

LEXEME_H = ${MIN_H} \
	   ${LAYERED}/include/ll_lexeme_standard.h \
	   ${LAYERED}/include/ll_lexeme.h

LEXEME_O = ${MIN_O} \
	   ll_lexeme.o \
	   ll_lexeme_ndl.o \
	   ll_lexeme_standard.o

LLI = ${LAYERED}/include
PARSER_H = ${LEXEME_H} \
	   ${LLI}/ll_lexeme_alternate.h \
	   ${LLI}/ll_lexeme_c++.h \
	   ${LLI}/ll_lexeme_combining.h \
	   ${LLI}/ll_lexeme.h \
	   ${LLI}/ll_lexeme_ndl_data.h \
	   ${LLI}/ll_lexeme_ndl.h \
	   ${LLI}/ll_lexeme_non_ascii_characters.h \
	   ${LLI}/ll_lexeme_program_data.h \
	   ${LLI}/ll_lexeme_standard.h \
	   ${LLI}/ll_lexeme_test.h \
	   ${LLI}/ll_parser_bracketed.h \
	   ${LLI}/ll_parser_command.h \
	   ${LLI}/ll_parser.h \
	   ${LLI}/ll_parser_oper.h \
	   ${LLI}/ll_parser_prefix.h \
	   ${LLI}/ll_parser_standard.h \
	   ${LLI}/ll_parser_table.h


PARSER_O = ${LEXEME_O} \
	   ll_parser.o \
	   ll_parser_table.o \
	   ll_parser_command.o \
	   ll_parser_prefix.o \
	   ll_parser_bracketed.o \
	   ll_parser_oper.o \
	   ll_parser_standard.o \
	   ll_parser_standard_input.o \
	   ll_parser_standard_brackets.o \
	   ll_parser_standard_oper.o

.SECONDARY:	${TESTS:=.out}

all:		diff

reckon_diff:	${RECKON_TESTS:=.diff}
diff:		${TESTS:=.diff}

out:		${TESTS:=.out}

test:		${TESTS:=.test}


%.test:	%.out Makefile
	rm -f $*.test
	sed \
	    -e '/\.cc:[0-9]* assert/s//.cc:XXXX assert/' \
	    -e '/\.h:[0-9]* assert/s//.h:XXXX assert/' \
	    -e '/\.cc:[0-9]* desire/s//.cc:XXXX desire/' \
	    -e '/\.L/s/\(\.L[A-Z][A-Z]*\)[0-9][0-9]*/\1XXX/g' \
	    -e '/line [0-9]/s//line #/g' \
	    -e '/\(lines [0-9][0-9]*-\)[0-9]/s//\1#/g' \
	    -e '/lines [0-9]\([0-9]*-#\)/s//lines #\1/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/\$DDate: .*\$D/s//\$DRCS Date: XXX\$D/' \
	    -e '/\$DRevision: .*\$D/s//\$DRCS Revision: XXX\$D/' \
	    -e '/\$DRCSfile: .*\$D/s//\$DRCS RCSfile: XXX\$D/' \
            < $*.out > $*.test

%.diff:	%.out
	@echo DIFFING $*.test $*.out
	-@sed \
	    -e '/\.cc:[0-9]* assert/s//.cc:XXXX assert/' \
	    -e '/\.h:[0-9]* assert/s//.h:XXXX assert/' \
	    -e '/\.cc:[0-9]* desire/s//.cc:XXXX desire/' \
	    -e '/\.L/s/\(\.L[A-Z][A-Z]*\)[0-9][0-9]*/\1XXX/g' \
	    -e '/line [0-9]/s//line #/g' \
	    -e '/\(lines [0-9][0-9]*-\)[0-9]/s//\1#/g' \
	    -e '/lines [0-9]\([0-9]*-#\)/s//lines #\1/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/\$DDate: .*\$D/s//\$DRCS Date: XXX\$D/' \
	    -e '/\$DRevision: .*\$D/s//\$DRCS Revision: XXX\$D/' \
	    -e '/\$DRCSfile: .*\$D/s//\$DRCS RCSfile: XXX\$D/' \
            < $*.out | diff --minimal -u ${DIFF_FLAGS} $*.test -

%.out:	%
	rm -f $*.out
	${RUNENV} $* > $*.out

%.lex:	../src/%.lexcc Makefile
	sed -n \
		-e '/^lex	/s///p' \
		-e '/^lex$$/s///p' \
		< ../src/$*.lexcc \
		> $*.lex

%.cc:	../src/%.lexcc Makefile
	sed -n \
		-e '/^cc/!s/.*/\/\//p' \
		-e '/^cc	/s///p' \
		-e '/^cc$$/s///p' \
		< ../src/$*.lexcc \
		> $*.cc

min.o:	${MIN_H} \
	${MIN}/src/min.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${MIN}/include \
	    -c ${MIN}/src/min.cc

min_assert.o:	${MIN}/src/min_assert.cc \
		${MIN}/include/min_parameters.h
	rm -f min_assert.o
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${MIN}/include \
	    -c ${MIN}/src/min_assert.cc \

min_os.o:	${MIN_H} \
	${MIN}/src/min_os.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${MIN}/include \
	    -c ${MIN}/src/min_os.cc

min_acc.o:	${MIN_H} \
        ${MIN}/include/min_acc.h \
        ${MIN}/include/min_acc_parameters.h \
	${MIN}/src/min_acc.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${MIN}/include \
	    -c ${MIN}/src/min_acc.cc

min_unicode.o:	${MIN_H} \
        ${MIN}/include/min_unicode.h \
	${MIN}/src/min_unicode.cc \
	${MIN}/unicode/unicode_data.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${MIN}/include \
	    -c ${MIN}/src/min_unicode.cc

ll_lexeme.o:	\
	${PARSER_H} \
	${LAYERED}/src/ll_lexeme.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ../include -I ${MIN}/include \
	    -c ../src/ll_lexeme.cc

ll_lexeme_ndl.o:	\
		${MIN_H} \
		${LAYERED}/src/ll_lexeme_ndl.cc \
		../include/ll_lexeme.h \
		../include/ll_lexeme_ndl.h \
		../include/ll_lexeme_ndl_data.h \
		Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ../include -I ${MIN}/include \
	    -c ../src/ll_lexeme_ndl.cc

ll_lexeme_standard.lex:	\
		${LAYERED}/src/ll_lexeme_standard.lexcc

ll_lexeme_standard.cc:	\
		${LAYERED}/src/ll_lexeme_standard.lexcc

ll_parser.o:	\
	${PARSER_H} \
	${LAYERED}/src/ll_parser.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${LAYERED}/include -I ${MIN}/include \
	    -c ${LAYERED}/src/ll_parser.cc

ll_parser_standard_input.o:	\
	${PARSER_H} \
	${LAYERED}/src/ll_parser_standard_input.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${LAYERED}/include -I ${MIN}/include \
	    -c ${LAYERED}/src/ll_parser_standard_input.cc

ll_parser_table.o:	\
	${PARSER_H} \
	${LAYERED}/src/ll_parser_table.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${LAYERED}/include -I ${MIN}/include \
	    -c ${LAYERED}/src/ll_parser_table.cc

ll_parser_standard.o:	\
	${PARSER_H} \
	${LAYERED}/src/ll_parser_standard.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${LAYERED}/include -I ${MIN}/include \
	    -c ${LAYERED}/src/ll_parser_standard.cc

ll_parser_prefix.o:	\
	${PARSER_H} \
	${LAYERED}/src/ll_parser_prefix.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${LAYERED}/include -I ${MIN}/include \
	    -c \
	    ${LAYERED}/src/ll_parser_prefix.cc

ll_parser_bracketed.o:	\
	${PARSER_H} \
	${LAYERED}/src/ll_parser_bracketed.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${LAYERED}/include -I ${MIN}/include \
	    -c \
	    ${LAYERED}/src/ll_parser_bracketed.cc

ll_parser_standard_brackets.o:	\
	${PARSER_H} \
	${LAYERED}/src/ll_parser_standard_brackets.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${LAYERED}/include -I ${MIN}/include \
	    -c ${LAYERED}/src/ll_parser_standard_brackets.cc

ll_parser_oper.o:	\
	${PARSER_H} \
	${LAYERED}/src/ll_parser_oper.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${LAYERED}/include -I ${MIN}/include \
	    -c ${LAYERED}/src/ll_parser_oper.cc

ll_parser_standard_oper.o:	\
	${PARSER_H} \
	${LAYERED}/src/ll_parser_standard_oper.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${LAYERED}/include -I ${MIN}/include \
	    -c ${LAYERED}/src/ll_parser_standard_oper.cc

ll_parser_command.o:	\
	${PARSER_H} \
	${LAYERED}/src/ll_parser_command.cc \
	Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ${LAYERED}/include -I ${MIN}/include \
	    -c ${LAYERED}/src/ll_parser_command.cc

clean:
	rm -f ${TESTS} ${TESTS:=.out} *.o \
	      ll_lexeme_standard.cc \
	      ll_lexeme_standard.lex
