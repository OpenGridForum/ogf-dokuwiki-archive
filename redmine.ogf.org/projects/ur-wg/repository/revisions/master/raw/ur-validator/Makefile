
ALL_SAMPLES := $(wildcard *.xml) $(wildcard *.xml.gz)
ALL_VALIDATED = $(ALL_SAMPLES:%=%.ok)
VERSION = $(shell cat VERSION)
XSD_FORM = ur-2.0.xsd
release_filename = /tmp/starval-$(VERSION).tar

help:
	@echo "UR-2.0 Validation test suite"
	@echo ""
	@echo "Targets:"
	@echo "   test      --  run samples against schema"
	@echo "   testclean --  remove generated files"
	@echo "   clean     --  remove generated files and old backups"
	@echo "   release   --  create a tar.gz file from current repository"

test: $(ALL_VALIDATED)

%.ok : % $(XSD_FORM)
	@xmllint --noout --schema $(XSD_FORM) $< && touch $@

testclean:
	rm -f $(ALL_VALIDATED)

distclean: clean

clean: testclean
	rm -f *~ *.bak

release: $(release_filename:%=%.gz)
	@echo ""
	@echo " Release $(VERSION) available as $<"
	@echo

$(release_filename:%=%.gz) : $(release_filename)
	gzip $<


$(release_filename): distclean
	tar -cf $@ --transform="s,\(.*\),starval-$(VERSION)/\1," *

