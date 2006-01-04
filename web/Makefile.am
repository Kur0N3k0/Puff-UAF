SUBDIRS = mini_httpd

bin_SCRIPTS = ashxpOptions.pl login.pl logout.pl main.pl runParams.pl webpuff.pl

PIX_FILES =   pix/puff_bg_dark.jpg \
  pix/puff_bg.gif \
  pix/puff_1.gif \
  pix/puff_dragon.ico \
  pix/updating.gif \
  pix/map-button.gif \
  pix/movie.png

JAVASCRIPT_FILES =   javascript/webpuff.js 

HELP_FILES = help/ashLogMean.html \
  help/diffuseH.html \
  help/diffuseZ.html \
  help/eruptHours.html \
  help/gridlines.html \
  help/latlon.html \
  help/model.html \
  help/nAsh.html \
  help/phiDist.html \
  help/plumeWidth.html \
  help/plumeZwidth.html \
  help/plumeMax.html \
  help/plumeMin.html \
  help/previous.html \
  help/restartFile.html \
  help/runHours.html \
  help/saveHours.html \
  help/webpuff.html \
  help/volcano_name.html


 EXTRA_DIST = webpuff.css \
  $(HELP_FILES) \
  $(JAVASCRIPT_FILES) \
  $(PIX_FILES)

WEBDIR=$(DESTDIR)$(prefix)/web
# move all this web stuff to a separate directory
install-exec-local:
	$(mkinstalldirs) $(WEBDIR)
	$(mkinstalldirs) $(WEBDIR)/help
	$(mkinstalldirs) $(WEBDIR)/pix
	$(mkinstalldirs) $(WEBDIR)/javascript
	$(mkinstalldirs) $(WEBDIR)/run
	$(INSTALL) Webpuff.pm $(WEBDIR)/Webpuff.pm
	$(INSTALL) webpuff.css $(WEBDIR)/webpuff.css
	for i in $(HELP_FILES);do cp $$i $(WEBDIR)/help; done
	for i in $(PIX_FILES);do cp $$i $(WEBDIR)/pix; done
	for i in $(JAVASCRIPT_FILES); do cp $$i $(WEBDIR)/javascript; done
	for i in $(bin_SCRIPTS); do mv $(DESTDIR)$(bindir)/$$i $(WEBDIR);done
