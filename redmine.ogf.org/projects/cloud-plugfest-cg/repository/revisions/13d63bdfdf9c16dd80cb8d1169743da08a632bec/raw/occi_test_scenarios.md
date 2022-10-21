# OCCI Test Scenarios

In the examples, the OCCI endpoint will be substituted by the variable `$ENDPOINT`. If an OCCI endpoint is available through an HTTP server with DNS entry *localhost*, port *3000* and application root *occi* then the variable should be set to `ENDPOINT=http://localhost:3000/occi`.

## General Information from an OCCI endpoint

An OCCI Server SHOULD identify itself and the supported OCCI version using the *HTTP Server header*. An OCCI Server MUST return the supported OCCI formats using media-types in the *HTTP Accept header*. A server MAY provide priorities for the media types as specified in the HTTP RFC.

### Example

    curl -v -X HEAD $ENDPOINT
    < HTTP/1.1 200 OK
    < Status: 200
    < Accept: application/occi+json,text/uri-list,text/plain;q=0.5,text/occi;q=0.2
    < Server: SERVERNAME/SERVERVERSION OCCI/1.1
    < Content-Length: 0    

## Discovery Interface

### Discovering all kinds, mixins and actions

Each OCCI endpoint must allow discovering all available categories, e.g. kinds, mixins and actions through the Discovery Interface.

#### Request

Send a GET to the OCCI endpoint OPTIONAL with one of the allowed accept types *text/occi*, *text/plain*, *application/occi+json*.

