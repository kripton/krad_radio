function create_handler (inst, func) {
    
	return (function(e){
		func.call(inst, e);		        
	});
}

function Tag (tag_item, tag_name, tag_value) {

	this.tag_item = tag_item;
	this.tag_name = tag_name;
	this.tag_value = tag_value;

	this.tag_num = Math.floor(Math.random()*122231);

	this.show();

}

Tag.prototype.show = function() {

	if ((this.tag_name != "artist") && (this.tag_name != "title")) {

		if (this.tag_name == "now_playing") {
			$('#ktags_' + this.tag_item).append("<div id='ktag_" + this.tag_num + "' class='tag'><span class='tag_value'>" + this.tag_value + "</span></div>");
			return;
		}
		if (this.tag_name == "playtime") {
			$('#ktags_' + this.tag_item).append("<div id='ktag_" + this.tag_num + "' class='tag'><span class='tag_value'>" + this.tag_value + "</span></div>");
			return;
		}
	
		$('#ktags_' + this.tag_item).append("<div id='ktag_" + this.tag_num + "' class='tag'>" + this.tag_item + ": " + this.tag_name + " - <span class='tag_value'>" + this.tag_value + "</span></div>");					
	}
}

Tag.prototype.update_value = function(tag_value) {

	this.tag_value = tag_value;
	if ((this.tag_name != "artist") && (this.tag_name != "title")) {
		$('#ktag_' + this.tag_num + ' .tag_value').html(this.tag_value);
	}
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

	kradradio = new Kradradio ();	
}

Kradwebsocket.prototype.on_close = function(evt) {
	this.connected = false;
	this.debug ("Disconnected!");
	if (kradradio != false) {
		kradradio.destroy ();
		kradradio = false;
	}
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
	//console.log (message);
}

Kradwebsocket.prototype.on_message = function(evt) {
	this.debug ("got message: " + evt.data);

	kradradio.got_messages (evt.data);	
}
	

function Kradradio () {

	this.sysname = "";
	this.sample_rate = 0;
	//this.real_interfaces = new Array();
	//this.ignoreupdate = false;
	//this.update_rate = 50;
	//this.timer;
	this.tags = new Array();
	
	this.decklink_devices = new Array();	
	
}

Kradradio.prototype.destroy = function () {


	$('#' + this.sysname).remove();

	//clearTimeout(this.timer);

	//for (real_interface in this.real_interfaces) {
	//	this.real_interfaces[real_interface].destroy();
	//}

}

Kradradio.prototype.got_sample_rate = function (sample_rate) {

	this.sample_rate = sample_rate;

	$('.kradmixer_info').append("<div>Sample Rate: " + this.sample_rate + " </div>"); 

}

Kradradio.prototype.got_frame_rate = function (numerator, denominator) {

	this.frame_rate_numerator = numerator;
	this.frame_rate_denominator = denominator;
	
	$('.kradcompositor_info').append("<div>Frame Rate: " + this.frame_rate_numerator + " / " + this.frame_rate_denominator + "</div>"); 

}

Kradradio.prototype.got_frame_size = function (width, height) {

	this.frame_width = width;
	this.frame_height = height;

	$('.kradcompositor_info').append("<div>Frame Size: " + this.frame_width + " x " + this.frame_height + "</div>"); 

}

Kradradio.prototype.got_sysname = function (sysname) {

	this.sysname = sysname;

	$('body').append("<div class='kradradio_station' id='" + this.sysname + "'>\
	                    <div class='kradmixer'></div>\
	                    <br clear='both'>\
	                    <div class='kradcompositor'></div>\
	                    <br clear='both'>\
	                    <div class='kradlink'></div>\
	                    <br clear='both'>\
						<div class='kradmixer_info'></div>\
	                    <br clear='both'>\
	                    <div class='kradcompositor_info'></div>\
	                    <br clear='both'>\
	                    <div class='kradlink_info'></div>\
	                    <br clear='both'>\
	                    <div class='kradmixer_tools'></div>\
	                    <br clear='both'>\
	                    <div class='kradcompositor_tools'></div>\
						<br clear='both'>\
	                    <div class='kradlink_tools'></div>\
	                    <br clear='both'>\
						<div class='kradradio'>\
	                      <h2>" + this.sysname + "</h2>\
	                      <div class='tags'></div>\
	                    </div>\
	                    <br clear='both'>\
	                  </div>");
}

