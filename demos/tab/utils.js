
function vod(obj, field, value) {
	if(obj && obj[field] !== undefined) return obj[field];
	return value;
}

function debug(obj, keysOnly){
	var s = '';
	if(typeof(obj) == 'string' || typeof(obj) == 'number'){
		alert(obj);
	} else {
		for (var k in obj){
			s += k + (keysOnly ? ', ' : '=' + obj[k]+"\n");
		}
		alert(s);
	}
}

function g(x) { return $_h(x); }

function $(id) { return $_h(id); }
function $_h(id) {
    if(typeof(id) == 'string') {
	    return document.getElementById(id);
    } else {
        //id is really a node
        return id;
    }
}

function c(x, y, z) { return $c(x, y, z); }

function $t(text) {
	return document.createTextNode(text);
}

function $c(tagname, attribs, children) {
	if(!tagname) {
		return document.createTextNode(attribs);
	}

	var node = document.createElement(tagname);
	if(attribs) {
		copyProps(attribs, node);
	}
	if(children) {
		if(isScalar(children)) {
			node.innerHTML = children;
		} else {
			//children is an array of nodes, or a single node.
			$ac(node, children);
		}
	}
	return node;
}
function ac(n, c) { return $ac(n, c); }
function $a(n, c) { return $ac(n, c); }
function $ac(parentNode, childNode) {
	$nodeAction(childNode, function(node) {
			$(parentNode).appendChild(node)
		});
	/*
	if(is_array(c)) {
		for(var i = 0; i < c.length; i++) {
			var x = c[i];
			if(x.mainNode) x = x.mainNode;
			n.appendChild(x);
		}
	} else {
		if(c.mainNode) c = c.mainNode;
		n.appendChild(c);
	}*/
}
//insert after
function $ia(siblingNode, insertNode) {
	$nodeAction(insertNode, function(node) {
			var p = siblingNode.parentNode;
			if(node == p.lastChild) p.appendChild(node);
			else p.insertBefore(node, siblingNode.nextSibling);
		});
}
//insert before
function $ib(siblingNode, insertNode) {
	$nodeAction(insertNode, function(node) {
			siblingNode.parentNode.insertBefore(node, siblingNode);
		});
}
function $nodeAction(c, func) {
	if(is_array(c)) {
		for(var i = 0; i < c.length; i++) {
			var x = c[i];
			if(x.mainNode) x = x.mainNode;
			func(x);
		}
	} else {
		if(c.mainNode) c = c.mainNode;
		func(c);
	}
}

function $dc(node) {
	while(node.firstChild) { node.removeChild(node.firstChild); }
}

function $rm(node) {
  if (node.parentNode) {
    node.parentNode.removeChild(node);
  }
}

// hack, better way?
function is_array(o) {
	return (o.length !== undefined) && o.push;
}

function mkSet() {
	var set = {};
	for(var i = 0; i < arguments.length; i++) {
		set[arguments[i]] = arguments[i];
	}
	return set;
}

//takes either
// html element, coords object {x:,y:,width:,height:}
// or
// html element, x, y, w, h
function setPos(obj, x, y, w, h) {
	obj = obj.mainNode || obj;
	var giveUnits = function(v) {
		if( (''+v).match(/[0-9]$/) ) return v+'px';
		return v;
	}
	if(typeof(x) == 'object') {
		assert(obj.style);
		if(x.x != undefined) obj.style.left = giveUnits(x.x);
		if(x.y != undefined) obj.style.top = giveUnits(x.y);
		if(x.width != undefined) obj.style.width = giveUnits(x.width);
		if(x.height != undefined) obj.style.height = giveUnits(x.height);
		return;
	}
	var params = {};
	switch(arguments.length) {
		case 5:
		params.height = h;
		case 4:
		params.width = w;
		case 3:
		params.y = y;
		case 2:
		params.x = x;
		break;
		default:
			assert(0);
	}
	setPos(obj, params);
}

function assert(mustBeTrue) {

  if (mustBeTrue) return;

  try {

    throw new Error("x");

  }

  catch(e) {

   // dump(e.stack + "\n");
	  if(!confirm(e.stack+"\n\n\n"+"Keep Executing?")) {
		  var x = 1 / 0;
		  //guess what this does
		  eval('(*#&$^&*(^%*(^$#%(*&$');
	  };

  }

}


