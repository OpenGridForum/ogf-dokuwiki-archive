# OCCI Test Scenarios

In the examples, the OCCI endpoint will be substituted by the variable `$ENDPOINT`. If an OCCI endpoint is available through an HTTP server with DNS entry *localhost*, port *3000* and application root *occi* then the variable should be set to `ENDPOINT=http://localhost:3000/occi`.

## Tools

To analyse the HTTP messages sent between cleint and server, the use of [ngrep](http://ngrep.sourceforge.net/) is recommended. 

For example on Mac OS X, to listen on the loopback interface (*lo0*) on port 3000 and write the output in pcap format to the file test.pcap the command should be execute with the following options:

    ngrep -O test.pcap -T -d lo0 -W byline port 3000

## General Information from an OCCI endpoint

An OCCI Server SHOULD identify itself and the supported OCCI version using the *HTTP Server header*. An OCCI Server MUST return the supported OCCI formats using media-types in the *HTTP Accept header*. A server MAY provide priorities for the media types as specified in the HTTP RFC.

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

Send a GET to the location `/-/` of the OCCI endpoint. It MAY contain one of the allowed accept types *text/occi*, *text/plain*, *application/occi+json*.

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

### application/json forma

    {
        "kinds": [
            {
                "scheme": "http://schemas.ogf.org/occi/core#",
                "term": "entity",
                "title": "Entity",
                "attributes": {
                    "occi": {
                        "core": {
                            "id": {
                                "type": "string",
                                "required": false,
                                "mutable": false,
                                "pattern": "[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}"
                            },
                            "title": {
                                "type": "string",
                                "required": false,
                                "mutable": true,
                                "pattern": ".*"
                            }
                        }
                    }
                }
            },
            {
                "related": [
                    "http://schemas.ogf.org/occi/core#entity"
                ],
                "scheme": "http://schemas.ogf.org/occi/core#",
                "term": "resource",
                "title": "Resource",
                "attributes": {
                    "occi": {
                        "core": {
                            "summary": {
                                "type": "string",
                                "required": false,
                                "mutable": true,
                                "pattern": ".*"
                            }
                        }
                    }
                }
            },
            {
                "related": [
                    "http://schemas.ogf.org/occi/core#entity"
                ],
                "scheme": "http://schemas.ogf.org/occi/core#",
                "term": "link",
                "title": "Link",
                "attributes": {
                    "occi": {
                        "core": {
                            "target": {
                                "type": "string",
                                "required": false,
                                "mutable": true,
                                "pattern": ".*"
                            },
                            "source": {
                                "type": "string",
                                "required": false,
                                "mutable": true,
                                "pattern": ".*"
                            }
                        }
                    }
                }
            },
            {
                "related": [
                    "http://schemas.ogf.org/occi/core#resource"
                ],
                "actions": [
                    "http://schemas.ogf.org/occi/infrastructure/compute/action#start",
                    "http://schemas.ogf.org/occi/infrastructure/compute/action#stop",
                    "http://schemas.ogf.org/occi/infrastructure/compute/action#restart",
                    "http://schemas.ogf.org/occi/infrastructure/compute/action#suspend"
                ],
                "scheme": "http://schemas.ogf.org/occi/infrastructure#",
                "term": "compute",
                "title": "Compute Resource",
                "attributes": {
                    "occi": {
                        "compute": {
                            "architecture": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": "x86|x64"
                            },
                            "cores": {
                                "mutable": true,
                                "required": false,
                                "type": "number",
                                "pattern": ".*"
                            },
                            "hostname": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": "(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*"
                            },
                            "speed": {
                                "mutable": true,
                                "required": false,
                                "type": "number",
                                "pattern": ".*"
                            },
                            "memory": {
                                "mutable": true,
                                "required": false,
                                "type": "number",
                                "pattern": ".*"
                            },
                            "state": {
                                "mutable": false,
                                "required": false,
                                "type": "string",
                                "pattern": "inactive|active|suspended|error",
                                "Default": "inactive"
                            }
                        }
                    }
                }
            },
            {
                "related": [
                    "http://schemas.ogf.org/occi/core#resource"
                ],
                "actions": [
                    "http://schemas.ogf.org/occi/infrastructure/network/action#up",
                    "http://schemas.ogf.org/occi/infrastructure/network/action#down"
                ],
                "scheme": "http://schemas.ogf.org/occi/infrastructure#",
                "term": "network",
                "title": "Network Resource",
                "attributes": {
                    "occi": {
                        "network": {
                            "vlan": {
                                "mutable": true,
                                "required": false,
                                "type": "number",
                                "Minimum": 0,
                                "Maximum": 4095,
                                "pattern": ".*"
                            },
                            "label": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": ".*"
                            },
                            "state": {
                                "mutable": false,
                                "required": false,
                                "type": "string",
                                "pattern": "active|inactive|error",
                                "Default": "inactive"
                            }
                        }
                    }
                }
            },
            {
                "related": [
                    "http://schemas.ogf.org/occi/core#link"
                ],
                "scheme": "http://schemas.ogf.org/occi/infrastructure#",
                "term": "networkinterface",
                "title": "Networkinterface",
                "attributes": {
                    "occi": {
                        "networkinterface": {
                            "interface": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": ".*"
                            },
                            "mac": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": "^([0-9a-fA-F]{2}[:-]){5}([0-9a-fA-F]{2})$"
                            },
                            "state": {
                                "mutable": false,
                                "required": false,
                                "type": "string",
                                "pattern": "active|inactive",
                                "Default": "inactive"
                            }
                        }
                    }
                }
            },
            {
                "related": [
                    "http://schemas.ogf.org/occi/core#resource"
                ],
                "actions": [
                    "http://schemas.ogf.org/occi/infrastructure/storage/action#online",
                    "http://schemas.ogf.org/occi/infrastructure/storage/action#offline",
                    "http://schemas.ogf.org/occi/infrastructure/storage/action#backup",
                    "http://schemas.ogf.org/occi/infrastructure/storage/action#snapshot",
                    "http://schemas.ogf.org/occi/infrastructure/storage/action#resize"
                ],
                "scheme": "http://schemas.ogf.org/occi/infrastructure#",
                "term": "storage",
                "title": "Storage Resource",
                "attributes": {
                    "occi": {
                        "storage": {
                            "size": {
                                "mutable": true,
                                "required": false,
                                "type": "number",
                                "pattern": ".*"
                            },
                            "state": {
                                "mutable": false,
                                "required": false,
                                "type": "string",
                                "pattern": "online|offline|backup|snapshot|resize|degraded",
                                "Default": "offline"
                            }
                        }
                    }
                }
            },
            {
                "related": [
                    "http://schemas.ogf.org/occi/core#link"
                ],
                "scheme": "http://schemas.ogf.org/occi/infrastructure#",
                "term": "storagelink",
                "title": "Storage Link",
                "attributes": {
                    "occi": {
                        "storagelink": {
                            "deviceid": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": ".*"
                            },
                            "mountpoint": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": ".*"
                            },
                            "state": {
                                "mutable": false,
                                "required": false,
                                "type": "string",
                                "pattern": "active|inactive",
                                "Default": "inactive"
                            }
                        }
                    }
                }
            }
        ],
        "mixins": [
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure/network#",
                "term": "ipnetwork",
                "title": "IP Network Mixin",
                "attributes": {
                    "occi": {
                        "network": {
                            "address": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": "(^\\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:)))(%.+)?\\s*(\\/(\\d|\\d\\d|1[0-1]\\d|12[0-8]))$)|(^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])(\\/(\\d|[1-2]\\d|3[0-2]))$)"
                            },
                            "gateway": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": "(^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$)|(^\\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:)))(%.+)?\\s*)"
                            },
                            "allocation": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": "dynamic|static"
                            },
                            "state": {
                                "mutable": false,
                                "required": false,
                                "type": "string",
                                "pattern": "active|inactive",
                                "Default": "inactive"
                            }
                        }
                    }
                }
            },
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure/networkinterface#",
                "term": "ipnetworkinterface",
                "title": "IP Networkinterface Mixin",
                "attributes": {
                    "occi": {
                        "networkinterface": {
                            "address": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": "(^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$)|(^\\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:)))(%.+)?\\s*)"
                            },
                            "gateway": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": "(^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$)|(^\\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:)))(%.+)?\\s*)"
                            },
                            "allocation": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": "dynamic|static"
                            },
                            "state": {
                                "mutable": false,
                                "required": false,
                                "type": "string",
                                "pattern": "active|inactive",
                                "Default": "inactive"
                            }
                        }
                    }
                }
            },
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure#",
                "term": "os_tpl",
                "title": "Operating System Template"
            },
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure#",
                "term": "resource_tpl",
                "title": "Resource Template"
            },
            {
                "related": [
                    "http://schemas.ogf.org/occi/infrastructure#resource_tpl"
                ],
                "scheme": "http://my.occi.service//occi/infrastructure/resource_tpl#",
                "term": "extra_large",
                "title": "Extra Large Instance",
                "attributes": {
                    "occi": {
                        "compute": {
                            "architecture": {
                                "mutable": false,
                                "type": "string",
                                "Default": "x64",
                                "required": false,
                                "pattern": ".*"
                            },
                            "cores": {
                                "mutable": false,
                                "type": "number",
                                "Default": 4,
                                "required": false,
                                "pattern": ".*"
                            },
                            "speed": {
                                "mutable": false,
                                "type": "number",
                                "Default": 2,
                                "required": false,
                                "pattern": ".*"
                            },
                            "memory": {
                                "mutable": false,
                                "type": "number",
                                "Default": 15,
                                "required": false,
                                "pattern": ".*"
                            }
                        }
                    }
                }
            },
            {
                "related": [
                    "http://schemas.ogf.org/occi/infrastructure#resource_tpl"
                ],
                "scheme": "http://my.occi.service//occi/infrastructure/resource_tpl#",
                "term": "large",
                "title": "Large Instance",
                "attributes": {
                    "occi": {
                        "compute": {
                            "architecture": {
                                "mutable": false,
                                "type": "string",
                                "Default": "x64",
                                "required": false,
                                "pattern": ".*"
                            },
                            "cores": {
                                "mutable": false,
                                "type": "number",
                                "Default": 2,
                                "required": false,
                                "pattern": ".*"
                            },
                            "speed": {
                                "mutable": false,
                                "type": "number",
                                "Default": 2,
                                "required": false,
                                "pattern": ".*"
                            },
                            "memory": {
                                "mutable": false,
                                "type": "number",
                                "Default": 7.5,
                                "required": false,
                                "pattern": ".*"
                            }
                        }
                    }
                }
            },
            {
                "related": [
                    "http://schemas.ogf.org/occi/infrastructure#resource_tpl"
                ],
                "scheme": "http://my.occi.service//occi/infrastructure/resource_tpl#",
                "term": "medium",
                "title": "Medium Instance",
                "attributes": {
                    "occi": {
                        "compute": {
                            "architecture": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": "x86|x64",
                                "Default": "x86"
                            },
                            "cores": {
                                "mutable": false,
                                "type": "number",
                                "Default": 2,
                                "required": false,
                                "pattern": ".*"
                            },
                            "speed": {
                                "mutable": false,
                                "type": "number",
                                "Default": 1,
                                "required": false,
                                "pattern": ".*"
                            },
                            "memory": {
                                "mutable": false,
                                "type": "number",
                                "Default": 3.75,
                                "required": false,
                                "pattern": ".*"
                            }
                        }
                    }
                }
            },
            {
                "related": [
                    "http://schemas.ogf.org/occi/infrastructure#os_tpl"
                ],
                "scheme": "http://my.occi.service//occi/infrastructure/os_tpl#",
                "term": "my_os",
                "title": "My OS Template"
            },
            {
                "related": [
                    "http://schemas.ogf.org/occi/infrastructure#resource_tpl"
                ],
                "scheme": "http://my.occi.service//occi/infrastructure/resource_tpl#",
                "term": "small",
                "title": "Small Instance",
                "attributes": {
                    "occi": {
                        "compute": {
                            "architecture": {
                                "mutable": true,
                                "required": false,
                                "type": "string",
                                "pattern": "x86|x64",
                                "Default": "x86"
                            },
                            "cores": {
                                "mutable": false,
                                "type": "number",
                                "Default": 1,
                                "required": false,
                                "pattern": ".*"
                            },
                            "speed": {
                                "mutable": false,
                                "type": "number",
                                "Default": 1,
                                "required": false,
                                "pattern": ".*"
                            },
                            "memory": {
                                "mutable": false,
                                "type": "number",
                                "Default": 1.7,
                                "required": false,
                                "pattern": ".*"
                            }
                        }
                    }
                }
            }
        ],
        "actions": [
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure/compute/action#",
                "term": "start",
                "title": "Start Compute instance"
            },
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure/compute/action#",
                "term": "stop",
                "title": "Stop Compute instance",
                "attributes": {
                    "method": {
                        "mutable": true,
                        "required": false,
                        "type": "string",
                        "pattern": "graceful|acpioff|poweroff",
                        "Default": "poweroff"
                    }
                }
            },
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure/compute/action#",
                "term": "restart",
                "title": "Restart Compute instance",
                "attributes": {
                    "method": {
                        "mutable": true,
                        "required": false,
                        "type": "string",
                        "pattern": "graceful|warm|cold",
                        "Default": "cold"
                    }
                }
            },
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure/compute/action#",
                "term": "suspend",
                "title": "Suspend Compute instance",
                "attributes": {
                    "method": {
                        "mutable": true,
                        "required": false,
                        "type": "string",
                        "pattern": "hibernate|suspend",
                        "Default": "suspend"
                    }
                }
            },
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure/network/action#",
                "term": "up",
                "title": "Activate network"
            },
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure/network/action#up",
                "term": "down",
                "title": "Activate network"
            },
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure/storage/action#",
                "term": "online",
                "title": "Activate Storage"
            },
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure/storage/action#",
                "term": "offline",
                "title": "Deactivate Storage"
            },
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure/storage/action#",
                "term": "backup",
                "title": "Backup Storage"
            },
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure/storage/action#",
                "term": "snapshot",
                "title": "Snapshot Storage"
            },
            {
                "scheme": "http://schemas.ogf.org/occi/infrastructure/storage/action#",
                "term": "resize",
                "title": "Resize Storage",
                "attributes": {
                    "size": {
                        "mutable": true,
                        "required": false,
                        "type": "number",
                        "pattern": ".*"
                    }
                }
            }
        ]
    }

