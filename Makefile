parser.lex.cpp: parser.l
	flex -o $@ -d -L $<

parser.yacc.cpp: parser.y
	bison --debug --verbose -d -o $@ $<

fbjs: parser.lex.cpp parser.yacc.cpp node.cpp fbjs.cpp
	g++ -ggdb -Wall $^ -o $@

.PHONY: clean

clean:
	rm -f fbjs parser.yacc.cpp parser.lex.cpp

#target: dependencies
# command 1
# command 2

# $@ = target
# $< = dependencies
