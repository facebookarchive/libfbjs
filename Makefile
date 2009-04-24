all: libfbjs.so

install:
	cp libfbjs.so /usr/lib64/libfbjs.so

parser.lex.cpp: parser.l
	flex -o $@ -d $<

parser.yacc.cpp: parser.y
	bison --debug --verbose -d -o $@ $<

libfbjs.so: parser.lex.cpp parser.yacc.cpp parser.cpp node.cpp fbjs.cpp
	g++ -ggdb -fPIC -shared -Wall $^ -o $@

fbjs: parser.lex.cpp parser.yacc.cpp parser.cpp node.cpp fbjs.cpp cli.cpp
	g++ -ggdb -Wall $^ -o $@

troy: parser.lex.cpp parser.yacc.cpp parser.cpp node.cpp troy.cpp
	g++ -ggdb -Wall $^ -o $@

jsbeautify: parser.lex.cpp parser.yacc.cpp parser.cpp node.cpp jsbeautify.cpp
	g++ -ggdb -Wall $^ -o $@

jsexports: parser.lex.cpp parser.yacc.cpp parser.cpp node.cpp jsexports.cpp
	g++ -ggdb -Wall $^ -o $@

clean:
	rm -f fbjs troy parser.yacc.cpp parser.lex.cpp libfbjs.so
