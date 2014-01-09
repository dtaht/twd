all:	draft.html draft.xml draft.txt

FILES=back.mkd middle.mkd template.xml

draft.txt: $(FILES)
	bash pandoc2rfc -t template.xml -x transform.xsl -T back.mkd middle.mkd 

draft.html: $(FILES)
	bash pandoc2rfc -t template.xml -x transform.xsl -M back.mkd middle.mkd 

draft.xml: $(FILES)
	bash pandoc2rfc -t template.xml -x transform.xsl -X back.mkd middle.mkd 

.PHONY: clean
clean:
	rm -f draft.txt draft.xml draft.html
