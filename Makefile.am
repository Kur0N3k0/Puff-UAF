data_files = data/2002092500_avn-U.cdf data/2002092500_avn-V.cdf data/2003042300_cmc.cdf
etc_files = etc/volcanos.txt etc/puffrc
doc_files = license.terms INSTALL


EXTRA_DIST = $(data_files) $(etc_files) $(doc_files)

SUBDIRS = doc man src test web 
	
install-data-local:
	$(mkinstalldirs) $(prefix)/etc
	$(INSTALL_DATA) etc/volcanos.txt $(prefix)/etc/volcanos.txt
	$(INSTALL_DATA) etc/puffrc $(prefix)/etc/puffrc

