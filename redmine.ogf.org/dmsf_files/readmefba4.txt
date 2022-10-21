
Proposal (glue2_2.xsd) includes: 
- Flat XSD style rendering (preferred over a parent/child nesting approach, see OGF 35 mins)
- Introduction of abstract elements and substitution groups
- A global <Entities> Document Root element 
- All core entities are also made global so that they can be imported for re-use in other XSDs 
==================================================================================

*Please note, only the modified glue2_2.xsd forms the proposal (with the 
glue2_2SampleDoc2.xml sample instance doc). 
The glue2_2Extension.xsd schema is merely an example showing how you can 
subsequently extend substitutable/abstract elements with the use of 
xsd:substitution groups if needed for future profiling of new types. 
Importantly, the modified glue2_2.xsd still defines its own (concrete) set of 
standard glue2 Service/Endpoint/Domain/Manager/Resource/Share/Activity/Policy 
elements that form the core/standard set.

The abstract elements provide extension points that allow different concrete element
specialisations to be nested within <Entities> in future. This follows the 
glue2 specification more closely which states that "Grid services requiring a 
richer set of attributes for the [Endpoint|Service|EntityX], specific models MAY be 
derived by specializing from the [Endpoint|Service|EntityX] class and adding new 
properties or relationships." 
 
Third party XSD schemas that import the (modified) glue2_2.xsd can also 
extend the abstract element types in order to define their own sub-type 
specialisations (these sub-types inherit elements and attributes from their parents).
The xsd:substitutionGroupS mean that those sub-type specialisations can 
be validated as part of a glue2 <Entities> element bag. 

As an example, the sampleGlue2_2Extension.xsd schema file imports the (modified) glue2_2.xsd 
schema and defines its own <MonitoredXService> and <MonitoredXEndpoint> sub-types 
The sampleGlue2_ExtensionDoc.xml instance doc is then valid only when validating
against both XSDs (these are intended as examples only). 



XSD Validation
===============
To validate the sample xml docs, both xsd schemas and the xml sample docs 
need to be in the same directory.  


Some useful comments/questions and answers:
============================================
> what's exactly wrong with the current schema when it comes to extend it?
> can you elaborate a bit more?

In the GLUE2 specification doc it states "For Grid services requiring a
richer set of attributes for the Endpoint, specific models MAY be derived by
specializing from the Endpoint class and adding new properties or
relationships".
However, in current (unmodified) glue.xsd, the Service_t defines an
'Endpoint' element that can't be extended with a custom Endpoint
specialisation because Endpoint must be of type 'glue:Endoint_t'  (<element
name="Endpoint" type="glue:Endpoint_t" minOccurs="0" maxOccurs="unbounded"/>
).  The problem is that 'Endpoint_t' is not abstract and so can't be
substituted with an Endpoint specialisation using an xsd:substitution group.
The same problem occurs for the Service. Therefore, I am proposing that the
Endpoint and Service elements be defined as abstract. The standard glue2
services (Service, ComputingService and StorageService) then provide a core
set of concrete elements (as they are defined within the glue2_2.xsd).



> What I am afraid of is the crazy creation of thousands of custom 
> endpoints that in my opinion will totally make useless the main 
> purpose of GLUE2, which is, always in my opinion, to avoid having 
> product-specific renderings...

 Nope, this is not an issue because the modified glue2_2.xsd defines its own
 standard set of glue2 Service and Endpoint implementations. These are
 defined within the glue2_2.xsd schema itself. Therefore, for a base/standard
 glue2 profile, only these standard elements are considered when validating,


 However, if a new service or endpoint specialisation needs to be defined in
 the future, the relevant working group can simply define a new (service
 and/or endpoint) element in a new extending schema and specify the
 appropriate substitution group (as per the sampleGlue2_2Extension.xsd example
 - is an example only). This extension schema can then be profiled within OGF
 for example. Importantly, by using the xsd:substitution groups, any new
 Service and Endpoint elements can be validated as part of an
 AdminDomain (e.g. this registry supports the glue2_2.xsd base profile +
 'ServiceX1' from the 'X1' extension profile document). In doing this, no
 changes are required in the base glue2_2.xsd.