##### No Accept type

    curl -v -X GET $ENDPOINT/-/        
    > GET /-/ HTTP/1.1
    > Host: localhost:3000
    > Accept: */*

##### text/occi format
            
    curl -v -X GET -H 'Accept: text/occi' $ENDPOINT/-/
    > GET /-/ HTTP/1.1
	> Host: localhost:3000
	> Accept: text/occi
    
##### text/plain format

    curl -v -X GET -H 'Accept: text/plain' $ENDPOINT/-/
    > GET /-/ HTTP/1.1
	> Host: localhost:3000
	> Accept: text/plain
    
##### application/occi+json format

    curl -v -X GET -H 'Accept: application/occi+json' $ENDPOINT/-/
    > GET /-/ HTTP/1.1
	> User-Agent: curl/7.27.0
	> Host: localhost:3000
	> Accept: application/json
	
#### Response

The Status of the response MUST be 200.

##### No Accept type

The response by the server depends on the default media type of the server and MUST be identified via the *HTTP Content-Type header*. The following sections describe the responses for each media-type.

##### text/occi format

All categories MUST be rendered in the Category HTTP header. 

    < HTTP/1.1 200 OK
    < Content-Type: text/occi;charset=utf-8
    < Connection: keep-alive
    < Status: 200
    < Accept: application/occi+json,text/uri-list,text/plain;q=0.5,text/occi;q=0.2
    < Server: SERVERNAME/SERVERVERSION OCCI/1.1
    < Category: entity;scheme="http://schemas.ogf.org/occi/core#";class="kind";title="Entity";location="/entity/";attributes="occi.core.id occi.core.title",resource;scheme="http://schemas.ogf.org/occi/core#";class="kind";title="Resource";rel="http://schemas.ogf.org/occi/core#entity";location="/resource/";attributes="occi.core.summary",link;scheme="http://schemas.ogf.org/occi/core#";class="kind";title="Link";rel="http://schemas.ogf.org/occi/core#entity";location="/link/";attributes="occi.core.target occi.core.source",compute;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind";title="Compute Resource";rel="http://schemas.ogf.org/occi/core#resource";location="/compute/";attributes="occi.compute.architecture occi.compute.cores occi.compute.hostname occi.compute.speed occi.compute.memory occi.compute.state";actions="http://schemas.ogf.org/occi/infrastructure/compute/action#start http://schemas.ogf.org/occi/infrastructure/compute/action#stop http://schemas.ogf.org/occi/infrastructure/compute/action#restart http://schemas.ogf.org/occi/infrastructure/compute/action#suspend",network;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind";title="Network Resource";rel="http://schemas.ogf.org/occi/core#resource";location="/network/";attributes="occi.network.vlan occi.network.label occi.network.state";actions="http://schemas.ogf.org/occi/infrastructure/network/action#up http://schemas.ogf.org/occi/infrastructure/network/action#down",networkinterface;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind";title="Networkinterface";rel="http://schemas.ogf.org/occi/core#link";location="/networkinterface/";attributes="occi.networkinterface.interface occi.networkinterface.mac occi.networkinterface.state",storage;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind";title="Storage Resource";rel="http://schemas.ogf.org/occi/core#resource";location="/storage/";attributes="occi.storage.size occi.storage.state";actions="http://schemas.ogf.org/occi/infrastructure/storage/action#online http://schemas.ogf.org/occi/infrastructure/storage/action#offline http://schemas.ogf.org/occi/infrastructure/storage/action#backup http://schemas.ogf.org/occi/infrastructure/storage/action#snapshot http://schemas.ogf.org/occi/infrastructure/storage/action#resize",storagelink;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind";title="Storage Link";rel="http://schemas.ogf.org/occi/core#link";location="/storagelink/";attributes="occi.storagelink.deviceid occi.storagelink.mountpoint occi.storagelink.state",ipnetwork;scheme="http://schemas.ogf.org/occi/infrastructure/network#";class="mixin";title="IP Network Mixin";location="/mixins/ipnetwork/";attributes="occi.network.address occi.network.gateway occi.network.allocation occi.network.state",ipnetworkinterface;scheme="http://schemas.ogf.org/occi/infrastructure/networkinterface#";class="mixin";title="IP Networkinterface Mixin";location="/mixins/ipnetworkinterface/";attributes="occi.networkinterface.address occi.networkinterface.gateway occi.networkinterface.allocation occi.networkinterface.state",os_tpl;scheme="http://schemas.ogf.org/occi/infrastructure#";class="mixin";title="Operating System Template";location="/mixins/os_tpl/",resource_tpl;scheme="http://schemas.ogf.org/occi/infrastructure#";class="mixin";title="Resource Template";location="/mixins/resource_tpl/",start;scheme="http://schemas.ogf.org/occi/infrastructure/compute/action#";class="action";title="Start Compute instance",stop;scheme="http://schemas.ogf.org/occi/infrastructure/compute/action#";class="action";title="Stop Compute instance";attributes="method",restart;scheme="http://schemas.ogf.org/occi/infrastructure/compute/action#";class="action";title="Restart Compute instance";attributes="method",suspend;scheme="http://schemas.ogf.org/occi/infrastructure/compute/action#";class="action";title="Suspend Compute instance";attributes="method",up;scheme="http://schemas.ogf.org/occi/infrastructure/network/action#";class="action";title="Activate network",down;scheme="http://schemas.ogf.org/occi/infrastructure/network/action#up";class="action";title="Activate network",online;scheme="http://schemas.ogf.org/occi/infrastructure/storage/action#";class="action";title="Activate Storage",offline;scheme="http://schemas.ogf.org/occi/infrastructure/storage/action#";class="action";title="Deactivate Storage",backup;scheme="http://schemas.ogf.org/occi/infrastructure/storage/action#";class="action";title="Backup Storage",snapshot;scheme="http://schemas.ogf.org/occi/infrastructure/storage/action#";class="action";title="Snapshot Storage",resize;scheme="http://schemas.ogf.org/occi/infrastructure/storage/action#";class="action";title="Resize Storage";attributes="size"
    < Content-Length: 2
    OK

##### text/plain format

All categories must be rendered in the HTTP body and be prefixed with `Category:`.

	< HTTP/1.1 200 OK
	< Content-Type: text/plain;charset=utf-8
	< Status: 200
    < Accept: application/occi+json,text/uri-list,text/plain;q=0.5,text/occi;q=0.2
    < Server: SERVERNAME/SERVERVERSION OCCI/1.1
	< Content-Length: 4965
	< 
	Category: entity;scheme="http://schemas.ogf.org/occi/core#";class="kind";title="Entity";location="/entity/";attributes="occi.core.id occi.core.title"
	Category: resource;scheme="http://schemas.ogf.org/occi/core#";class="kind";title="Resource";rel="http://schemas.ogf.org/occi/core#entity";location="/resource/";attributes="occi.core.summary"
	Category: link;scheme="http://schemas.ogf.org/occi/core#";class="kind";title="Link";rel="http://schemas.ogf.org/occi/core#entity";location="/link/";attributes="occi.core.target occi.core.source"
	Category: compute;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind";title="Compute Resource";rel="http://schemas.ogf.org/occi/core#resource";location="/compute/";attributes="occi.compute.architecture occi.compute.cores occi.compute.hostname occi.compute.speed occi.compute.memory occi.compute.state";actions="http://schemas.ogf.org/occi/infrastructure/compute/action#start http://schemas.ogf.org/occi/infrastructure/compute/action#stop http://schemas.ogf.org/occi/infrastructure/compute/action#restart http://schemas.ogf.org/occi/infrastructure/compute/action#suspend"
	Category: network;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind";title="Network Resource";rel="http://schemas.ogf.org/occi/core#resource";location="/network/";attributes="occi.network.vlan occi.network.label occi.network.state";actions="http://schemas.ogf.org/occi/infrastructure/network/action#up http://schemas.ogf.org/occi/infrastructure/network/action#down"
	Category: networkinterface;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind";title="Networkinterface";rel="http://schemas.ogf.org/occi/core#link";location="/networkinterface/";attributes="occi.networkinterface.interface occi.networkinterface.mac occi.networkinterface.state"
	Category: storage;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind";title="Storage Resource";rel="http://schemas.ogf.org/occi/core#resource";location="/storage/";attributes="occi.storage.size occi.storage.state";actions="http://schemas.ogf.org/occi/infrastructure/storage/action#online http://schemas.ogf.org/occi/infrastructure/storage/action#offline http://schemas.ogf.org/occi/infrastructure/storage/action#backup http://schemas.ogf.org/occi/infrastructure/storage/action#snapshot http://schemas.ogf.org/occi/infrastructure/storage/action#resize"
	Category: storagelink;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind";title="Storage Link";rel="http://schemas.ogf.org/occi/core#link";location="/storagelink/";attributes="occi.storagelink.deviceid occi.storagelink.mountpoint occi.storagelink.state"
	Category: ipnetwork;scheme="http://schemas.ogf.org/occi/infrastructure/network#";class="mixin";title="IP Network Mixin";location="/mixins/ipnetwork/";attributes="occi.network.address occi.network.gateway occi.network.allocation occi.network.state"
	Category: ipnetworkinterface;scheme="http://schemas.ogf.org/occi/infrastructure/networkinterface#";class="mixin";title="IP Networkinterface Mixin";location="/mixins/ipnetworkinterface/";attributes="occi.networkinterface.address occi.networkinterface.gateway occi.networkinterface.allocation occi.networkinterface.state"
	Category: os_tpl;scheme="http://schemas.ogf.org/occi/infrastructure#";class="mixin";title="Operating System Template";location="/mixins/os_tpl/"
	Category: resource_tpl;scheme="http://schemas.ogf.org/occi/infrastructure#";class="mixin";title="Resource Template";location="/mixins/resource_tpl/"
	Category: start;scheme="http://schemas.ogf.org/occi/infrastructure/compute/action#";class="action";title="Start Compute instance"
	Category: stop;scheme="http://schemas.ogf.org/occi/infrastructure/compute/action#";class="action";title="Stop Compute instance";attributes="method"
	Category: restart;scheme="http://schemas.ogf.org/occi/infrastructure/compute/action#";class="action";title="Restart Compute instance";attributes="method"
	Category: suspend;scheme="http://schemas.ogf.org/occi/infrastructure/compute/action#";class="action";title="Suspend Compute instance";attributes="method"
	Category: up;scheme="http://schemas.ogf.org/occi/infrastructure/network/action#";class="action";title="Activate network"
	Category: down;scheme="http://schemas.ogf.org/occi/infrastructure/network/action#up";class="action";title="Activate network"
	Category: online;scheme="http://schemas.ogf.org/occi/infrastructure/storage/action#";class="action";title="Activate Storage"
	Category: offline;scheme="http://schemas.ogf.org/occi/infrastructure/storage/action#";class="action";title="Deactivate Storage"
	Category: backup;scheme="http://schemas.ogf.org/occi/infrastructure/storage/action#";class="action";title="Backup Storage"
	Category: snapshot;scheme="http://schemas.ogf.org/occi/infrastructure/storage/action#";class="action";title="Snapshot Storage"
	Category: resize;scheme="http://schemas.ogf.org/occi/infrastructure/storage/action#";class="action";title="Resize Storage";attributes="size"

### application/json format

## Create

### Create an OCCI compute resource

### Create an OCCI storage resource

### Create an OCCI network resource

### Create an OCCI compute resource with a storage link and a networkinterface

### Create an OCCI compute resource using an OS and resource template

## Read

### List all entities

### List all entities of a specific kind

### Describe all entities

### Describe all entities of a specific kind

### Describe specific entity

## Update

## Delete

### Delete all entities

### Delete all entities of a specific kind

### Delete a specific entity

## Interoperability Scenarios

### OVF

### CDMI

### AMQP