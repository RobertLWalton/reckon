# Reckon Makefile
#
# File:		Makefile
# Author:	Bob Walton (walton@acm.org)
# Date:		Sun Aug 11 17:16:08 EDT 2024
#
# The authors have placed this program in the public
# domain; they make no warranty and accept no liability
# for this program.

R = ../../test/reckon

.SUFFIXES:
.SUFFIXES: .rec .out .mex .parse \
	   .run_test .run_diff \
	   .compile_test .compile_diff \
	   .parse_test .parse_diff

DIFFOPT ?= -u

RUN_TESTS = reckon_number_constants \
	    reckon_assignments
PARSE_TESTS = reckon_lexemes \
	      reckon_logical_lines \
	      ${RUN_TESTS}

.SECONDARY:	${RUN_TESTS:=.out} \
		${RUN_TESTS:=.mex} \
		${PARSE_TESTS:=.parse}

all:		parse_diff compile_diff run_diff

parse_diff:	${PARSE_TESTS:=.parse_diff}
compile_diff:	${RUN_TESTS:=.compile_diff}
run_diff:	${RUN_TESTS:=.run_diff}

%.parse:	../examples/%.rec Makefile $R
	rm -f $*.parse
	$R --output-parse <../examples/$*.rec >$*.parse

%.mex:	../examples/%.rec Makefile $R
	rm -f $*.mex
	$R --compile <../examples/$*.rec >$*.mex

%.run:	../examples/%.rec Makefile $R
	rm -f $*.run
	$R --run <../examples/$*.rec >$*.run

%.parse_test:	%.parse Makefile
	rm -f $*.parse_test
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
	    < $*.parse > $*.parse_test

%.parse_diff:	%.parse Makefile
	@echo DIFFING $*.parse_test $*.parse
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
	    < $*.parse | diff ${DIFFOPT} $*.parse_test -

clean:
	rm -f ${PARSER_TESTS:=.out} reckon *.o