## Create

### Create an OCCI compute resource

#### Request

Send a POST to the location of the compute kind e.g. `/compute/` of the OCCI endpoint. The request MUST have the *HTTP Content Type header* set to the correct media type of the enclosed OCCI message. It MAY contain one of the allowed accept types *text/occi*, *text/plain*, *application/occi+json*.

##### Content type text/occi with no Accept type

    curl -v -X POST \
        -H 'Content-Type: text/occi' \
        -H 'Category: compute;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind"' \
        -H 'X-OCCI-Attribute: occi.core.id="02581c73-7ddd-4c30-bfcd-8cb8259976bd",occi.core.title="title",occi.core.summary="summary",occi.compute.architecture="x86",occi.compute.cores=1,occi.compute.hostname="hostname",occi.compute.speed=1.0,occi.compute.memory=1.0,occi.compute.state="inactive"'\
        $ENDPOINT/compute/        
    > POST /compute/ HTTP/1.1
    > Host: localhost:3000
    > Accept: */*
    > Content-Type: text/occi
    > Category: compute;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind"
    > X-OCCI-Attribute: occi.core.id="02581c73-7ddd-4c30-bfcd-8cb8259976bd",occi.core.title="title",occi.core.summary="summary",occi.compute.architecture="x86",occi.compute.cores=1,occi.compute.hostname="hostname",occi.compute.speed=1.0,occi.compute.memory=1.0,occi.compute.state="inactive"

##### text/plain format with no Accept type
    
    BODY='Category: compute;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind"
    X-OCCI-Attribute: "occi.core.id"="02581c73-7ddd-4c30-bfcd-8cb8259976bd"
    X-OCCI-Attribute: "occi.core.title"="title"
    X-OCCI-Attribute: "occi.core.summary"="summary"
    X-OCCI-Attribute: "occi.compute.architecture"="x86"
    X-OCCI-Attribute: "occi.compute.cores"=1
    X-OCCI-Attribute: "occi.compute.hostname"="hostname"
    X-OCCI-Attribute: "occi.compute.speed"=1.0
    X-OCCI-Attribute: "occi.compute.memory"=1.0
    X-OCCI-Attribute: "occi.compute.state"="inactive"'

    curl -v -X POST -H 'Content-Type: text/plain' -d "$BODY" $ENDPOINT/compute/
    > POST /compute/ HTTP/1.1
    > Host: localhost:3000
    > Accept: */*
    > Content-Type: text/plain
    
