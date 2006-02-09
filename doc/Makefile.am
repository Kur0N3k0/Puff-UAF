info_TEXINFOS = puff.texi
EXTRA_DIST = puff.pdf puff.ps puff.html

# my own addition not currently supported by autotools
html_docs = puff.html
.texi.html:
	$(MAKEINFO) --html --no-split $<

pdf_docs = puff.pdf
.texi.pdf:
	$(TEXI2DVI) --pdf $<

if BUILD_INSTALL_DOCS
  .PHONY: html_docs pdf_docs

install-data-local:
	$(mkinstalldirs) $(prefix)/share/doc
	$(MAKE) puff.pdf
	$(INSTALL_DATA) puff.pdf $(prefix)/share/doc/puff.pdf

endif