function bound(inside, outside) {
	var bounded = false;
	if(inside.x < outside.x) { inside.x = outside.x; bounded = true; }
	if(inside.y < outside.y) { inside.x = outside.x; bounded = true; }
	var maxx = outside.x + outside.width -
		(inside.width ? inside.width : 0);
	var maxy = outside.y + outside.height -
		(inside.height ? inside.height : 0);
	if(inside.x > maxx) { inside.x = maxx; bounded = true; }
	if(inside.y > maxy) { inside.y = maxy; bounded = true; }
	return bounded;
}

function getWindowSize() {
	var myWidth = 0, myHeight = 0;
	if( typeof( window.innerWidth ) == 'number' ) {
		//Non-IE
		myWidth = window.innerWidth;
		myHeight = window.innerHeight;
	} else if( document.documentElement && ( document.documentElement.clientWidth || document.documentElement.clientHeight ) ) {
		//IE 6+ in 'standards compliant mode'
		myWidth = document.documentElement.clientWidth;
		myHeight = document.documentElement.clientHeight;
	} else if( document.body && ( document.body.clientWidth || document.body.clientHeight ) ) {
		//IE 4 compatible
		myWidth = document.body.clientWidth;
		myHeight = document.body.clientHeight;
	}
	var coord = new Coord(myWidth, myHeight);
	//backwards compatibility:
	coord.width = myWidth; coord.height = myHeight;
	return coord;
}

function coord(x, y) { return new Coord(x, y); }
function Coord(x, y) {
    this.x = x == undefined ? 0 : x;
    this.y = y == undefined ? 0 : y;
}
Coord.prototype.plus = function(coord, y) {
	if(y != undefined) coord = new Coord(coord, y);
	return new Coord(this.x + coord.x, this.y + coord.y);
}
Coord.prototype.minus = function(coord, y) {
	if(y != undefined) coord = new Coord(coord, y);
	return new Coord(this.x - coord.x, this.y - coord.y);
}
Coord.prototype.mul = function(num) {
	return new Coord(this.x * num, this.y * num);
}
Coord.prototype.div = function(num) {
	return new Coord(this.x / num, this.y / num);
}
Coord.prototype.avg = function(coord) {
	return this.plus(coord).div(2);
}
Coord.prototype.toString = function() {
	return '('+this.x+','+this.y+')';
}
Coord.prototype.within = function(obj, c2) {
	var targPos;
	var targWidth;
	var targHeight;

	if(c2) {
		targPos = obj;
		targWidth = c2.x;
		targHeight = c2.y;
	} else if(obj.style) {
			targPos    = getPagePosition(obj);
			targWidth  = parseInt(obj.offsetWidth);
			targHeight = parseInt(obj.offsetHeight);
	} else {
		return false;
	}

	if(
		(this.x > targPos.x)                &&
		(this.x < (targPos.x + targWidth))  &&
		(this.y > targPos.y)                &&
		(this.y < (targPos.y + targHeight))){

		return true;
	}


}


function getPagePosition(element) {
    var coord = new Coord();

    while (element.offsetParent) {
        coord.x += element.offsetLeft;
        coord.y += element.offsetTop;
        element = element.offsetParent;
    }

    return coord;
}

function getRelPosition(element) {
	return new Coord(element.offsetLeft, element.offsetTop);
}


function isScalar(val) {
	var t = typeof(val);
	var ret = t == typeof(' ') || t == typeof(1) || t == typeof(1.2)
		|| t == typeof(true) || !val;

	return ret;
}

function makeFormData(obj, root_field) {
	var s = '';
	for(var k in obj) {
		if(s != '') s += '&';
		var base = root_field ? root_field+'['+k+']' : k;
		if(isScalar(obj[k])) {
			s += escape(base) + '=' + escape(obj[k]);
		} else {
			s += makeFormData(obj[k], base);
		}
	}
	return s;
}