##### application/occi+json format with no Accept type

    BODY='{
        "resources": [
            {
                "kind": "http://schemas.ogf.org/occi/infrastructure#compute",
                "attributes": {
                    "occi": {
                        "core": {
                            "title": "title",
                            "summary": "summary"
                        },
                        "compute": {
                            "architecture": "x86",
                            "cores": 1,
                            "hostname": "hostname",
                            "speed": 1,
                            "memory": 1,
                            "state": "inactive"
                        }
                    }
                },
                "id": "175059f6-8f49-4279-98a3-1f4ccfc7ec4b"
            }
        ]
    }'

    curl -v -X POST -H 'Content-Type: application/occi+json' -d "$BODY" $ENDPOINT/compute/
    > POST /compute/ HTTP/1.1
    > User-Agent: curl/7.27.0
    > Host: localhost:3000
    > Accept: */*
    > Content-Type: application/occi+json
    
#### Response

If the compute resource was created successfully, the status of the response MUST be 201. The response returns a URI which ends with the UUID of the resource as defined by the server. Thus the UUID may differ from the UUID used in the request.

##### text/uri-list

The text/uri-list is the preferred format for responses after resource creation.

    < HTTP/1.1 201 Created
    < Content-Type: text/uri-list;charset=utf-8
    < Status: 201
    < Content-Length: 66
    <
    http://localhost:3000/compute/0872c4e0-001a-11e2-b82d-a4b197fffef3
    
##### text/occi

    < HTTP/1.1 201 Created
    < Content-Type: text/occi;charset=utf-8
    < Status: 201
    < X-OCCI-Location: http://localhost:3000/compute/5ad53aba-001a-11e2-b82d-a4b197fffef3
    < Content-Length: 2
    <
    OK

##### text/plain

    < HTTP/1.1 201 Created
    < Content-Type: text/plain;charset=utf-8
    < Status: 201
    < Content-Length: 85
    <
    X-OCCI-Location:  http://localhost:3000/compute/38114460-001a-11e2-b82d-a4b197fffef3

### Create an OCCI storage resource

#### Request

Send a POST to the location of the storage kind e.g. `/storage/` of the OCCI endpoint. The request MUST have the *HTTP Content Type header* set to the correct media type of the enclosed OCCI message. It MAY contain one of the allowed accept types *text/occi*, *text/plain*, *application/occi+json*.

##### Content type text/occi with no Accept type

    curl -v -X POST -H 'Content-Type: text/occi' -H 'Category: storage;scheme="http://schemas.ogf.org/occi/infrastructure"#;class="kind"' -H 'X-OCCI-Attribute: occi.core.id="56933260-6dce-4d89-990f-0c4602b1f2ca",occi.core.title="title",occi.core.summary="summary",occi.storage.size=1.0,occi.storage.state="offline"' $ENDPOINT/storage/        
    > POST /storage/ HTTP/1.1
    > Host: localhost:3000
    > Accept: */*
    > Content-Type: text/occi
    > Category: storage;scheme="http://schemas.ogf.org/occi/infrastructure"#;class="kind"
    > X-OCCI-Attribute: occi.core.id="56933260-6dce-4d89-990f-0c4602b1f2ca",occi.core.title="title",occi.core.summary="summary",occi.storage.size=1.0,occi.storage.state="offline"

