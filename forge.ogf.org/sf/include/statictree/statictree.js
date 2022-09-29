function getTreeCookieSuffix() {
    var currentPath = window.location.pathname;

    // First trim off everything upto the folder path
    var projectPathRegexp = new RegExp(".*/projects\\.[^/]+");
    var ignore = currentPath.match(projectPathRegexp);
    if (ignore == null) {
	return "";
    }

    ignore = new String(ignore);
    currentPath = currentPath.substring(ignore.length + 1);

    // now trim off everything after the application folder name
    var dotPos = currentPath.indexOf(".");
    if (dotPos > 0) {
	currentPath = currentPath.substring(0, dotPos);
    }

    if (currentPath.length > 0) {
	return "_" + currentPath;
    } else {
	return "";
    }
}

var PREFIX = "";
var BLOCK_SUFFIX = "";
var IMAGE_SUFFIX = "";
var LINK_SUFFIX = "";
var ICON_SUFFIX = "";
var ROOT_NODE_ID = "";
var BASE_NODE_ID = "";

var TREE_COOKIE_NAME = "TreeNodes" + getTreeCookieSuffix();
var TREE_COOKIE_PATTERN = new RegExp(";?" + TREE_COOKIE_NAME + "=[^;]+");
var TREE_COOKIE_DELIMITER = ",";

var _treeNodeArray = new Array();
_treeNodeArray.size = 0;

var _selectedNodeId = 0;

var _selectedNodeStyle = "TreeNodeSelected";

function setPrefix(prefix) {
    PREFIX = prefix;
}

function setBlockSuffix(blockSuffix) {
    BLOCK_SUFFIX = blockSuffix;
}

function setImageSuffix(imageSuffix) {
    IMAGE_SUFFIX = imageSuffix;
}

function setLinkSuffix(linkSuffix) {
    LINK_SUFFIX = linkSuffix;
}

function setIconSuffix(iconSuffix) {
    ICON_SUFFIX = iconSuffix;
}

function setBaseNodeId(baseNodeId) {
    BASE_NODE_ID = baseNodeId;
}

function setRootNodeId(rootNodeId) {
    ROOT_NODE_ID = rootNodeId;
}

function setCurrentNodeId(currentNodeId) {
    _selectedNodeId = currentNodeId;
}

function setSelectedNodeStyle(style) {
    _selectedNodeStyle = style;
}

/**
 * Gets the nodes that should be expanded as a map for quick checking.
 */
function getExpandedNodeMapFromCookie() {
    var nodeArray = getExpandedNodesFromCookie();
    var nodeMap = new Array();

    for (var i = 0; i < nodeArray.length; i++) {
	nodeMap[nodeArray[i]] = true;
    }

    nodeMap.size = nodeArray.length;
    return nodeMap;
}

/**
 * Checks the tree node cookie for all expanded nodes.
 * @returns An array of all expanded nodes.  This is never null.
 */
function getExpandedNodesFromCookie() {
    var match = document.cookie.match(TREE_COOKIE_PATTERN);

    if (match == null) {
	return new Array();
    } else {
	var oldCookieValue = new String(match);

	var startIndex = TREE_COOKIE_NAME.length + 1; // add one for the '=' sign
	if (oldCookieValue.substring(0, 1) == ";") {
	    startIndex++;
	}

	oldCookieValue = oldCookieValue.substring(startIndex);
        return oldCookieValue.split(TREE_COOKIE_DELIMITER);
    }
}

/**
 * Take an array of div ids and add them to the list of expanded nodes in the tree cookie.
 * @param nodeArray The array of nodes that are set to be expanded when the tree is viewed.
 */
function setExpandedNodesInCookie(nodeArray) {
    var nodeString;

    if (nodeArray == null || nodeArray.length == 0) {
	nodeString = "";
    } else {
	nodeString = nodeArray[0];

	for (var i = 1; i < nodeArray.length; i++) {
	    nodeString += TREE_COOKIE_DELIMITER + nodeArray[i];
	}
    }

    var expirationDate = new Date();
    expirationDate.setDate(expirationDate.getDate() + 1);

    document.cookie = TREE_COOKIE_NAME + "=" + nodeString +
		      "; expires=" + expirationDate.toGMTString() + "; path=/sf";
}

/**
 * Given the id of a div, remove the node from the list of expanded nodes in the cookie.
 */
function removeNodeFromCookie(divId) {
    var newExpandedNodes = new Array();

    var arrayLength = 1;
    var expandedNodes = getExpandedNodesFromCookie();
    for (var i = 0; i < expandedNodes.length; i++) {
	if (expandedNodes[i] != divId) {
	    newExpandedNodes[newExpandedNodes.length] = expandedNodes[i];
	    newExpandedNodes.length = arrayLength++;
	}
    }

    setExpandedNodesInCookie(newExpandedNodes);
}

