XMLSRC = cookbook.xml \
	chap1.xml \
	chap2.xml \
	chap2_main.xml \
	chap2_app.xml \
	chap2_window.xml \
	gfdl.xml

all: html

html: db2html-dir

db2html-dir: $(XMLSRC) cookbook.dsl
	../tools/mydb2html -d `pwd`/cookbook.dsl#html cookbook.xml

clean:
	rm -rf db2html-dir db2html-dir.junk
	rm -f *~
