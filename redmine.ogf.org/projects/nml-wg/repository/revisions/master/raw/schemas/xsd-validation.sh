#!/bin/sh


###########################################################################################

## An example of XML file that could be used as the command-line parameter

# <?xml version="1.0" encoding="UTF-8"?>
# <nml:Topology xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
#              xsi:schemaLocation="http://schemas.ogf.org/nml/2013/05/base# nmlbase.xsd"
#              xmlns:nml="http://schemas.ogf.org/nml/2013/05/base#"
#              xmlns:nmlExperimental="http://schemas.ogf.org/nml/2013/05/experimantal#"
#              id="urn:ogf:network:example.net:2013:org"
#              version="20120814T212121Z">
#
#  <nml:Location id="urn:ogf:network:example.net:2013:redcity">
#    <nml:name>Red City</nml:name>
#    <nml:lat>30.600</nml:lat>
#    <nml:long>12.640</nml:long>
#  </nml:Location>

#  <nml:Link id="urn:ogf:network:example.net:2013:LinkA" encoding="XYZ">
#  </nml:Link>

#  <nml:Node id="urn:ogf:network:example.net:2013:nodeA">
#    <nml:name>Node_A</nml:name>

#    <nml:Location id="urn:ogf:network:example.net:2013:redcity"/>

#    <nml:Relation type="http://schemas.ogf.org/nml/2013/05/base#hasOutboundPort">
#      <nml:Port id="urn:ogf:network:example.net:2013:nodeA:port_X:out"/>
#      <nml:Port id="urn:ogf:network:example.net:2013:nodeA:port_Y:out"/>
#    </nml:Relation>
#    <nml:Relation type="http://schemas.ogf.org/nml/2013/05/base#hasInboundPort">
#      <nml:Port id="urn:ogf:network:example.net:2013:nodeA:port_X:in"/>
#      <nml:Port id="urn:ogf:network:example.net:2013:nodeA:port_Y:in"/>
#    </nml:Relation>

#  </nml:Node>

#  <nml:Topology id="urn:ogf:network:example.net:2013:org:org_sub"/>

#  <nml:BidirectionalPort id="urn:ogf:network:example.net:2013:nodeA:port_X">
#    <nml:Port id="urn:ogf:network:example.net:2013:nodeA:port_X:out"/>
#    <nml:Port id="urn:ogf:network:example.net:2013:nodeA:port_X:in"/>
#  </nml:BidirectionalPort>

#  <nmlExperimental:Relation type="http://schemas.ogf.org/nml/2013/05/base#isAlias">
#    <nmlExperimental:Topology id="urn:ogf:network:example.net:2013:org:nml-test"/>
#  </nmlExperimental:Relation>

#  <nml:Relation type="http://schemas.ogf.org/nml/2013/05/base#isAlias">
#    <nml:Topology id="urn:ogf:network:example.net:2013:org:nml-test"/>
#  </nml:Relation>

# </nml:Topology>

###########################################################################################


StdInParse -v=auto -n -s -f < $1