Kradradio.prototype.got_tag = function (tag_item, tag_name, tag_value) {

	for (t in this.tags) {
		if ((this.tags[t].tag_item == tag_item) && (this.tags[t].tag_name == tag_name)) {
			this.tags[t].update_value (tag_value);
			return;
		}
	}
	
	tag = new Tag (tag_item, tag_name, tag_value);
	
	this.tags.push (tag);
	
}

Kradradio.prototype.got_messages = function (msgs) {

	var msg_arr = JSON.parse (msgs);

	for (m in msg_arr) {
		if (msg_arr[m].com == "kradmixer") {
			if (msg_arr[m].cmd == "control_portgroup") {
				kradradio.got_control_portgroup (msg_arr[m].portgroup_name, msg_arr[m].control_name, msg_arr[m].value);
			}
			if (msg_arr[m].cmd == "update_portgroup") {
				kradradio.got_update_portgroup (msg_arr[m].portgroup_name, msg_arr[m].control_name, msg_arr[m].value);
			}			
			if (msg_arr[m].cmd == "add_portgroup") {
				kradradio.got_add_portgroup (msg_arr[m].portgroup_name, msg_arr[m].volume, msg_arr[m].crossfade_name, msg_arr[m].crossfade );
			}
			if (msg_arr[m].cmd == "remove_portgroup") {
				kradradio.got_remove_portgroup (msg_arr[m].portgroup_name);
			}
			if (msg_arr[m].cmd == "set_sample_rate") {
				kradradio.got_sample_rate (msg_arr[m].sample_rate);
			}
		}
		if (msg_arr[m].com == "kradcompositor") {
			if (msg_arr[m].cmd == "set_frame_rate") {
				kradradio.got_frame_rate (msg_arr[m].numerator, msg_arr[m].denominator);
			}
			if (msg_arr[m].cmd == "set_frame_size") {
				kradradio.got_frame_size (msg_arr[m].width, msg_arr[m].height);
			}			
		}		
		if (msg_arr[m].com == "kradradio") {
			if (msg_arr[m].info == "sysname") {
				kradradio.got_sysname (msg_arr[m].infoval);
			}
			if (msg_arr[m].info == "tag") {
				kradradio.got_tag (msg_arr[m].tag_item, msg_arr[m].tag_name, msg_arr[m].tag_value);
			}			
			
		}
		if (msg_arr[m].com == "kradlink") {
			if (msg_arr[m].cmd == "add_link") {
				kradradio.got_add_link (msg_arr[m]);
			}
			if (msg_arr[m].cmd == "add_decklink_device") {
				kradradio.got_add_decklink_device (msg_arr[m]);
			}
		}			
	}

}

Kradradio.prototype.update_portgroup = function (portgroup_name, control_name, value) {

	var cmd = {};  
	cmd.com = "kradmixer";  
	cmd.cmd = "update_portgroup";
  	cmd.portgroup_name = portgroup_name;  
  	cmd.control_name = control_name;  
	cmd.value = value;
	
	var JSONcmd = JSON.stringify(cmd); 

	kradwebsocket.send(JSONcmd);

	kradwebsocket.debug(JSONcmd);
}

Kradradio.prototype.push_dtmf = function (value) {

	var cmd = {};  
	cmd.com = "kradmixer";  
	cmd.cmd = "push_dtmf";
	cmd.dtmf = value;
	
	var JSONcmd = JSON.stringify(cmd); 

	kradwebsocket.send(JSONcmd);

	kradwebsocket.debug(JSONcmd);
}

Kradradio.prototype.got_control_portgroup = function (portgroup_name, control_name, value) {

	//kradwebsocket.debug("control portgroup " + portgroup_name + " " + value);

	if ($('#' + portgroup_name)) {
		if (control_name == "volume") {
			$('#' + portgroup_name).slider( "value" , value )
		} else {
			$('#' + portgroup_name + '_crossfade').slider( "value" , value )
		}
	}
}

Kradradio.prototype.got_update_portgroup = function (portgroup_name, control_name, value) {


}

