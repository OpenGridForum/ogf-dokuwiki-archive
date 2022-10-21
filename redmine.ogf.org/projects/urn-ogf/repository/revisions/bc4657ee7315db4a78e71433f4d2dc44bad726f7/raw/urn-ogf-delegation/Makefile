all: rfc6453.txt rfc6453.html

draft-dijkstra-urn-ogf.txt: draft-dijkstra-urn-ogf.xml
	xml2rfc draft-dijkstra-urn-ogf

draft-dijkstra-urn-ogf.html: draft-dijkstra-urn-ogf.xml
	xsltproc draft-dijkstra-urn-ogf.xml > draft-dijkstra-urn-ogf.html

rfc6453.txt: rfc6453.xml
	xml2rfc rfc6453

rfc6453.html: rfc6453.xml
	xsltproc rfc6453.xml > rfc6453.html

