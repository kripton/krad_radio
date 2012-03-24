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
				if (typeof MozWebSocket != 'undefined') {
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

		this.debug ("got message: " + evt.data);


			slices = evt.data.split("|");
			
			for (piece in slices) {

				if (slices[piece].indexOf("kradmixer:set_control") != -1) {
					//alert("add portgroup name " + slices[piece].split(":")[2] + " volume " + slices[piece].split(":")[3]);
					set_control (slices[piece].split(":")[2], slices[piece].split(":")[4]);
					
				}
				if (slices[piece].indexOf("kradmixer:add_portgroup") != -1) {
					
					//alert("add portgroup name " + slices[piece].split(":")[2] + " volume " + slices[piece].split(":")[3]);
					
					add_portgroup (slices[piece].split(":")[2], slices[piece].split(":")[3]);
					
				}
			}
			

	}
	
	
	
	
	
	
/* mixer */


function Kradmixer(mixername) {

	this.instance_name = mixername;
	this.abstraction_type = "Krad Mixer";
	this.nice_name = "Krad Mixer";
	this.controls = "";
	this.real_interfaces = new Array();
	this.ignoreupdate = false;
	this.peak_update_rate = 60;
	this.get_controls();
	this.timer;
}


	Kradmixer.prototype.destroy = function() {
	
		clearTimeout(this.timer);
	
		for (real_interface in this.real_interfaces) {
			this.real_interfaces[real_interface].destroy();
		}
	
	}



	Kradmixer.prototype.add_real_interface = function(real_interface) {
	
		this.real_interfaces.push( real_interface );

		if (this.controls == "") {
			this.get_controls();
		} else {
			//real_interface.generate_controls(this.controls);
		}	
	}


	Kradmixer.prototype.get_controls = function() {
	
		kradwebsocket.send("kradmixer:listcontrols");
	
	}
	
	Kradmixer.prototype.set_controls = function(controls) {
	
		this.controls = controls.split("*");
  		//kludge fix last blank
  		this.controls.pop();
		
		for (real_interface in this.real_interfaces) {
			this.real_interfaces[real_interface].generate_controls(this.controls);
		}
		
		
		setTimeout('kradwebsocket.send("getpeak=")', this.peak_update_rate);

	}
	
	
	Kradmixer.prototype.set_peaks = function(peaks) {
	
		peaks_arr = peaks.split("*");
		peaks_arr.pop();
		
		for ( peak in peaks_arr) {
		
			a_peak = peaks_arr[peak].split("/");
			peakname = a_peak[0];
			peakval = parseFloat(a_peak[1]);
			
			
			for (real_interface in this.real_interfaces) {
				this.real_interfaces[real_interface].set_peak(peakname, peakval);
			}
			
		}
	}
	
	Kradmixer.prototype.set_control_state = function(message) {
		if (this.ignoreupdate == false) {
			for (real_interface in this.real_interfaces) {
				this.real_interfaces[real_interface].set_interface_state(message);
			}		
		}
	}
		
	Kradmixer.prototype.set_control = function(subunit, control, value) {
		kradwebsocket.send("kradmixer:" + control + "/" + value);
	}


	
		
	
	
	
