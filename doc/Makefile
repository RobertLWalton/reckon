reckon:
	pdflatex -recorder -interaction nonstopmode \
	         -no-shell-escape -file-line-error \
		 reckon > reckon.out 2> reckon.err; \
		 cat reckon.out reckon.err
	makeindex reckon