Kradradio.prototype.got_add_portgroup = function (portgroup_name, volume, crossfade_name, crossfade) {

	$('.kradmixer').append("<div class='kradmixer_control volume_control' id='portgroup_" + portgroup_name + "_wrap'><div id='" + portgroup_name + "'></div><h2>" + portgroup_name + "</h2><div id='ktags_" + portgroup_name + "'></div></div>");

	$('#' + portgroup_name).slider({orientation: 'vertical', value: volume });

	$( '#' + portgroup_name ).bind( "slide", function(event, ui) {
		kradradio.update_portgroup (portgroup_name, "volume", ui.value);
	});
	
	if (portgroup_name == "DTMF") {
	
		$('.kradmixer').append("<div class='kradmixer_control dtmf_pad'> <div id='" + portgroup_name + "_dtmf'> </div> </div>");
	
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button' id='" + portgroup_name + "_dtmf_1'>1</div></div>");
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button' id='" + portgroup_name + "_dtmf_2'>2</div></div>");
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button' id='" + portgroup_name + "_dtmf_3'>3</div></div>");
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button2' id='" + portgroup_name + "_dtmf_a'>F0</div></div>");
		
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button' id='" + portgroup_name + "_dtmf_4'>4</div>");
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button' id='" + portgroup_name + "_dtmf_5'>5</div>");
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button' id='" + portgroup_name + "_dtmf_6'>6</div>");
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button2' id='" + portgroup_name + "_dtmf_b'>F</div></div>");
		
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button' id='" + portgroup_name + "_dtmf_7'>7</div>");
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button' id='" + portgroup_name + "_dtmf_8'>8</div>");
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button' id='" + portgroup_name + "_dtmf_9'>9</div>");		
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button2' id='" + portgroup_name + "_dtmf_c'>I</div></div>");
		
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button' id='" + portgroup_name + "_dtmf_star'>*</div>");		
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button' id='" + portgroup_name + "_dtmf_0'>0</div>");
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button' id='" + portgroup_name + "_dtmf_hash'>#</div>");
		$('#' + portgroup_name + '_dtmf').append("<div class='button_wrap'><div class='krad_button2' id='" + portgroup_name + "_dtmf_d'>P</div></div>");

		$( '#' + portgroup_name + '_dtmf_1').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("1");
		});
		
		$( '#' + portgroup_name + '_dtmf_2').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("2");
		});
		
		$( '#' + portgroup_name + '_dtmf_3').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("3");
		});
		
		$( '#' + portgroup_name + '_dtmf_4').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("4");
		});						

		$( '#' + portgroup_name + '_dtmf_5').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("5");
		});
		
		$( '#' + portgroup_name + '_dtmf_6').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("6");
		});
		
		$( '#' + portgroup_name + '_dtmf_7').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("7");
		});
		
		$( '#' + portgroup_name + '_dtmf_8').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("8");
		});	
		
		$( '#' + portgroup_name + '_dtmf_9').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("9");
		});
		
		$( '#' + portgroup_name + '_dtmf_0').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("0");
		});
		
		$( '#' + portgroup_name + '_dtmf_star').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("*");
		});
		
		$( '#' + portgroup_name + '_dtmf_hash').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("#");
		});
		
		$( '#' + portgroup_name + '_dtmf_a').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("A");
		});
		
		$( '#' + portgroup_name + '_dtmf_b').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("B");
		});
		
		$( '#' + portgroup_name + '_dtmf_c').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("C");
		});					

		$( '#' + portgroup_name + '_dtmf_d').bind( "click", function(event, ui) {
			kradradio.push_dtmf ("D");
		});
		
	}
	
	if (crossfade_name.length > 0) {
	
		$('.kradmixer').append("<div class='kradmixer_control crossfade_control'> <div id='" + portgroup_name + "_crossfade'></div> <h2>" + portgroup_name + " - " + crossfade_name + "</h2></div>");

		$('#' + portgroup_name + '_crossfade').slider({orientation: 'horizontal', value: crossfade, min: -100, max: 100 });

		$( '#' + portgroup_name + '_crossfade' ).bind( "slide", function(event, ui) {
			kradradio.update_portgroup (portgroup_name, "crossfade", ui.value);
		});	
	
	
	}
}

Kradradio.prototype.got_remove_portgroup = function (name) {

	$('#portgroup_' + name + '_wrap').remove();

}

Kradradio.prototype.remove_link = function (link_num) {

	var cmd = {};  
	cmd.com = "kradlink";  
	cmd.cmd = "remove_link";
  	cmd.link_num = link_num;  
	
	var JSONcmd = JSON.stringify(cmd); 

	kradwebsocket.send (JSONcmd);

	kradwebsocket.debug(JSONcmd);
}

Kradradio.prototype.update_link = function (link_num, control_name, value) {

	var cmd = {};  
	cmd.com = "kradlink";  
	cmd.cmd = "update_link";
  	cmd.link_num = link_num;  
  	cmd.control_name = control_name;  
	cmd.value = value;
	
	var JSONcmd = JSON.stringify(cmd); 

	kradwebsocket.send (JSONcmd);

	kradwebsocket.debug(JSONcmd);
}
	