##### text/plain format with no Accept type
            
    BODY='Category: storage;scheme="http://schemas.ogf.org/occi/infrastructure"#;class="kind"
    X-OCCI-Attribute: occi.core.id="56933260-6dce-4d89-990f-0c4602b1f2ca"
    X-OCCI-Attribute: occi.core.title="title"
    X-OCCI-Attribute: occi.core.summary="summary"
    X-OCCI-Attribute: occi.storage.size=1.0
    X-OCCI-Attribute: occi.storage.state="offline"'

    curl -v -X GET -H 'Content-Type: text/plain' -d "$BODY" $ENDPOINT/storage/
    > POST /storage/ HTTP/1.1
    > Host: localhost:3000
    > Accept: */*
    > Content-Type: text/occi
    
##### application/occi+json format with no Accept type

    BODY='{
        "resources": [
            {
                "kind": "http://schemas.ogf.org/occi/infrastructure#storage",
                "attributes": {
                    "occi": {
                        "core": {
                            "title": "title",
                            "summary": "summary"
                        },
                        "storage": {
                            "size": 1,
                            "state": "offline"
                        }
                    }
                },
                "id": "d06180c6-3b5b-4868-8e9e-9e3d52648aeb"
            }
        ]
    }'

    curl -v -X POST -H 'Content-Type: application/occi+json' -d "$BODY" $ENDPOINT/storage/
    > POST /storage/ HTTP/1.1
    > User-Agent: curl/7.27.0
    > Host: localhost:3000
    > Accept: */*
    