/**
 * Add a node to the cookie.
 */
function addNodeToCookie(divId) {
    var expandedNodes = getExpandedNodesFromCookie();

    for (var i = 0; i < expandedNodes.length; i++) {
	if (expandedNodes[i] == divId) {
	    return;
	}
    }

    var arrayLength = expandedNodes.length;
    expandedNodes[expandedNodes.length] = divId;
    expandedNodes.length = arrayLength + 1;

    setExpandedNodesInCookie(expandedNodes);
}

/**
 * Change the visibility of a node's children
 * @param divId The id of the div tag that contains node whose children are being affected
 */
function hideChildren(divId) {
    var divChild = document.getElementById(divId + BLOCK_SUFFIX);
    var imageEl = document.getElementById(divId + IMAGE_SUFFIX);

    if (divChild == null) {
	return true;
    }

    if (divChild.style.display == "none") {
	divChild.style.display = "block";
	if (imageEl != null) {
	    imageEl.src = imageEl.src.replace(/plus/gi, "minus");
	    addNodeToCookie(divId);
	}
    } else {
	divChild.style.display = "none";
	if (imageEl != null) {
	    imageEl.src = imageEl.src.replace(/minus/gi, "plus");
	    removeNodeFromCookie(divId);
	}
    }

    return true;
}

/**
 * Open a child node if it is closed.  If it is already opened, do nothing.
 */
function openChildren(divId) {
    var divElId;
    if (divId.indexOf(PREFIX) != 0) {
        divElId = PREFIX + divId;
    } else {
        divElId = divId;
        divId = divId.substring(PREFIX.length);
    }

    var divChild = document.getElementById(divElId + BLOCK_SUFFIX);
    if (divChild == null) {
	return true;
    }

    if (divChild.style.display == "none") {
        if (_treeNodeArray[divId] == null) {
            var imageEl = document.getElementById(divElId + IMAGE_SUFFIX);
	    imageEl.onclick();
	} else {
	    hideChildren(divElId);
	}
    }

    return true;
}

/**
 * Toggle a nodes visibility.  Make sure that it is registered first.
 * @param nodeId The id of the node
 * @param parentNodeId The id of the parent node.
 * @param The location of the open icon for this node.
 */
function toggleVisibility(nodeId, parentNodeId, openIcon) {
    // If we have not added this node yet, do it now.
    if (_treeNodeArray[nodeId] == null) {
        var linkLocation = document.getElementById(PREFIX + nodeId + LINK_SUFFIX);
        var closedIcon = document.getElementById(PREFIX + nodeId + ICON_SUFFIX).src;

        addNode(new NodeInfo(nodeId, parentNodeId, linkLocation, openIcon, closedIcon));
    }

    hideChildren(PREFIX + nodeId);
}

/**
 * Add a node to the known tree nodes.
 */
function addNode(nodeInfo) {
    if (nodeInfo != null && nodeInfo.getId() != null) {
        _treeNodeArray[nodeInfo.getId()] = nodeInfo;
	_treeNodeArray.size++;
    }
}

/**
 * This object stores information about a node
 */
function NodeInfo(id, parentId, linkLocation, openIcon, closedIcon) {
    this.id = id;
    this.parentId = parentId;
    this.linkLocation = linkLocation;
    this.openIcon = openIcon;
    this.closedIcon = closedIcon;
    this.indexId = -1;

    this.getId = function() {
        return this.id;
    }

    this.getParentId = function() {
        return this.parentId;
    }

    this.getLinkLocation = function() {
        return this.linkLocation;
    }

    this.getOpenIcon = function() {
        return this.openIcon;
    }

    this.getClosedIcon = function() {
        return this.closedIcon;
    }

    this.setIndexId = function(indexId) {
        this.indexId = indexId;
    }

    this.getIndexId = function() {
        return this.indexId;
    }
}

/**
 * Initialize the tree.  Open the folder that is presently being visited and restore the state from cookies of the
 * other folders.
 */
function initializeTree() {
    // Get the node that is being viewed
    var parentNode = null;
    var currentNode = _treeNodeArray[_selectedNodeId];
    if (currentNode != null) {
	var selectedClass = _selectedNodeId == ROOT_NODE_ID ? "RootNodeSelected" : _selectedNodeStyle;
	var currentDiv = document.getElementById(PREFIX + currentNode.getId());
	for (var i = 0; i < currentDiv.attributes.length; i++) {
	    var attribute = currentDiv.attributes[i];
	    if (attribute.name == "class") {
		attribute.value += " " + selectedClass;
		break;
	    }
	}
    }
	    
    // Now expand all of the nodes that should be expanded
    var expandedNodeMap = getExpandedNodeMapFromCookie();

    // Step through the smaller of the two maps to open blocks that were previously opened
    for (var blockId in expandedNodeMap) {
	openChildren(blockId);
    }
}
