# Makefile for (Sphinx based) restructured text documentation
#

# You can set these variables from the command line.
SPHINXOPTS    =
SPHINXBUILD   = sphinx-build
PAPER         =

QCOLLECTIONGENERATOR = qcollectiongenerator

# Internal variables.
PAPEROPT_a4     = -D latex_paper_size=a4
PAPEROPT_letter = -D latex_paper_size=letter
ALLSPHINXOPTS   = -d build/doctrees $(PAPEROPT_$(PAPER)) $(SPHINXOPTS) .

.PHONY: help clean distclean html web pickle htmlhelp qthelp qhc latex changes linkcheck

help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  html      to make standalone HTML files"
	@echo "  pickle    to make pickle files"
	@echo "  json      to make JSON files"
	@echo "  htmlhelp  to make HTML files and a HTML help project"
	@echo "  qthelp    to make HTML files and a qthelp project"
	@echo "  qhc       to make QHC file"
	@echo "  latex     to make LaTeX files, you can set PAPER=a4 or PAPER=letter"
	@echo "  man       to make manpages"
	@echo "  changes   to make an overview over all changed/added/deprecated items"
	@echo "  linkcheck to check all external links for integrity"

clean:
	-rm -rf build/*

distclean: clean
	-rmdir build/

html:
	mkdir -p build/html build/doctrees
	$(SPHINXBUILD) -b html $(ALLSPHINXOPTS) build/html
	@echo
	@echo "Build finished. The HTML pages are in build/html."

zip: html
	(cd build ; zip TortoiseHg.html.zip -r html)

pickle:
	mkdir -p build/pickle build/doctrees
	$(SPHINXBUILD) -b pickle $(ALLSPHINXOPTS) build/pickle
	@echo
	@echo "Build finished; now you can process the pickle files."

web: pickle

json:
	mkdir -p build/json build/doctrees
	$(SPHINXBUILD) -b json $(ALLSPHINXOPTS) build/json
	@echo
	@echo "Build finished; now you can process the JSON files."

htmlhelp:
	mkdir -p build/htmlhelp build/doctrees
	$(SPHINXBUILD) -b htmlhelp $(ALLSPHINXOPTS) build/htmlhelp
	@echo
	@echo "Build finished; now you can run HTML Help Workshop with the" \
	      ".hhp project file in build/htmlhelp."

qthelp:
	mkdir -p build/qthelp build/doctrees
	$(SPHINXBUILD) -b qthelp $(ALLSPHINXOPTS) build/qthelp
	@echo
	@echo "Build finished; now you can run "qcollectiongenerator" with the" \
	      ".qhcp project file in build/qthelp, like this:"
	@echo "# qcollectiongenerator build/qthelp/foo.qhcp"
	@echo "To view the help file:"
	@echo "# assistant -collectionFile build/qthelp/foo.qhc"

qhc: qthelp
	$(QCOLLECTIONGENERATOR) build/qthelp/TortoiseHg.qhcp
	@echo "Build finished. To view the help file:"
	@echo "# assistant -collectionFile build/qthelp/TortoiseHg.qhc"

latex:
	mkdir -p build/latex build/doctrees
	$(SPHINXBUILD) -b latex $(ALLSPHINXOPTS) build/latex
	@echo
	@echo "Build finished; the LaTeX files are in build/latex."
	@echo "Run \`make all-pdf' or \`make all-ps' in that directory to" \
	      "run these through (pdf)latex."

man:
	mkdir -p build/man build/doctrees
	$(SPHINXBUILD) -b man $(ALLSPHINXOPTS) build/man
	@echo
	@echo "Build finished; the manpages are in build/man."
	@echo "Run \`man -l build/man/x265.1' or \`man -l build/man/libx265.3'" \
	      "to view them."

changes:
	mkdir -p build/changes build/doctrees
	$(SPHINXBUILD) -b changes $(ALLSPHINXOPTS) build/changes
	@echo
	@echo "The overview file is in build/changes."

linkcheck:
	mkdir -p build/linkcheck build/doctrees
	$(SPHINXBUILD) -b linkcheck $(ALLSPHINXOPTS) build/linkcheck
	@echo
	@echo "Link check complete; look for any errors in the above output " \
	      "or in build/linkcheck/output.txt."