#### Response

If the compute resource was created successfully, the status of the response MUST be 201. The response returns a URI which ends with the UUID of the resource as defined by the server. Thus the UUID may differ from the UUID used in the request.

##### text/uri-list

The text/uri-list is the preferred format for responses after resource creation.

    < HTTP/1.1 201 Created
    < Content-Type: text/uri-list;charset=utf-8
    < Status: 201
    < Content-Length: 66
    <
    http://localhost:3000/storage/0872c4e0-001a-11e2-b82d-a4b197fffef3
    
##### text/occi

    < HTTP/1.1 201 Created
    < Content-Type: text/occi;charset=utf-8
    < Status: 201
    < X-OCCI-Location: http://localhost:3000/storage/5ad53aba-001a-11e2-b82d-a4b197fffef3
    < Content-Length: 2
    <
    OK

##### text/plain

    < HTTP/1.1 201 Created
    < Content-Type: text/plain;charset=utf-8
    < Status: 201
    < Content-Length: 85
    <
    X-OCCI-Location:  http://localhost:3000/storage/38114460-001a-11e2-b82d-a4b197fffef3

### Create an OCCI network resource

#### Request

Send a POST to the location of the network kind e.g. `/network/` of the OCCI endpoint. The request MUST have the *HTTP Content Type header* set to the correct media type of the enclosed OCCI message. It MAY contain one of the allowed accept types *text/occi*, *text/plain*, *application/occi+json*.

##### Content type text/occi with no Accept type

    curl -v -X POST -H 'Content-Type: text/occi' -H 'Category: network;scheme="http://schemas.ogf.org/occi/infrastructure"#;class="kind"' -H 'X-OCCI-Attribute: occi.core.id="dbbde773-35c7-45e8-b315-19c5777d249b",occi.core.title="title",occi.core.summary="summary",occi.network.vlan=1,occi.network.label="vlan",occi.network.state="inactive"' $ENDPOINT/network/        
    > POST /network/ HTTP/1.1
    > Host: localhost:3000
    > Accept: */*
    > Content-Type: text/occi
    > Category: storage;scheme="http://schemas.ogf.org/occi/infrastructure"#;class="kind"
    > X-OCCI-Attribute: occi.core.id="56933260-6dce-4d89-990f-0c4602b1f2ca",occi.core.title="title",occi.core.summary="summary",occi.storage.size=1.0,occi.storage.state="offline"

##### text/plain format with no Accept type
            
    BODY='Category: network;scheme="http://schemas.ogf.org/occi/infrastructure"#;class="kind"
    X-OCCI-Attribute: occi.core.id="dbbde773-35c7-45e8-b315-19c5777d249b"
    X-OCCI-Attribute: occi.core.title="title"
    X-OCCI-Attribute: occi.core.summary="summary"
    X-OCCI-Attribute: occi.network.vlan=1
    X-OCCI-Attribute: occi.network.label="vlan"
    X-OCCI-Attribute: occi.network.state="inactive"'

    curl -v -X GET -H 'Content-Type: text/plain' -d "$BODY" $ENDPOINT/network/
    > POST /network/ HTTP/1.1
    > Host: localhost:3000
    > Accept: */*
    > Content-Type: text/occi
    