function makeFormPairs(obj, root_field, pairs) {
	if(pairs == undefined) {pairs = {}; }
	for(var k in obj) {
		var base = root_field ? root_field+'['+k+']' : k;
		if(isScalar(obj[k])) {
			pairs[base] = obj[k];
		} else {
			makeFormPairs(obj[k], base, pairs);
		}
	}
	return pairs;
}

//note, 'form' can be e.g. a div inside a form, doesn't have to be
//the form itself. note that everything inside the 'form' gets cleared
//and replaced if 'clear' is set to true
function placeDataInForm(form, obj, root_field, clear) {
	var data = makeFormPairs(obj, root_field);
	if(clear) g(form).innerHTML = '';
	for(var name in data) {
		ac(g(form), c('input',{
			type  : 'hidden',
			name  : name,
			value : data[name]}));
	}
}

/*
copies the contents of container.
the 'name' attribute of all form fields is adjusted:
 - nameMappings is pos => val, pos is which'th [] to set to val.
   e.g. 2=>7, 3=>6 mapps blah[1][abc][1][hi][2] to blah[1][abc][7][6][2]

 */
function createFieldsFromTemplate(container, nameMappings, params) {
	var node = container.cloneNode(true),
		fields = getFormFields(node),
		params = params || {};
	for(var name in fields) {
		var list = is_array(fields[name]) ? fields[name] : [fields[name]];
		list.each(function(field) {
			var parts = name.split(/\[/),
				newname = parts.shift();
			for(i = 0; i < parts.length; i++) {
				newname += '[';
				if(nameMappings[i] !== undefined) {
					newname += nameMappings[i] + parts[i].replace(/^[^\]]*/, '');
				} else {
					newname += parts[i];
				}
			}
			field.name = newname;
		});
	}
	return node;
}



// can work on objects as well as functions,
// so that you can make "virtual classes" using
// just { blah : function() { hello(); } } style
// constructs.
function mixin(sup, child, noProto) {
	var parentMethods = sup.prototype && !noProto ? sup.prototype : sup;
	var childMethods = child.prototype && !noProto ? child.prototype : child;
	copyProps(parentMethods, childMethods);
	if(child.prototype) {
		child.prototype.constructor = child;
	}
}

function is_object(x) {
	return typeof(x) == 'object';
}

function copyProps(from, to, noOverwrite) {
	for(var k in from) {
		if(is_object(from[k]) && from[k]) {
			if(!to[k]) to[k] = {};
			//make sure they are compatible!
			assert(is_object(to[k]));
			copyProps(from[k], to[k], noOverwrite);
		} else {
			try {
				if(!noOverwrite || to[k] == undefined) to[k] = from[k];
			} catch(e) {
				//TODO: try other ways
			}
/*
			NO!! breaks good code!!! also shoudln't it have been k == button
			//TODO: Fix, Dodgy hack, as setAttribute wont work either
			if(to[k] != "button"){
				to[k] = from[k];
			}*/
		}
	}
	return to;
}

function copy(obj) { return copyProps(obj, {}); }

//copies values in 'from' to 'to' only when they do not exist in 'to'.
function copyDefaults(from, to) {
	return copyProps(from, to, true);
}

function extend(sup, child) {
	mixin(sup, child);
	child.prototype.superClass = sup.prototype;
	child.prototype.superCons = function() {
		this.superClass.constructor.apply(this, arguments);
	};
}


// this function isn't tested properly.
function is_a(obj, classObj) {
	do {
		if(obj.constructor && obj.constructor == classObj) return true;
		obj = obj.superClass;
	} while( obj );

	return false;
}

function coverElement(baseNode, coverNode) {
	var targPos    = getPagePosition(baseNode);
	var startPos = new Coord(coverNode.offsetLeft, coverNode.offsetTop);
	coverNode.style.position='absolute';
	var newPos = startPos.plus(targPos).minus(getPagePosition(coverNode));
	newPos.width  = parseInt(baseNode.offsetWidth);
	newPos.height = parseInt(baseNode.offsetHeight);
	setPos(coverNode, newPos);
}

function textSelect(textbox, start, length) {
	textbox = $(textbox);
	if(start == undefined) start = 0;
	if(length == undefined) length = textbox.value.length - start;
	if (textbox.createTextRange) {
		var oRange = textbox.createTextRange();
		oRange.moveStart("character", start);
		oRange.moveEnd("character", length - textbox.value.length);
		oRange.select();
	} else if (textbox.setSelectionRange) {
		textbox.setSelectionRange(start, length);
	}

}

