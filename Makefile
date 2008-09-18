all: fbjs

parser.lex.cpp: parser.l
	flex -o $@ -d $<

parser.yacc.cpp: parser.y
	bison --debug --verbose -d -o $@ $<

fbjs: parser.lex.cpp parser.yacc.cpp parser.cpp node.cpp fbjs.cpp cli.cpp
	g++ -ggdb -Wall $^ -o $@

troy: parser.lex.cpp parser.yacc.cpp parser.cpp node.cpp troy.cpp
	g++ -ggdb -Wall $^ -o $@

clean:
	rm -f fbjs troy parser.yacc.cpp parser.lex.cpp