##### application/occi+json format with no Accept type

    BODY='{
        "resources": [
            {
                "kind": "http://schemas.ogf.org/occi/infrastructure#network",
                "attributes": {
                    "occi": {
                        "core": {
                            "title": "title",
                            "summary": "summary"
                        },
                        "network": {
                            "vlan": 1,
                            "label": "vlan",
                            "state": "inactive"
                        }
                    }
                },
                "id": "dbbde773-35c7-45e8-b315-19c5777d249b"
            }
        ]
    }'

    curl -v -X POST -H 'Content-Type: application/occi+json' -d "$BODY" $ENDPOINT/network/
    > POST /network/ HTTP/1.1
    > User-Agent: curl/7.27.0
    > Host: localhost:3000
    > Accept: */*
    
#### Response

If the compute resource was created successfully, the status of the response MUST be 201. The response returns a URI which ends with the UUID of the resource as defined by the server. Thus the UUID may differ from the UUID used in the request.

##### text/uri-list

The text/uri-list is the preferred format for responses after resource creation.

    < HTTP/1.1 201 Created
    < Content-Type: text/uri-list;charset=utf-8
    < Status: 201
    < Content-Length: 66
    <
    http://localhost:3000/network/0872c4e0-001a-11e2-b82d-a4b197fffef3
    
##### text/occi

    < HTTP/1.1 201 Created
    < Content-Type: text/occi;charset=utf-8
    < Status: 201
    < X-OCCI-Location: http://localhost:3000/network/5ad53aba-001a-11e2-b82d-a4b197fffef3
    < Content-Length: 2
    <
    OK

##### text/plain

    < HTTP/1.1 201 Created
    < Content-Type: text/plain;charset=utf-8
    < Status: 201
    < Content-Length: 85
    <
    X-OCCI-Location:  http://localhost:3000/network/38114460-001a-11e2-b82d-a4b197fffef3

### Create an OCCI compute resource with a storagelink and a networkinterface

#### Request

##### text/occi

    curl -v -X POST \
      -H 'Category: compute;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind"' \
      -H 'X-OCCI-Attribute: occi.core.id="30e770fb-876d-4e57-a7eb-9734f0ea4ad1",occi.core.title="title",occi.core.summary="summary",occi.compute.architecture="x86",occi.compute.cores=1,occi.compute.hostname="hostname",occi.compute.speed=1.0,occi.compute.memory=1.0,occi.compute.state="inactive"' \
      -H 'Link: </storage/e99aa5a9-2bb7-42f6-bc35-2fad3a624b93>;rel="http://schemas.ogf.org/occi/infrastructure#storage";self="/storagelink/3670994b-cc03-4cdc-aa8f-9cab0116b66f";category="http://schemas.ogf.org/occi/infrastructure#storagelink";occi.core.id="3670994b-cc03-4cdc-aa8f-9cab0116b66f";occi.core.source="30e770fb-876d-4e57-a7eb-9734f0ea4ad1";occi.core.target="e99aa5a9-2bb7-42f6-bc35-2fad3a624b93";occi.storagelink.deviceid="hda";occi.storagelink.mountpoint="/";occi.storagelink.state="inactive";,</network/a3d6cbda-7d9d-4334-ad95-1c111be78c9c>;rel="http://schemas.ogf.org/occi/infrastructure#network";self="/networkinterface/502a17cb-e856-4f95-9867-f5519c68e30e";category="http://schemas.ogf.org/occi/infrastructure#networkinterface";occi.core.id="502a17cb-e856-4f95-9867-f5519c68e30e";occi.core.source="30e770fb-876d-4e57-a7eb-9734f0ea4ad1";occi.core.target="a3d6cbda-7d9d-4334-ad95-1c111be78c9c";occi.networkinterface.interface="eth0";occi.networkinterface.mac="00:00:00:00:00:00";occi.networkinterface.state="inactive";occi.networkinterface.address="192.168.0.2";occi.networkinterface.gateway="192.168.0.1";occi.networkinterface.allocation="dynamic";' \
      $ENDPOINT/compute/
    > POST /compute/ HTTP/1.1
    > Host: localhost:3000
    > Accept: */*
    > Content-Type: text/occi
    > Category: compute;scheme="http://schemas.ogf.org/occi/infrastructure"#;class="kind"
	> X-OCCI-Attribute: occi.core.id="beb75a39-4888-46d8-a632-557f41d48095",occi.core.title="title",occi.core.summary="summary",occi.compute.architecture="x86",occi.compute.cores=1,occi.compute.hostname="hostname",occi.compute.speed=1.0,occi.compute.memory=1.0,occi.compute.state="inactive
	> Link: </storage/05bbfd97-9482-4177-a15b-3ced927907e0>;rel="http://schemas.ogf.org/occi/infrastructure#storage";self=/storagelink/877e3dd7-c08f-45bb-95c3-d97a6788607b;category=http://schemas.ogf.org/occi/infrastructure#storagelinkocci.core.id=877e3dd7-c08f-45bb-95c3-d97a6788607b;occi.core.source=beb75a39-4888-46d8-a632-557f41d48095;occi.core.target=05bbfd97-9482-4177-a15b-3ced927907e0;occi.storagelink.deviceid=hda;occi.storagelink.mountpoint=/;occi.storagelink.state=inactive;,</network/0843721c-94a8-4963-9d01-8e68ae721aa8>;rel="http://schemas.ogf.org/occi/infrastructure#network";self=/networkinterface/30b87ec3-0bfe-4e5f-bfcb-9489c26b65fd;category=http://schemas.ogf.org/occi/infrastructure#networkinterfaceocci.core.id=30b87ec3-0bfe-4e5f-bfcb-9489c26b65fd;occi.core.source=beb75a39-4888-46d8-a632-557f41d48095;occi.core.target=0843721c-94a8-4963-9d01-8e68ae721aa8;occi.networkinterface.interface=eth0;occi.networkinterface.mac=00:00:00:00:00:00;occi.networkinterface.state=inactive;occi.networkinterface.address=192.168.0.2;occi.networkinterface.gateway=192.168.0.1;occi.networkinterface.allocation=dynamic;
      

