function create_handler(inst, func){
    
    return (function(e){
        func.call(inst, e);		        
    });

}

function Kradwebsocket (port) {

	this.instance_name = "kw";
	this.abstraction_type = "Kradwebsocket";

	this.real_interfaces = new Array();
	this.port = port;
//	this.station = station;
	this.uri = 'ws://' + location.hostname + ':' + this.port + '/';
	this.websocket = "";
	this.connected = false;
	this.connecting = false;
	this.debug("Created")
	
	this.connect();

}


	Kradwebsocket.prototype.add_real_interface = function(real_interface) {
		this.real_interfaces.push( real_interface );
	}
	
	
	Kradwebsocket.prototype.connect = function() {
	
		if (this.connected != true) {
			if (this.connecting != true) {
				this.connecting = true;
				this.debug("Connecting..");
				if ( j.browser.mozilla ) {
					this.websocket = new MozWebSocket(this.uri, "krad-ipc");
				} else {
					this.websocket = new WebSocket(this.uri, "krad-ipc");
				}
				this.websocket.onopen = create_handler(this, this.on_open);
				this.websocket.onclose = create_handler(this, this.on_close);
				this.websocket.onmessage = create_handler(this, this.on_message);
				this.websocket.onerror = create_handler(this, this.on_error);
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
		this.debug("Disconnecting..");
		this.websocket.close();
		this.websocket.onopen = false;
		this.websocket.onclose = false;
		this.websocket.onmessage = false;
		this.websocket.onerror = false;
	}
 
	Kradwebsocket.prototype.on_open = function(evt) {
		this.connected = true;
		this.debug("Connected!");
		this.connecting = false;
		//Kradstudio.load_abstractions(this.station);
	}
 
	Kradwebsocket.prototype.on_close = function(evt) {
		this.connected = false;
		this.debug("Disconnected!");
	}
	
	Kradwebsocket.prototype.send = function(message) {
		if (this.connected == true) {
		    this.websocket.send(message);
			this.debug("Sent " + message); 
		} else {
			this.debug("NOT CONNECTED, wanted to send: " + message); 
		}
	}
 
	Kradwebsocket.prototype.on_error = function(evt) {
		this.debug("Error! " + evt.data);
		this.disconnect();
	}
 
	Kradwebsocket.prototype.debug = function(message) {
		console.log(message);
	}
 
	Kradwebsocket.prototype.on_message = function(evt) {

		if (evt.data.indexOf("kradmixer:") != -1) {
			mixer.set_control_state(evt.data.replace("kradmixer:", ""));
			return;
		}
	}
