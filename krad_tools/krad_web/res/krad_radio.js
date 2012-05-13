function create_handler (inst, func) {
    
	return (function(e){
		func.call(inst, e);		        
	});
}

function Kradwebsocket (port) {

	this.port = port;
	this.uri = 'ws://' + location.hostname + ':' + this.port + '/';
	this.websocket = "";
	this.connected = false;
	this.connecting = false;
	this.debug ("Created")	
	this.connect ();

}
		
Kradwebsocket.prototype.connect = function() {

	if (this.connected != true) {
		if (this.connecting != true) {
			this.connecting = true;
			this.debug ("Connecting..");
			this.websocket = new WebSocket (this.uri, "krad-ipc");
			this.websocket.onopen = create_handler (this, this.on_open);
			this.websocket.onclose = create_handler (this, this.on_close);
			this.websocket.onmessage = create_handler (this, this.on_message);
			this.websocket.onerror = create_handler (this, this.on_error);
		} else {
			this.debug("Tried to connect but in the process of connecting."); 
		}
	} else {
		this.debug("Tried to connect when already connected."); 
	}
}
	
Kradwebsocket.prototype.disconnect = function() {
	this.connected = false;
	this.connecting = false;
	this.debug ("Disconnecting..");
	this.websocket.close ();
	this.websocket.onopen = false;
	this.websocket.onclose = false;
	this.websocket.onmessage = false;
	this.websocket.onerror = false;
}
 
Kradwebsocket.prototype.on_open = function(evt) {
	this.connected = true;
	this.debug ("Connected!");
	this.connecting = false;
	
	kradradio = new Kradradio ("testname");
	
}

Kradwebsocket.prototype.on_close = function(evt) {
	this.connected = false;
	this.debug ("Disconnected!");
	kradradio.destroy ();
}

Kradwebsocket.prototype.send = function(message) {
	if (this.connected == true) {
	    this.websocket.send (message);
		this.debug ("Sent " + message); 
	} else {
		this.debug ("NOT CONNECTED, wanted to send: " + message); 
	}
}

Kradwebsocket.prototype.on_error = function(evt) {
	this.debug ("Error! " + evt.data);
	this.disconnect ();
}

Kradwebsocket.prototype.debug = function(message) {
	console.log (message);
}

Kradwebsocket.prototype.on_message = function(evt) {
	this.debug ("got message: " + evt.data);
	kradradio.got_messages (evt.data);
}
	

function Kradradio (sysname) {

	this.sysname = sysname;
	this.controls = "";
	this.real_interfaces = new Array();
	this.ignoreupdate = false;
	this.update_rate = 50;
	//this.timer;
	
	$('body').append("<div class='kradradio' id='" + this.sysname + "'><h2>" + this.sysname + "</h2><div class='kradmixer'></div></div>");
	
}

Kradradio.prototype.destroy = function() {


	$('#' + this.sysname).remove();

	//clearTimeout(this.timer);

	//for (real_interface in this.real_interfaces) {
	//	this.real_interfaces[real_interface].destroy();
	//}

}

Kradradio.prototype.got_messages = function (msgs) {

	var msg_arr = JSON.parse (msgs);

	for (m in msg_arr) {
		if (msg_arr[m].com == "kradmixer") {
			if (msg_arr[m].cmd == "update_portgroup") {
				kradradio.got_update_portgroup (msg_arr[m].portgroup_name, msg_arr[m].volume);
			}
			if (msg_arr[m].cmd == "add_portgroup") {
				kradradio.got_add_portgroup (msg_arr[m].portgroup_name, msg_arr[m].volume);
			}
			if (msg_arr[m].cmd == "remove_portgroup") {
				kradradio.got_remove_portgroup (msg_arr[m].portgroup_name);
			}			
		}
		if (msg_arr[m].com == "kradradio") {
			console.log ("got info!" + msg_arr[m].info);
		}		
	}

}

Kradradio.prototype.update_portgroup = function (portgroup_name, value) {

	var cmd = {};  
	cmd.com = "kradmixer";  
	cmd.cmd = "update_portgroup";
  	cmd.portgroup_name = portgroup_name;  
	cmd.volume = value;
	
	var JSONcmd = JSON.stringify(cmd); 

	kradwebsocket.send (JSONcmd);

	console.log (JSONcmd);
}

Kradradio.prototype.got_update_portgroup = function (portgroup_name, value) {

	console.log ("update portgroup " + portgroup_name + " " + value);

	if ($('#' + portgroup_name)) {
		$('#' + portgroup_name).slider( "value" , value )
	}
}

Kradradio.prototype.got_add_portgroup = function (name, volume) {

	$('.kradmixer').append("<div id='" + name + "'></div> <h2>" + name + "</h2></div>");

	$('#' + name).slider();

	$('#' + name).slider({orientation: 'vertical', value: volume });

	$( "#" + name ).bind( "slide", function(event, ui) {
		kradradio.update_portgroup (name, ui.value);
	});
	
	//		$('#xfader_' + name).slider({orientation: 'horizontal', value: 0, min: -100 });

}

Kradradio.prototype.got_remove_portgroup = function (name) {

	$('#' + name).remove();

}
	
		
	
	
	