##### text/plain

    BODY='Category: compute;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind"
	X-OCCI-Attribute: occi.core.id="30e770fb-876d-4e57-a7eb-9734f0ea4ad1"
	X-OCCI-Attribute: occi.core.title="title"
	X-OCCI-Attribute: occi.core.summary="summary"
	X-OCCI-Attribute: occi.compute.architecture="x86"
	X-OCCI-Attribute: occi.compute.cores=1
	X-OCCI-Attribute: occi.compute.hostname="hostname"
	X-OCCI-Attribute: occi.compute.speed=1.0
	X-OCCI-Attribute: occi.compute.memory=1.0
	X-OCCI-Attribute: occi.compute.state="inactive"
	Link: </storage/e99aa5a9-2bb7-42f6-bc35-2fad3a624b93>;rel="http://schemas.ogf.org/occi/infrastructure#storage";self=/storagelink/3670994b-cc03-4cdc-aa8f-9cab0116b66f;category=http://schemas.ogf.org/occi/infrastructure#storagelink;occi.core.id="3670994b-cc03-4cdc-aa8f-9cab0116b66f";occi.core.source="30e770fb-876d-4e57-a7eb-9734f0ea4ad1";occi.core.target="e99aa5a9-2bb7-42f6-bc35-2fad3a624b93";occi.storagelink.deviceid="hda";occi.storagelink.mountpoint="/";occi.storagelink.state="inactive";
	Link: </network/a3d6cbda-7d9d-4334-ad95-1c111be78c9c>;rel="http://schemas.ogf.org/occi/infrastructure#network";self=/networkinterface/502a17cb-e856-4f95-9867-f5519c68e30e;category=http://schemas.ogf.org/occi/infrastructure#networkinterface;occi.core.id="502a17cb-e856-4f95-9867-f5519c68e30e";occi.core.source="30e770fb-876d-4e57-a7eb-9734f0ea4ad1";occi.core.target="a3d6cbda-7d9d-4334-ad95-1c111be78c9c";occi.networkinterface.interface="eth0";occi.networkinterface.mac="00:00:00:00:00:00";occi.networkinterface.state="inactive";occi.networkinterface.address="192.168.0.2";occi.networkinterface.gateway="192.168.0.1";occi.networkinterface.allocation="dynamic";'

    curl -v -X GET -H 'Content-Type: text/plain' -d "$BODY" $ENDPOINT/compute/
    > POST /compute/ HTTP/1.1
    > Host: localhost:3000
    > Accept: */*
    > Content-Type: text/plain

### Create an OCCI compute resource using an OS and resource template

#### Request

##### text/occi

    curl -v -X POST \
      -H 'Content_type: text/occi' \
      -H 'Category: compute;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind",my_os;scheme="http://my.occi.service//occi/infrastructure/os_tpl#";class="mixin",small;scheme="http://my.occi.service//occi/infrastructure/resource_tpl#";class="mixin"' \
      -H 'X-OCCI-Attribute: occi.core.id="e8a0ea85-3e0d-444c-ae54-2f5540d41d95"' \
      $ENDPOINT/compute/
    > POST /compute/ HTTP/1.1
	> Host: localhost:3000
	> Content-Type: text/occi
	> Category: compute;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind","my_os;scheme="http://my.occi.service//occi/infrastructure/os_tpl#";class="mixin",small;scheme="http://my.occi.service//occi/infrastructure/resource_tpl#";class="mixin"
	> X-OCCI-Attribute: occi.core.id="e8a0ea85-3e0d-444c-ae54-2f5540d41d95"
   

##### text/plain

    BODY='Category: compute;scheme="http://schemas.ogf.org/occi/infrastructure#";class="kind"
	Category: my_os;scheme="http://my.occi.service//occi/infrastructure/os_tpl#";class="mixin"
	Category: small;scheme="http://my.occi.service//occi/infrastructure/resource_tpl#";class="mixin"
	X-OCCI-Attribute: occi.core.id="e8a0ea85-3e0d-444c-ae54-2f5540d41d95"'
	
	curl -v -X POST -H 'Content-Type: text/plain' -d "$BODY" $ENDPOINT/compute/
	> POST /compute/ HTTP/1.1
    > Host: localhost:3000
    > Accept: */*
    > Content-Type: text/plain

