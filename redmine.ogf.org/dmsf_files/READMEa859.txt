
Please note: 

- glue2_2.xsd is the XSD schema
- glue2_2SampleDoc1.xml is a sample doc that is valid against 'glue2_2.xsd' 
- glue2_2SampleManyEndpoints.xml is a sample doc that is valid against 'glue2_2.xsd' 

- 'sampleGlue2_2Extension.xsd' imports the GLUE2 XSD (glue2_2.xsd) 
  in order to derive custom sub-types that are NOT part of the GLUE2 spec. 
- 'sampleGlue2_2ExtensionDoc.xml' needs to be validated against both 
  'glue2_2.xsd' and 'sampleGlue2_2Extension.xsd' and it is NOT GLUE2 compliant! 
  (it merely demonstrates how custom sub-types can be derived and nested within 
  Entities without having to modify the base glue2_2.xsd schema - see Doc for details).

- To validate the xml docs, the XSDs need to be in the same folder (unless you 
 modify the xsd:schemaLocation attribute). See doc for details.   