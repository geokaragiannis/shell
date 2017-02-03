myshell: myshell.c
	cc lex.yy.c myshell.c -lfl -o myshell
clean: 
	rm -f myshell