##### application/occi+json

    BODY='{
	    "resources": [
	        {
	            "kind": "http://schemas.ogf.org/occi/infrastructure#compute",
	            "mixins": [
	                "http://my.occi.service//occi/infrastructure/os_tpl#my_os",
	                "http://my.occi.service//occi/infrastructure/resource_tpl#small"
	            ],
	            "attributes": {
	                "occi": {
	                    "core": {
	                        "id": "e8a0ea85-3e0d-444c-ae54-2f5540d41d95"
	                    }
	                }
	            },
	            "id": "e8a0ea85-3e0d-444c-ae54-2f5540d41d95"
	        }
	    ]
	}'
	
	curl -v -X POST -H 'Content-Type: application/occi+json' -d "$BODY" $ENDPOINT/compute/
	> POST /compute/ HTTP/1.1
    > Host: localhost:3000
    > Accept: */*
    > Content-Type: application/occi+json
	

#### Response

### Create a storagelink between an existing OCCI compute and OCCI storage resource

### Create a networkinterface between an existing OCCI compute and OCCI network resource

### Create a user defined mixin

### Associate resource with mixin

## Read

### List all entities

#### Request
            
    curl -v -X GET -H 'Accept: text/uri-list' $ENDPOINT/
        
#### Response

##### text/occi

##### text/plain

##### application/occi+json

### List all entities of a specific kind

#### Request

    curl -v -X GET -H 'Accept: text/uri-list' $ENDPOINT/$KIND/

### Describe all entities

#### Request

    curl -v -X GET -H 'Accept: application/occi+json' $ENDPOINT/$KIND/

### Describe all entities of a specific kind

### Describe specific entity

## Update

### Partial update of a specific entity

### Full update of a specific entity

### Full update of a mixin collection

## Delete

### Delete all entities

#### Request

    curl -v -X DELETE $ENDPOINT/
    > DELETE / HTTP/1.1
    > Host: localhost:3000
    > Accept: */*

#### Response

    < HTTP/1.1 200 OK
    < Status: 200
    < Accept: application/occi+json,text/uri-list,text/plain;q=0.5,text/occi;q=0.2
    < Server: SERVERNAME/SERVERVERSION OCCI/1.1
    < Content-Length: 0    

### Delete all entities of a specific kind

#### Request

    curl -v -X DELETE $ENDPOINT/compute/
    > DELETE /compute/ HTTP/1.1
    > Host: localhost:3000
    > Accept: */*

#### Response

    < HTTP/1.1 200 OK
    < Status: 200
    < Accept: application/occi+json,text/uri-list,text/plain;q=0.5,text/occi;q=0.2
    < Server: SERVERNAME/SERVERVERSION OCCI/1.1
    < Content-Length: 0 

### Delete a specific entity

#### Request

    curl -v -X DELETE $ENDPOINT/compute/02581c73-7ddd-4c30-bfcd-8cb8259976bd
    > DELETE /compute/02581c73-7ddd-4c30-bfcd-8cb8259976bd HTTP/1.1
    > Host: localhost:3000
    > Accept: */*

#### Response

    < HTTP/1.1 200 OK
    < Status: 200
    < Accept: application/occi+json,text/uri-list,text/plain;q=0.5,text/occi;q=0.2
    < Server: SERVERNAME/SERVERVERSION OCCI/1.1
    < Content-Length: 0 

### Delete a user defined mixin

### Dissociate resource from a mixin

## Perform action

### Perform action on all entities of a specific kind

#### Request

    curl -v -X POST $ENDPOINT/compute/?action=start
    > POST /compute/ HTTP/1.1
    > Host: localhost:3000
    > Accept: */*

### Perform action on a specific kind

# Interoperability Scenarios

## OVF

### Create an OCCI compute resource from the DMTF OVF example (DSP2021)

    curl -v -X POST -H 'Content-Type: application/ova' -d '@dsp2021.ova' $ENDPOINT/compute/

### Describe OCCI compute resource

### Delete OCCI compute resource

## CDMI

### Create CDMI container

### Create CDMI object by uploading an image file

### Create an OCCI compute resource with a storagelink to an CDMI object

### Describe OCCI compute resource

### Delete OCCI compute resource

### Delete CDMI object

### Delete CDMI container

## AMQP