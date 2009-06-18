CPPFLAGS=-fPIC -Wall -ggdb

all: libfbjs.so

install:
	cp libfbjs.so /usr/lib64/libfbjs.so

parser.lex.cpp: parser.l
	flex -o $@ -d $<

parser.yacc.cpp: parser.y
	bison --debug --verbose -d -o $@ $<

dmg_fp_dtoa.c:
	curl 'http://www.netlib.org/fp/dtoa.c' -o $@

dmg_fp_g_fmt.c:
	curl 'http://www.netlib.org/fp/g_fmt.c' -o $@

dmg_fp_dtoa.o: dmg_fp_dtoa.c
	$(CC) -fPIC -c $< -o $@ -DIEEE_8087=1 -DNO_HEX_FP=1 -DLong=int32_t -DULong=uint32_t -include stdint.h

dmg_fp_g_fmt.o: dmg_fp_g_fmt.c
	$(CC) -fPIC -c $< -o $@ -DIEEE_8087=1 -DNO_HEX_FP=1 -DLong=int32_t -DULong=uint32_t -include stdint.h

libfbjs.a: parser.yacc.o parser.lex.o parser.o node.o dmg_fp_dtoa.o dmg_fp_g_fmt.o
	$(AR) rc $@ $^
	$(AR) -s $@

libfbjs.so: libfbjs.a fbjs.o
	$(CC) -fPIC -shared $^ -o $@

fbjs: cli.cpp fbjs.cpp libfbjs.a
	$(CXX) -ggdb -Wall $^ -o $@

troy: troy.cpp libfbjs.a
	$(CXX) -ggdb -Wall $^ -o $@

jsbeautify: jsbeautify.cpp libfbjs.a
	$(CXX) -ggdb -Wall $^ -o $@

jsexports: jsexports.cpp libfbjs.a
	$(CXX) -ggdb -Wall $^ -o $@

clean:
	$(RM) -f fbjs troy jsbeautify jsexports \
    parser.lex.cpp parser.yacc.cpp parser.yacc.hpp parser.yacc.output \
    libfbjs.so libfbjs.a \
    dmg_fp_dtoa.o dmg_fp_g_fmt.o \
    parser.lex.o parser.yacc.o parser.o node.o fbjs.o
