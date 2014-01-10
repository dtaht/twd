title=draft-taht-twd-00
objects=$(title).html $(title).xml $(title).txt

all: $(title).html $(title).xml $(title).txt

FILES=back.mkd middle.mkd template.xml

$(title).txt: $(FILES)
	bash pandoc2rfc -t template.xml -x transform.xsl -T back.mkd middle.mkd 

$(title).html: $(FILES)
	bash pandoc2rfc -t template.xml -x transform.xsl -H back.mkd middle.mkd 

$(title).xml: $(FILES)
	bash pandoc2rfc -t template.xml -x transform.xsl -X back.mkd middle.mkd 

.PHONY: clean
clean:
	rm -f $(objects)