function findNodeUpwardsByTagName(node, tagName) {
	tagName = tagName.toLowerCase();
	while(node) {
		if(node.tagName.toLowerCase() == tagName) return node;
		node = node.parentNode;
	}

	return null;
}

function getFormFields(form) {
	var fields = {}, elts;

	for(var tag in mkSet('input','select','textarea','button')) {
		elts = form.getElementsByTagName(tag);
		for(var i = 0; i < elts.length; i++) {
			var name = elts[i].name;
			//compact dups into an array. useful for fields
			//called e.g. 'blah[]'
			if(fields[name]) {
				if(!is_array(fields[name])) fields[name] = [fields[name]];
				fields[name].push(elts[i]);
			} else {
				fields[elts[i].name] = elts[i];
			}
		}
	}

	return fields;
}

function hasProps(obj) {
	for(var x in obj) return true;
	return false;
}


function setClass(node, className, uniqList) {
	uniqList = uniqList || [className];
	assert(!className || uniqList.include(className));
	uniqList.each(function(name) {
		if(name == className) {
			if(!node.className.match(new RegExp('(^| )'+className+'($| )'))) {
				node.className += (node.className ? ' ' : '') + className;
			}
		} else {
			delClass(node, name);
		}
	});
}
function delClass(node, className) {
	node.className = node.className.replace(new RegExp('(^| )'+className+'($| )'), '');
	node.className.replace(/^ *| *$/g, '');
}

function ucwords(str) {
	return str.replace(/\w+/g, function(s){
          return s.charAt(0).toUpperCase() + s.substr(1).toLowerCase();
     });
}
function ucuswords(str) {
	return ucwords(str.replace(/_/g, ' '));
}

function bytes2size(num) {
	['bytes','KB','MB','GB','TB'].each(function(unit) {
		if(num < 1024 || unit == 'TB') { num = num + ' '+ unit; throw $break; }
		num = Math.round(num / 1024);
	});
	return num;
}