Kradradio.prototype.got_add_decklink_device = function (decklink_device) {


	this.decklink_devices[decklink_device.decklink_device_num] = decklink_device.decklink_device_name;

	$('.kradlink').append("<div class='kradlink_link'>" + this.decklink_devices[decklink_device.decklink_device_num] + "</div>");


}


Kradradio.prototype.got_add_link = function (link) {

	$('.kradlink').append("<div class='kradlink_link'> <div id='link_" + link.link_num + "'></div></div>");

	$('#link_' + link.link_num).append("<h3>" + link.operation_mode + " " + link.av_mode + "</h3>");

	$('#link_' + link.link_num).append("<div id='ktags_link" + link.link_num + "'></div>");

	if (link.operation_mode == "capture") {
		$('#link_' + link.link_num).append("<h5>" + link.video_source + "</h5>");
	}


	if (link.operation_mode == "transmit") {

		if ((link.av_mode == "video only") || (link.av_mode == "audio and video")) {
			$('#link_' + link.link_num).append("<h5>Video Codec: " + link.video_codec + "</h5>");
		}
		if ((link.av_mode == "audio only") || (link.av_mode == "audio and video")) {
			$('#link_' + link.link_num).append("<h5>Audio Codec: " + link.audio_codec + "</h5>");
		}
		
		if (link.av_mode == "audio only") {
			$('#link_' + link.link_num).append("<audio controls preload='none' src='http://" + link.host + ":" + link.port + link.mount + "'>Audio tag should be here.</audio>");		
		}

		if ((link.av_mode == "video only") || (link.av_mode == "audio and video")) {
			$('#link_' + link.link_num).append("<video controls poster='/snapshot' preload='none' width='480' height='270' src='http://" + link.host + ":" + link.port + link.mount + "'>Video tag should be here.</video>");		
		}

		$('#link_' + link.link_num).append("<h5><a href='http://" + link.host + ":" + link.port + link.mount + "'>" + link.host + ":" + link.port + link.mount + "</a></h5>");
		
		
		if ((link.audio_codec == "Opus") && ((link.av_mode == "audio only") || (link.av_mode == "audio and video"))) {
		
			$('#link_' + link.link_num).append("<h5>Opus frame size: </h5>");
			
			
			
			frame_size_controls = '<div id="link_' + link.link_num + '_opus_frame_size_setting">\
				<input type="radio" id="radio1" name="link_' + link.link_num + '_opus_frame_size" value="120"/><label for="radio1">120</label>\
				<input type="radio" id="radio2" name="link_' + link.link_num + '_opus_frame_size" value="240"/><label for="radio2">240</label>\
				<input type="radio" id="radio3" name="link_' + link.link_num + '_opus_frame_size" value="480"/><label for="radio3">480</label>\
				<input type="radio" id="radio4" name="link_' + link.link_num + '_opus_frame_size" value="960"/><label for="radio4">960</label>\
				<input type="radio" id="radio5" name="link_' + link.link_num + '_opus_frame_size" value="1920"/><label for="radio5">1920</label>\
				<input type="radio" id="radio6" name="link_' + link.link_num + '_opus_frame_size" value="2880"/><label for="radio6">2880</label>\
			</div>';
			
			$('#link_' + link.link_num).append(frame_size_controls);

			$("input[name=link_" + link.link_num + "_opus_frame_size][value=" + link.opus_frame_size + "]").attr('checked', 'checked');

			$('#link_' + link.link_num + '_opus_frame_size_setting').buttonset();			

			$( "input[name=link_" + link.link_num + "_opus_frame_size]" ).bind( "change", function(event, ui) {
				var valu = $('input[name=link_' + link.link_num + '_opus_frame_size]:checked').val();
				kradradio.update_link (link.link_num, "opus_frame_size", parseInt (valu));
			});

			signal_controls = '<h5>Opus Signal Type: </h5><div id="link_' + link.link_num + '_opus_signal_setting">\
				<input type="radio" id="radio7" name="link_' + link.link_num + '_opus_signal" value="OPUS_AUTO"/><label for="radio7">Auto</label>\
				<input type="radio" id="radio8" name="link_' + link.link_num + '_opus_signal" value="OPUS_SIGNAL_VOICE"/><label for="radio8">Voice</label>\
				<input type="radio" id="radio9" name="link_' + link.link_num + '_opus_signal" value="OPUS_SIGNAL_MUSIC"/><label for="radio9">Music</label>\
			</div>';
			
			$('#link_' + link.link_num).append(signal_controls);

			$("input[name=link_" + link.link_num + "_opus_signal][value=" + link.opus_signal + "]").attr('checked', 'checked');
			
			$('#link_' + link.link_num + '_opus_signal_setting').buttonset();

			$( "input[name=link_" + link.link_num + "_opus_signal]" ).bind( "change", function(event, ui) {
				var valus = $('input[name=link_' + link.link_num + '_opus_signal]:checked').val();
				kradradio.update_link (link.link_num, "opus_signal", valus);
			});	
			
			
			bandwidth_controls = '<h5>Opus Audio Bandwidth: </h5><div id="link_' + link.link_num + '_opus_bandwidth_setting">\
				<input type="radio" id="radio10" name="link_' + link.link_num + '_opus_bandwidth" value="OPUS_AUTO"/><label for="radio10">Auto</label>\
				<input type="radio" id="radio11" name="link_' + link.link_num + '_opus_bandwidth" value="OPUS_BANDWIDTH_NARROWBAND"/><label for="radio11">Narrowband</label>\
				<input type="radio" id="radio12" name="link_' + link.link_num + '_opus_bandwidth" value="OPUS_BANDWIDTH_MEDIUMBAND"/><label for="radio12">Mediumband</label>\
				<input type="radio" id="radio13" name="link_' + link.link_num + '_opus_bandwidth" value="OPUS_BANDWIDTH_WIDEBAND"/><label for="radio13">Wideband</label>\
				<input type="radio" id="radio14" name="link_' + link.link_num + '_opus_bandwidth" value="OPUS_BANDWIDTH_SUPERWIDEBAND"/><label for="radio14">Super Wideband</label>\
				<input type="radio" id="radio15" name="link_' + link.link_num + '_opus_bandwidth" value="OPUS_BANDWIDTH_FULLBAND"/><label for="radio15">Fullband</label>\
			</div>';
			
			$('#link_' + link.link_num).append(bandwidth_controls);

			$("input[name=link_" + link.link_num + "_opus_bandwidth][value=" + link.opus_bandwidth + "]").attr('checked', 'checked');
			
			$('#link_' + link.link_num + '_opus_bandwidth_setting').buttonset();

			$( "input[name=link_" + link.link_num + "_opus_bandwidth]" ).bind( "change", function(event, ui) {
				var valub = $('input[name=link_' + link.link_num + '_opus_bandwidth]:checked').val();
				kradradio.update_link (link.link_num, "opus_bandwidth", valub);
			});
			
			
			$('#link_' + link.link_num).append("<h5>Opus bitrate: <span id='link_" + link.link_num + "_opus_bitrate_value'>" + link.opus_bitrate + "</span></h5><div id='link_" + link.link_num + "_opus_bitrate_slider'></div>");
			$('#link_' + link.link_num).append("<h5>Opus complexity: <span id='link_" + link.link_num + "_opus_complexity_value'>" + link.opus_complexity + "</span></h5><div id='link_" + link.link_num + "_opus_complexity_slider'></div>");


			$('#link_' + link.link_num + '_opus_bitrate_slider').slider({orientation: 'vertical', value: link.opus_bitrate, min: 3000, max: 320000 });

			$( '#link_' + link.link_num + '_opus_bitrate_slider' ).bind( "slide", function(event, ui) {
				$( '#link_' + link.link_num + '_opus_bitrate_value' ).html(ui.value);			
				kradradio.update_link (link.link_num, "opus_bitrate", ui.value);
			});		


			$('#link_' + link.link_num + '_opus_complexity_slider').slider({orientation: 'vertical', value: link.opus_complexity, min: 0, max: 10 });

			$( '#link_' + link.link_num + '_opus_complexity_slider' ).bind( "slide", function(event, ui) {
				$( '#link_' + link.link_num + '_opus_complexity_value' ).html(ui.value);
				kradradio.update_link (link.link_num, "opus_complexity", ui.value);
			});
		}
	}

	$('#link_' + link.link_num).append("<div class='button_wrap'><div class='krad_button2' id='" + link.link_num + "_remove'>Remove</div>");

	$( '#' + link.link_num + '_remove').bind( "click", function(event, ui) {
		kradradio.remove_link(link.link_num);
	});
	
	$('#link_' + link.link_num).append("<br clear='both'/>");	

}
	
	
	