function path_split(path) {
	return path.replace(/\/$/, '').replace(/^\//, '').split('/')
			.findAll(function(s){return s.length>0;});
}


/**
 * Accepts a variable list of path fragments, and returns a valid
 * path concatenation, with no duplicate or trailing slashes
 */
function path_cat(varargs) {
	var path = arguments[0];
	for (var i = 1; i < arguments.length; i++) {
		path = path.replace(/\/*$/, '') + '/' + arguments[i].replace(/^\/*/, '');
	}
	return path.replace(/\/*$/, '');
}

/*
Coord.prototype.within = function(obj) {
	if(obj.style) {
			var targPos    = getPagePosition(obj);
			var targWidth  = parseInt(obj.offsetWidth);
			var targHeight = parseInt(obj.offsetHeight);

			if(
					(this.x > targPos.x)                &&
					(this.x < (targPos.x + targWidth))  &&
					(this.y > targPos.y)                &&
					(this.y < (targPos.y + targHeight))){

				return true;
			}

	}

	return false;
}
*/


Array.prototype.pushMany = function(list) {
	Array.push.apply(this, list);
	return this;
}


//don't touch
function popImage(imageURL,imageTitle){
//really not important (the first two should be small for Opera's sake)
PositionX = 150;
PositionY = 50;
defaultWidth  = 600;
defaultHeight = 400;

//kinda important
var AutoClose = false;

  var imgWin = window.open('','_blank','scrollbars=yes,resizable=1,width='+defaultWidth+',height='+defaultHeight+',left='+PositionX+',top='+PositionY);
  if( !imgWin ) { return true; } //popup blockers should not cause errors
  imgWin.document.write('<html><head><title>'+imageTitle+'<\/title><script type="text\/javascript">\n'+
    'function resizeWinTo() {\n'+
    'if( !document.images.length ) { document.images[0] = document.layers[0].images[0]; }'+
    'var oH = document.images[0].height, oW = document.images[0].width;\n'+
    'if( !oH || window.doneAlready ) { return; }\n'+ //in case images are disabled
    'window.doneAlready = true;\n'+ //for Safari and Opera
    'screenW = (screen.availWidth ? screen.availWidth : screen.width);\n'+
    'screenH = (screen.availHeight ? screen.availHeight : screen.height);\n'+
    'if( oW > screenW ){ oW = screenW-50; }\n'+
    'if( oH > screenH ){ oH = screenH-150; }\n'+
    'var x = window; x.resizeTo( oW + 200, oH + 200 );\n'+
    'var myW = 0, myH = 0, d = x.document.documentElement, b = x.document.body;\n'+
    'if( x.innerWidth ) { myW = x.innerWidth; myH = x.innerHeight; }\n'+
    'else if( d && d.clientWidth ) { myW = d.clientWidth; myH = d.clientHeight; }\n'+
    'else if( b && b.clientWidth ) { myW = b.clientWidth; myH = b.clientHeight; }\n'+
    'if( window.opera && !document.childNodes ) { myW += 16; }\n'+
    'x.resizeTo( oW = oW + ( ( oW + 200 ) - myW ), oH = oH + ( (oH + 200 ) - myH ) );\n'+
    'var scW = screen.availWidth ? screen.availWidth-160 : screen.width-160;\n'+
    'var scH = screen.availHeight ? screen.availHeight-70 : screen.height-70;\n'+
    '//if( !window.opera ) { x.moveTo(Math.round((scW-oW)/2),Math.round((scH-oH)/2)); }\n'+
    '}\n'+
    '<\/script>'+
    '<\/head><body onload="resizeWinTo();"'+(AutoClose?' onblur="self.close();"':'')+'>'+
    (document.layers?('<layer left="0" top="0">'):('<div style="position:absolute;left:0px;top:0px;display:table;">'))+
    '<img src="'+path_cat(g_cfg.the_base_path, imageURL)+'" alt="Loading image ..." title="" onload="resizeWinTo();">'+
    (document.layers?'<\/layer>':'<\/div>')+'<\/body><\/html>');
  imgWin.document.close();
  if( imgWin.focus ) { imgWin.focus(); }
  return false;
}

function autoResizeIframe(iframe_id){

	$(iframe_id).height = 100+ "%";

}

//Deprecate this as soon as prototype.js is updated to 1.5.1
function toJSON(obj) {
 switch (typeof obj) {
  case 'object':
   if (obj) {
    var list = [];
    if (obj instanceof Array) {
     for (var i=0;i < obj.length;i++) {
      list.push(toJSON(obj[i]));
     }
     return '[' + list.join(',') + ']';
    } else {
     for (var prop in obj) {
      list.push('"' + prop + '":' + toJSON(obj[prop]));
     }
     return '{' + list.join(',') + '}';
    }
   } else {
    return 'null';
   }
  case 'string':
   return '"' + obj.replace(/(["'])/g, '\\$1') + '"';
  case 'number':
  case 'boolean':
   return new String(obj);
 }
}

String.prototype.trim = function() {
	return this.replace(/^\s+|\s+$/g,"");
}

String.prototype.ltrim = function() {
	return this.replace(/^\s+/,"");
}

String.prototype.rtrim = function() {
	return this.replace(/\s+$/,"");
}

// fix to make evalJSON use the response text instead of the X-JSON header,
// which doesn't work on firefox when there is a lot of data.
Ajax.RequestFix = Class.create();
Object.extend(Object.extend(Ajax.RequestFix.prototype, Ajax.Request.prototype), {
        evalJSON : function() {
                try {
                        return eval('(' + this.transport.responseText + ')');
                } catch(e) { }
        }
});

function display_block(id) {
	var el = $(id);
	if (el) {
		el.style.display = 'block';
	}
}

function display_none(id) {
	var el = $(id);
	if (el) {
		el.style.display = 'none';
	}
}

/*
 * Used by the drop down menus
 */
function show_dropdown(menu_item, dropdown_id) {
	Element.addClassName($(menu_item),'hover');
}

/*
 * Used by the drop down menus
 */
function hide_dropdown(menu_item, dropdown_id) {
	Element.removeClassName($(menu_item), 'hover');
}
