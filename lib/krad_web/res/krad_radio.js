function create_handler (inst, func) {
    
  return (function(e){
    func.call(inst, e);            
  });
}

window.requestAnimFrame = (function(){
  return  window.requestAnimationFrame       || 
          window.webkitRequestAnimationFrame || 
          window.mozRequestAnimationFrame    || 
          window.oRequestAnimationFrame      || 
          window.msRequestAnimationFrame     || 
          function( callback ){
            window.setTimeout(callback, 1000 / 60);
          };
})();

function Shape(x, y, w, h, fill) {
  // This is a very simple and unsafe constructor. All we're doing is checking if the values exist.
  // "x || 0" just means "if there is a value for x, use that. Otherwise use 0."
  // But we aren't checking anything else! We could put "Lalala" for the value of x 
  this.x = x || 0;
  this.y = y || 0;
  this.w = w || 1;
  this.h = h || 1;
  this.fill = fill || '#AAAAAA';
}

// Draws this shape to a given context
Shape.prototype.draw = function(ctx) {
  ctx.fillStyle = this.fill;
  ctx.fillRect(this.x, this.y, this.w, this.h);
}

// Determine if a point is inside the shape's bounds
Shape.prototype.contains = function(mx, my) {
  // All we have to do is make sure the Mouse X,Y fall in the area between
  // the shape's X and (X + Height) and its Y and (Y + Height)
  return  (this.x <= mx) && (this.x + this.w >= mx) &&
          (this.y <= my) && (this.y + this.h >= my);
}

function CanvasState(canvas) {
  // **** First some setup! ****
  
  this.canvas = canvas;
  this.width = canvas.width;
  this.height = canvas.height;
  this.ctx = canvas.getContext('2d');
  // This complicates things a little but but fixes mouse co-ordinate problems
  // when there's a border or padding. See getMouse for more detail
  var stylePaddingLeft, stylePaddingTop, styleBorderLeft, styleBorderTop;
  if (document.defaultView && document.defaultView.getComputedStyle) {
    this.stylePaddingLeft = parseInt(document.defaultView.getComputedStyle(canvas, null)['paddingLeft'], 10)      || 0;
    this.stylePaddingTop  = parseInt(document.defaultView.getComputedStyle(canvas, null)['paddingTop'], 10)       || 0;
    this.styleBorderLeft  = parseInt(document.defaultView.getComputedStyle(canvas, null)['borderLeftWidth'], 10)  || 0;
    this.styleBorderTop   = parseInt(document.defaultView.getComputedStyle(canvas, null)['borderTopWidth'], 10)   || 0;
  }
  // Some pages have fixed-position bars (like the stumbleupon bar) at the top or left of the page
  // They will mess up mouse coordinates and this fixes that
  var html = document.body.parentNode;
  this.htmlTop = html.offsetTop;
  this.htmlLeft = html.offsetLeft;

  // **** Keep track of state! ****
  
  this.valid = false; // when set to false, the canvas will redraw everything
  this.shapes = [];  // the collection of things to be drawn
  this.dragging = false; // Keep track of when we are dragging
  // the current selected object. In the future we could turn this into an array for multiple selection
  this.selection = null;
  this.dragoffx = 0; // See mousedown and mousemove events for explanation
  this.dragoffy = 0;
  
  // **** Then events! ****
  
  // This is an example of a closure!
  // Right here "this" means the CanvasState. But we are making events on the Canvas itself,
  // and when the events are fired on the canvas the variable "this" is going to mean the canvas!
  // Since we still want to use this particular CanvasState in the events we have to save a reference to it.
  // This is our reference!
  var myState = this;
  
  //fixes a problem where double clicking causes text to get selected on the canvas
  canvas.addEventListener('selectstart', function(e) { e.preventDefault(); return false; }, false);
  // Up, down, and move are for dragging
  canvas.addEventListener('mousedown', function(e) {
    var mouse = myState.getMouse(e);
    var mx = mouse.x;
    var my = mouse.y;
    var shapes = myState.shapes;
    var l = shapes.length;
    for (var i = l-1; i >= 0; i--) {
      if (shapes[i].contains(mx, my)) {
        var mySel = shapes[i];
        // Keep track of where in the object we clicked
        // so we can move it smoothly (see mousemove)
        myState.dragoffx = mx - mySel.x;
        myState.dragoffy = my - mySel.y;
        myState.dragging = true;
        myState.selection = mySel;
        myState.valid = false;
        return;
      }
    }
    // havent returned means we have failed to select anything.
    // If there was an object selected, we deselect it
    if (myState.selection) {
      myState.selection = null;
      myState.valid = false; // Need to clear the old selection border
    }
  }, true);
  canvas.addEventListener('mousemove', function(e) {
    if (myState.dragging){
      var mouse = myState.getMouse(e);
      // We don't want to drag the object by its top-left corner, we want to drag it
      // from where we clicked. Thats why we saved the offset and use it here
      myState.selection.x = mouse.x - myState.dragoffx;
      myState.selection.y = mouse.y - myState.dragoffy;
      
      kradradio.setsprite(myState.selection.x, myState.selection.y);
      
      myState.valid = false; // Something's dragging so we must redraw
    }
  }, true);
  canvas.addEventListener('mouseup', function(e) {
    myState.dragging = false;
  }, true);
  // double click for making new shapes
  //canvas.addEventListener('dblclick', function(e) {
  //  var mouse = myState.getMouse(e);
  //  myState.addShape(new Shape(mouse.x - 10, mouse.y - 10, 20, 20, 'rgba(0,255,0,.6)'));
 // }, true);
  
  // **** Options! ****
  
  this.selectionColor = '#CC0000';
  this.selectionWidth = 2;  
  this.interval = 100;
  
  (function animloop(){
    requestAnimFrame(animloop);
    myState.draw();
  })();
  
}

CanvasState.prototype.addShape = function(shape) {
  this.shapes.push(shape);
  this.valid = false;
}

CanvasState.prototype.clear = function() {
  this.ctx.clearRect(0, 0, this.width, this.height);
}

// While draw is called as often as the INTERVAL variable demands,
// It only ever does something if the canvas gets invalidated by our code
CanvasState.prototype.draw = function() {
  // if our state is invalid, redraw and validate!
  if (!this.valid) {
    var ctx = this.ctx;
    var shapes = this.shapes;
    this.clear();
    
    // ** Add stuff you want drawn in the background all the time here **
    
    // draw all shapes
    var l = shapes.length;
    for (var i = 0; i < l; i++) {
      var shape = shapes[i];
      // We can skip the drawing of elements that have moved off the screen:
      if (shape.x > this.width || shape.y > this.height ||
          shape.x + shape.w < 0 || shape.y + shape.h < 0) continue;
      shapes[i].draw(ctx);
    }
    
    // draw selection
    // right now this is just a stroke along the edge of the selected Shape
    if (this.selection != null) {
      ctx.strokeStyle = this.selectionColor;
      ctx.lineWidth = this.selectionWidth;
      var mySel = this.selection;
      ctx.strokeRect(mySel.x,mySel.y,mySel.w,mySel.h);
    }
    
    // ** Add stuff you want drawn on top all the time here **
    
    this.valid = true;
  }
}


// Creates an object with x and y defined, set to the mouse position relative to the state's canvas
// If you wanna be super-correct this can be tricky, we have to worry about padding and borders
CanvasState.prototype.getMouse = function(e) {
  var element = this.canvas, offsetX = 0, offsetY = 0, mx, my;
  
  // Compute the total offset
  if (element.offsetParent !== undefined) {
    do {
      offsetX += element.offsetLeft;
      offsetY += element.offsetTop;
    } while ((element = element.offsetParent));
  }

  // Add padding and border style widths to offset
  // Also add the <html> offsets in case there's a position:fixed bar
  offsetX += this.stylePaddingLeft + this.styleBorderLeft + this.htmlLeft;
  offsetY += this.stylePaddingTop + this.styleBorderTop + this.htmlTop;

  mx = e.pageX - offsetX;
  my = e.pageY - offsetY;
  
  // We return a simple javascript object (a hash) with x and y defined
  return {x: mx, y: my};
}

function Tag (tag_item, tag_name, tag_value) {

  this.tag_item = tag_item;
  this.tag_name = tag_name;
  this.tag_value = tag_value;

  this.tag_num = Math.floor(Math.random()*122231);

  this.show();

}

Tag.prototype.show = function() {

  if (this.tag_item == "station") {
    
    text = "Station Tag: " + this.tag_name + " - " + this.tag_value;
    
    kradwebsocket.debug(text);
    
    if (this.tag_name.indexOf("_bool") != -1) {

      $('.kradradio_tools').append("<div class='button_wrap'><div class='krad_button_toggle' id='" + this.tag_name + "'>" + this.tag_name.replace("_bool","").replace("_", " ") + "</div>");
      $('.kradlink_tools').append("<br clear='both'/>");  
  
      kradradio.add_bool(this.tag_name);
      kradradio.got_set_bool(this.tag_name, this.tag_value);

      $( '#' + this.tag_name).bind( "click", {bool: this.tag_name}, function(event, ui) {
        kradradio.toggle_bool(event.data.bool);
      });
      
      
    } else {
      $('.station_tags').append("<div id='ktag_" + this.tag_num + "' class='tag'>" + this.tag_item + ": " + this.tag_name + " - <span class='tag_value'>" + this.tag_value + "</span></div>");    
    }

  } else {

    if ((this.tag_name != "artist") && (this.tag_name != "title")) {

      if (this.tag_name == "now_playing") {
        $('#ktags_' + this.tag_item).append("<div id='ktag_" + this.tag_num + "' class='tag nowplaying'><span class='tag_value'>" + this.tag_value + "</span></div>");
        return;
      }
      if (this.tag_name == "playtime") {
        $('#ktags_' + this.tag_item).append("<div id='ktag_" + this.tag_num + "' class='tag playtime'><span class='tag_value'>" + this.tag_value + "</span></div>");
        return;
      }
  
      $('#ktags_' + this.tag_item).append("<div id='ktag_" + this.tag_num + "' class='tag'>" + this.tag_item + ": " + this.tag_name + " - <span class='tag_value'>" + this.tag_value + "</span></div>");          
    }
  }
}

Tag.prototype.update_value = function(tag_value) {

  this.tag_value = tag_value;
  if (this.tag_name.indexOf("_bool") != -1) {  
    kradradio.got_set_bool(this.tag_name, this.tag_value);
  }  
  if ((this.tag_name != "artist") && (this.tag_name != "title")) {
    $('#ktag_' + this.tag_num + ' .tag_value').html(this.tag_value);
  }
}


function Kradwebsocket (port) {

  this.port = port;
  this.uri = 'ws://' + location.hostname + ':' + this.port + '/';
  this.websocket = "";
  this.reconnection_attempts = 0;
  this.reconnect = false;  
  this.connected = false;
  this.connecting = false;
  this.stay_connected = true;  
  this.debug ("Created");
  this.connect();

}

Kradwebsocket.prototype.do_reconnect = function() {

  if (this.connected != true) {
    this.reconnection_attempts += 1;
    if (this.reconnection_attempts == 3) {
      $('#kradradio').append("<div id='websockets_connection_problem'><h2>Websockets connection problem using port " + this.port + "</h2></div>");
    }
    this.connect();
  }

}
    
Kradwebsocket.prototype.connect = function() {

  this.stay_connected = true;
  if (this.connected != true) {
    if (this.connecting != true) {
      this.connecting = true;
      this.debug ("Connecting..");
      this.websocket = new WebSocket (this.uri, "krad-ws-api");
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
  this.stay_connected = false;  
  this.debug ("Disconnecting..");
  if (this.reconnect != false) {
    window.clearInterval(this.reconnect);
    this.reconnect = false;
  }
  this.websocket.close ();
  //this.websocket.onopen = false;
  //this.websocket.onclose = false;
  //this.websocket.onmessage = false;
  //this.websocket.onerror = false;
}
 
Kradwebsocket.prototype.on_open = function(evt) {
  this.connected = true;
  this.debug ("Connected!");
  this.connecting = false;
  this.reconnection_attempts = 0;

  if (this.reconnect != false) {
    window.clearInterval(this.reconnect);
    this.reconnect = false;
  }
  if ($('#websockets_connection_problem')) {
    $('#websockets_connection_problem').remove();
  }

  kradradio = new Kradradio ();  
}

Kradwebsocket.prototype.on_close = function(evt) {
  this.connected = false;
  this.debug ("Disconnected!");
  if (kradradio != false) {
    kradradio.destroy ();
    kradradio = false;
  }
  this.connecting = false;
  if (this.stay_connected == true) {
    if (this.reconnect == false) {
      this.reconnect = setInterval(create_handler(this, this.do_reconnect), 1000);
    }
  }
}

Kradwebsocket.prototype.send = function(message) {
  if (this.connected == true) {
      this.websocket.send (message);
    //this.debug ("Sent " + message); 
  } else {
    this.debug ("NOT CONNECTED, wanted to send: " + message); 
  }
}

Kradwebsocket.prototype.on_error = function(evt) {
  this.debug ("Error! " + evt.data);

}

Kradwebsocket.prototype.debug = function(message) {
  console.log (message);
}

Kradwebsocket.prototype.on_message = function(evt) {
  //this.debug ("got message: " + evt.data);

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
  this.v4l2_devices = new Array();  
  
  this.bools = new Object();  
  
  this.showing_snapshot_buttons = 0;
  this.system_cpu_usage = 0;
  this.admin = 0;
  this.kode = "";
  this.secretkode = "31337";
  this.portgroup_count = 0;
  this.xmms2_portgroup_count = 0;
  this.prepare_link_operation_mode = "";
  
}

Kradradio.prototype.destroy = function () {


  $('#' + this.sysname).remove();

  //clearTimeout(this.timer);

  //for (real_interface in this.real_interfaces) {
  //  this.real_interfaces[real_interface].destroy();
  //}

}

Kradradio.prototype.add_bool = function (name) {
  this.bools[name] = 'false';
}

Kradradio.prototype.set_bool = function (name, value) {
  this.bools[name] = value.toString();
  if (this.bools[name] == 'false') {
    $( '#' + name).removeClass("krad_button_toggle_on");    
  } else {
    $( '#' + name).addClass("krad_button_toggle_on");    
  }
  this.set_tag(name, this.bools[name]);
}

Kradradio.prototype.got_set_bool = function (name, value) {
  this.bools[name] = value.toString();
  if (this.bools[name] == 'false') {
    $( '#' + name).removeClass("krad_button_toggle_on");    
  } else {
    $( '#' + name).addClass("krad_button_toggle_on");    
  }
}

Kradradio.prototype.get_bool = function (name) {
  return this.bools[name];
}

Kradradio.prototype.toggle_bool = function (name) {
  if (this.bools[name] == 'false') {
    this.bools[name] = 'true';
    $( '#' + name).addClass("krad_button_toggle_on");    
    this.set_tag(name, this.bools[name]);
  } else {
    if (this.bools[name] == 'true') {
      this.bools[name] = 'false';
    }
    $( '#' + name).removeClass("krad_button_toggle_on");
    this.set_tag(name, this.bools[name]);
  }
}

Kradradio.prototype.remove_bool = function (name) {
  this.bools[name] = 'false';
}

Kradradio.prototype.set_tag = function (name, value) {

  var cmd = {};  
  cmd.com = "kradradio";  
  cmd.cmd = "stag";
  cmd.tag_name = name;
  cmd.tag_value = value;
  
  var JSONcmd = JSON.stringify(cmd); 

  kradwebsocket.send(JSONcmd);

  //kradwebsocket.debug(JSONcmd);
}

Kradradio.prototype.set_mixer_params = function (mixer_params) {

  this.sample_rate = mixer_params.sample_rate;

  $('.kradmixer_info').append("<div>Mixer Sample Rate: " + this.sample_rate + " </div>"); 

}

Kradradio.prototype.got_frame_rate = function (numerator, denominator) {

  this.frame_rate_numerator = numerator;
  this.frame_rate_denominator = denominator;
  
  $('.kradcompositor_info').append("<div>Compositor Frame Rate: " + this.frame_rate_numerator + " / " + this.frame_rate_denominator + "</div>"); 

}

Kradradio.prototype.got_frame_size = function (width, height) {

  this.frame_width = width;
  this.frame_height = height;

  $('.kradcompositor_info').append("<div>Compositor Frame Size: " + this.frame_width + " x " + this.frame_height + "</div>"); 

}


Kradradio.prototype.add_link = function () {

  var cmd = {};  
  cmd.com = "kradlink";  
  cmd.cmd = "add_link";

  cmd.operation_mode = this.prepare_link_operation_mode;
    cmd.av_mode = $('[name=prepare_link_avmode]:radio:checked').val();

  if (cmd.operation_mode == "transmit") {
      cmd.host = $('[name=prepare_link_server_info_url]').val();
      cmd.port = $('[name=prepare_link_server_info_port]').val();
      cmd.mount = $('[name=prepare_link_server_info_mount]').val();
      cmd.password = $('[name=prepare_link_server_info_password]').val();
  }

  if (cmd.operation_mode == "record") {
      cmd.filename = $('[name=prepare_link_file_info_path]').val();
  }

  if ((cmd.operation_mode == "record") || (cmd.operation_mode == "transmit")) {
    if ((cmd.av_mode == "audio_only") || (cmd.av_mode == "audio_and_video")) {
        cmd.audio_codec = $('[name=prepare_link_audio_codec]:radio:checked').val();  
    }
    if ((cmd.av_mode == "video_only") || (cmd.av_mode == "audio_and_video")) {
        cmd.video_codec = $('[name=prepare_link_video_codec]:radio:checked').val();
    }
  }

  if (cmd.operation_mode == "capture") {
    cmd.video_width = 1280;
    cmd.video_height = 720;
    cmd.fps_numerator = 60000;
    cmd.fps_denominator = 1000;
    cmd.video_source = "decklink";
    cmd.video_device = $('[name=prepare_link_device]:radio:checked').val();
  }


  cmd.port = parseInt(cmd.port);
  
  var JSONcmd = JSON.stringify(cmd); 

  kradwebsocket.send(JSONcmd);

  kradwebsocket.debug(JSONcmd);


}


Kradradio.prototype.add_kradlink_tools = function () {
  $('.kradlink_tools').append("<div class='kradlink_add_link'></div>");
  
  $('.kradlink_add_link').append("<div class='button_wrap'><div class='krad_button2' id='capture'>Capture</div>");  
  $('.kradlink_add_link').append("<div class='button_wrap'><div class='krad_button6' id='record'>Record</div>");
  $('.kradlink_add_link').append("<div class='button_wrap'><div class='krad_button7' id='transmit'>Transmit</div>");
  
  $('.kradlink_tools').append("<br clear='both'/>");  
  
  $('.kradlink_tools').append("<div class='kradlink_prepare_link'></div>");
    
  $('.kradlink_tools').append("<br clear='both'/>");  
  
  $( '#capture').bind( "click", function(event, ui) {
    $('.kradlink_prepare_link').empty();  
    $('.kradlink_prepare_link').append("<h3>Add Capture</h3>");
    
    if ((kradradio.decklink_devices.length == 0) && (kradradio.v4l2_devices.length == 0)) {
    
      $('.kradlink_prepare_link').append("<p>No V4L2 or Decklink devices detected.</p><br clear='both'/>");  
    
    } else {
      kradradio.prepare_link_operation_mode = "capture";
    
      avmode = '<div id="prepare_link_avmode">\
        <input type="radio" id="radio1900" name="prepare_link_avmode" value="audio_only"/><label for="radio1900">Audio</label>\
        <input type="radio" id="radio2900" name="prepare_link_avmode" value="video_only"/><label for="radio2900">Video</label>\
        <input type="radio" id="radio3900" name="prepare_link_avmode" value="audio_and_video"/><label for="radio3900">Audio and Video</label>\
      </div>';
      
      audio_input = '<div id="prepare_link_audio_input">\
        <input type="radio" id="radio431700" name="prepare_link_audio_input" value="opus"/><label for="radio431700">Embedded</label>\
        <input type="radio" id="radio432700" name="prepare_link_audio_input" value="vorbis"/><label for="radio432700">AES/EBU</label>\
        <input type="radio" id="radio433700" name="prepare_link_audio_input" value="flac"/><label for="radio433700">Analog</label>\
      </div>';
      
      video_resolution = '<div id="prepare_link_video_resolution">\
        <input type="radio" id="radio91600" name="prepare_link_video_resolution" value="320x240"/><label for="radio91600">320x240</label>\
        <input type="radio" id="radio92600" name="prepare_link_video_resolution" value="640x480"/><label for="radio92600">640x480</label>\
        <input type="radio" id="radio92601" name="prepare_link_video_resolution" value="480x270"/><label for="radio92601">480x270</label>\
        <input type="radio" id="radio92611" name="prepare_link_video_resolution" value="640x360"/><label for="radio92611">640x360</label>\
        <input type="radio" id="radio92602" name="prepare_link_video_resolution" value="960x540"/><label for="radio92602">960x540</label>\
        <input type="radio" id="radio92603" name="prepare_link_video_resolution" value="1280x720"/><label for="radio92603">1280x720</label>\
        <input type="radio" id="radio92644" name="prepare_link_video_resolution" value="1920x816"/><label for="radio92644">1920x816</label>\
        <input type="radio" id="radio92604" name="prepare_link_video_resolution" value="1920x1080"/><label for="radio92604">1920x1080</label>\
        <input type="radio" id="radio92655" name="prepare_link_video_resolution" value="2048x1556"/><label for="radio92655">2048x1556</label>\
        <input type="radio" id="radio92654" name="prepare_link_video_resolution" value="3840x2160"/><label for="radio92654">3840x2160</label>\
        <input type="radio" id="radio93654" name="prepare_link_video_resolution" value="custom"/><label for="radio93654">Custom</label>\
      </div>';

      video_framerate = '<div id="prepare_link_video_framerate">\
        <input type="radio" id="radio191600" name="prepare_link_video_framerate" value="24000/1001"/><label for="radio191600">23.98 - 24000/1001</label>\
        <input type="radio" id="radio192600" name="prepare_link_video_framerate" value="24000/1000"/><label for="radio192600">24 - 24000/1000</label>\
        <input type="radio" id="radio192601" name="prepare_link_video_framerate" value="30000/1001"/><label for="radio192601">29.97 - 30000/1001</label>\
        <input type="radio" id="radio192611" name="prepare_link_video_framerate" value="30000/1000"/><label for="radio192611">30 - 30000/1000</label>\
        <input type="radio" id="radio192602" name="prepare_link_video_framerate" value="60000/1001"/><label for="radio192602">59.94 - 60000/1001</label>\
        <input type="radio" id="radio192603" name="prepare_link_video_framerate" value="60000/1000"/><label for="radio192603">60 - 60000/1000</label>\
      </div>';

      $('.kradlink_prepare_link').append(audio_input);    
      $('.kradlink_prepare_link').append("<br clear='both'/>");
      $('.kradlink_prepare_link').append(avmode);    
      $('.kradlink_prepare_link').append("<br clear='both'/>");
    
      devices = '<div id="prepare_link_device">';
        for (i = 0; i < kradradio.decklink_devices.length; i++) {
          devices += '<input type="radio" id="radio1991' + i + '" name="prepare_link_device" value="' + i + '"/><label for="radio1991' + i + '">' + kradradio.decklink_devices[i] + '</label>';
        }
      devices += '</div>';
        
      $('.kradlink_prepare_link').append(devices);    
      $('.kradlink_prepare_link').append("<br clear='both'/>");
            
      $('.kradlink_prepare_link').append(video_resolution);    
      $('.kradlink_prepare_link').append("<br clear='both'/>");

      $('.kradlink_prepare_link').append(video_framerate);    
      $('.kradlink_prepare_link').append("<br clear='both'/>");
            
      $('#prepare_link_audio_input').buttonset();
      $('#prepare_link_avmode').buttonset();
      $('#prepare_link_device').buttonset();
      $('#prepare_link_video_resolution').buttonset();
      $('#prepare_link_video_framerate').buttonset();      

      if (kradradio.admin) {
        $('.kradlink_prepare_link').append("<div class='button_wrap'><div class='krad_button7' id='add_link'>Begin</div>");
        $( '#add_link').bind( "click", function(event, ui) {
          kradradio.add_link();
          $('.kradlink_prepare_link').empty();
        });      
      } else {
        $('.kradlink_prepare_link').append("<p>Sorry your not an admin.. but you could be one day!</p><br clear='both'/>");
      }

      $('.kradlink_prepare_link').append("<div class='button_wrap'><div class='krad_button3' id='cancel_add_link'>Cancel</div>");
      $( '#cancel_add_link').bind( "click", function(event, ui) {
        $('.kradlink_prepare_link').empty();
      });
    
    }
    
  });  

  $( '#record').bind( "click", function(event, ui) {
    $('.kradlink_prepare_link').empty();  
    $('.kradlink_prepare_link').append("<h3>Create Recording</h3>");
    
    kradradio.prepare_link_operation_mode = "record";    
    
    avmode = '<div id="prepare_link_avmode">\
      <input type="radio" id="radio1900" name="prepare_link_avmode" value="audio_only"/><label for="radio1900">Audio</label>\
      <input type="radio" id="radio2900" name="prepare_link_avmode" value="video_only"/><label for="radio2900">Video</label>\
      <input type="radio" id="radio3900" name="prepare_link_avmode" value="audio_and_video"/><label for="radio3900">Audio and Video</label>\
    </div>';
  
    container = '<div id="prepare_link_container">\
      <input type="radio" id="radio1800" name="prepare_link_container" value="ogg"/><label for="radio1800">Ogg</label>\
      <input type="radio" id="radio2800" name="prepare_link_container" value="mkv"/><label for="radio2800">MKV</label>\
      <input type="radio" id="radio3800" name="prepare_link_container" value="webm"/><label for="radio3800">WebM</label>\
    </div>';

    video_codec = '<div id="prepare_link_video_codec">\
      <input type="radio" id="radio1600" name="prepare_link_video_codec" value="theora"/><label for="radio1600">Theora</label>\
      <input type="radio" id="radio2600" name="prepare_link_video_codec" value="vp8"/><label for="radio2600">VP8</label>\
    </div>';
  
    audio_codec = '<div id="prepare_link_audio_codec">\
      <input type="radio" id="radio1700" name="prepare_link_audio_codec" value="opus"/><label for="radio1700">Opus</label>\
      <input type="radio" id="radio2700" name="prepare_link_audio_codec" value="vorbis"/><label for="radio2700">Vorbis</label>\
      <input type="radio" id="radio3700" name="prepare_link_audio_codec" value="flac"/><label for="radio3700">FLAC</label>\
    </div>';
    
    file_info = '<div id="prepare_link_file_info" class="prepare_link_file_info">\
      <label for="radio1550">Filename</label><input style="width: 400px;" type="text" id="radio1550" name="prepare_link_file_info_path" value=""/>\
      <br clear="both"/>\
    </div>';

  
    $('.kradlink_prepare_link').append(container);
    $('.kradlink_prepare_link').append("<br clear='both'/>");      
    $('.kradlink_prepare_link').append(avmode);    
    $('.kradlink_prepare_link').append("<br clear='both'/>");      
    $('.kradlink_prepare_link').append(video_codec);
    $('.kradlink_prepare_link').append("<br clear='both'/>");      
    $('.kradlink_prepare_link').append(audio_codec);

    $('.kradlink_prepare_link').append("<br clear='both'/>");      
    $('.kradlink_prepare_link').append(file_info);
            
            
    $('#prepare_link_avmode').buttonset();
    $('#prepare_link_container').buttonset();
    $('#prepare_link_audio_codec').buttonset();    
    $('#prepare_link_video_codec').buttonset();
    
    $('.kradlink_prepare_link').append("<br clear='both'/>");
    if (kradradio.admin) {
      $('.kradlink_prepare_link').append("<div class='button_wrap'><div class='krad_button7' id='add_link'>Start</div>");
      $( '#add_link').bind( "click", function(event, ui) {
        kradradio.add_link();
        $('.kradlink_prepare_link').empty();
      });        
    } else {
      $('.kradlink_prepare_link').append("<p>Sorry your not an admin.. but you could be one day!</p><br clear='both'/>");
    }
    $('.kradlink_prepare_link').append("<div class='button_wrap'><div class='krad_button3' id='cancel_add_link'>Cancel</div>");
    $( '#cancel_add_link').bind( "click", function(event, ui) {
      $('.kradlink_prepare_link').empty();
    });
  });  
  
  $( '#transmit').bind( "click", function(event, ui) {
    $('.kradlink_prepare_link').empty();  
    $('.kradlink_prepare_link').append("<h3>Add Transmission</h3>");
    
    kradradio.prepare_link_operation_mode = "transmit";    
    
    avmode = '<div id="prepare_link_avmode">\
      <input type="radio" id="radio1900" name="prepare_link_avmode" value="audio_only"/><label for="radio1900">Audio</label>\
      <input type="radio" id="radio2900" name="prepare_link_avmode" value="video_only"/><label for="radio2900">Video</label>\
      <input type="radio" id="radio3900" name="prepare_link_avmode" value="audio_and_video"/><label for="radio3900">Audio and Video</label>\
    </div>';
  
    container = '<div id="prepare_link_container">\
      <input type="radio" id="radio1800" name="prepare_link_container" value="ogg"/><label for="radio1800">Ogg</label>\
      <input type="radio" id="radio2800" name="prepare_link_container" value="mkv"/><label for="radio2800">MKV</label>\
      <input type="radio" id="radio3800" name="prepare_link_container" value="webm"/><label for="radio3800">WebM</label>\
    </div>';

    video_codec = '<div id="prepare_link_video_codec">\
      <input type="radio" id="radio1600" name="prepare_link_video_codec" value="theora"/><label for="radio1600">Theora</label>\
      <input type="radio" id="radio2600" name="prepare_link_video_codec" value="vp8"/><label for="radio2600">VP8</label>\
    </div>';
    
    video_resolution = '<div id="prepare_link_video_resolution">\
      <input type="radio" id="radio91600" name="prepare_link_video_resolution" value="320x240"/><label for="radio91600">320x240</label>\
      <input type="radio" id="radio92600" name="prepare_link_video_resolution" value="640x480"/><label for="radio92600">640x480</label>\
      <input type="radio" id="radio92601" name="prepare_link_video_resolution" value="480x270"/><label for="radio92601">480x270</label>\
      <input type="radio" id="radio92611" name="prepare_link_video_resolution" value="640x360"/><label for="radio92611">640x360</label>\
      <input type="radio" id="radio92602" name="prepare_link_video_resolution" value="960x540"/><label for="radio92602">960x540</label>\
      <input type="radio" id="radio92603" name="prepare_link_video_resolution" value="1280x720"/><label for="radio92603">1280x720</label>\
      <input type="radio" id="radio92644" name="prepare_link_video_resolution" value="1920x816"/><label for="radio92644">1920x816</label>\
      <input type="radio" id="radio92604" name="prepare_link_video_resolution" value="1920x1080"/><label for="radio92604">1920x1080</label>\
      <input type="radio" id="radio92655" name="prepare_link_video_resolution" value="2048x1556"/><label for="radio92655">2048x1556</label>\
      <input type="radio" id="radio92654" name="prepare_link_video_resolution" value="3840x2160"/><label for="radio92654">3840x2160</label>\
      <input type="radio" id="radio93654" name="prepare_link_video_resolution" value="custom"/><label for="radio93654">Custom</label>\
    </div>';
  
    audio_codec = '<div id="prepare_link_audio_codec">\
      <input type="radio" id="radio1700" name="prepare_link_audio_codec" value="opus"/><label for="radio1700">Opus</label>\
      <input type="radio" id="radio2700" name="prepare_link_audio_codec" value="vorbis"/><label for="radio2700">Vorbis</label>\
      <input type="radio" id="radio3700" name="prepare_link_audio_codec" value="flac"/><label for="radio3700">FLAC</label>\
    </div>';
  
  
    server_info = '<div id="prepare_link_server_info" class="prepare_link_server_info">\
      <label for="radio1500">Host</label><input style="width: 400px;" type="text" id="radio1500" name="prepare_link_server_info_url" value=""/>\
      <br clear="both"/>\
      <label for="radio1501">Port</label><input style="width: 60px;" type="text" id="radio1501" name="prepare_link_server_info_port" value=""/>\
      <br clear="both"/>\
      <label for="radio1502">Mount</label><input style="width: 190px;" type="text" id="radio1502" name="prepare_link_server_info_mount" value=""/>\
      <br clear="both"/>\
      <label for="radio2400">Password</label><input style="width: 120px;" type="password" id="radio2400" name="prepare_link_server_info_password" value=""/>\
    </div>';
  
    $('.kradlink_prepare_link').append(container);
    $('.kradlink_prepare_link').append("<br clear='both'/>");      
    $('.kradlink_prepare_link').append(avmode);
    $('.kradlink_prepare_link').append("<br clear='both'/>");
    $('.kradlink_prepare_link').append(video_resolution);
    $('.kradlink_prepare_link').append("<br clear='both'/>");          
    $('.kradlink_prepare_link').append(video_codec);
    $('.kradlink_prepare_link').append("<br clear='both'/>");      
    $('.kradlink_prepare_link').append(audio_codec);

    $('.kradlink_prepare_link').append("<br clear='both'/>");      
    $('.kradlink_prepare_link').append(server_info);            
            
    $('#prepare_link_avmode').buttonset();
    $('#prepare_link_video_resolution').buttonset();    
    $('#prepare_link_container').buttonset();
    $('#prepare_link_audio_codec').buttonset();    
    $('#prepare_link_video_codec').buttonset();
    
    $('.kradlink_prepare_link').append("<br clear='both'/>");    
    if (kradradio.admin) {
      $('.kradlink_prepare_link').append("<div class='button_wrap'><div class='krad_button7' id='add_link'>Activate</div>");    
      $( '#add_link').bind( "click", function(event, ui) {
        kradradio.add_link();
        $('.kradlink_prepare_link').empty();
      });
    } else {
      $('.kradlink_prepare_link').append("<p>Sorry your not an admin.. but you could be one day!</p><br clear='both'/>");
    }
    $('.kradlink_prepare_link').append("<div class='button_wrap'><div class='krad_button3' id='cancel_add_link'>Cancel</div>");
    $( '#cancel_add_link').bind( "click", function(event, ui) {
      $('.kradlink_prepare_link').empty();
    });
  });      

}

Kradradio.prototype.got_system_cpu_usage = function (system_cpu_usage) {

  this.system_cpu_usage = system_cpu_usage;

  $('.system_cpu_usage').html(this.system_cpu_usage);

}

Kradradio.prototype.got_sysname = function (sysname) {

  this.sysname = sysname;

  $('#kradradio').append("<div class='kradradio_station' id='" + this.sysname + "'>\
                      <div class='kradmixer'></div>\
                      <br clear='both'>\
                      <div class='kradmixer_beta'></div>\
                      <br clear='both'>\
                      <div class='kradmixer_cappa'></div>\
                      <br clear='both'>\
                      <div class='kradcompositor'>\
                      <canvas id='kradcompositor_canvas' width='1280' height='720'>\
                      </canvas>\
                      </div>\
                      <br clear='both'>\
            <div class='kradradio'>\
              <h2 class='sysname'>" + this.sysname + "</h2>\
              <br clear='both'>\
              <div class='kradradio_tools'></div>\
              <br clear='both'>\
              <div class='station_tags tags'></div>\
              <div class='system_cpu_usage'></div>\
                      </div>\
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
                    </div>");
   
   
  this.add_kradlink_tools();
       
       
  //var s = new CanvasState(document.getElementById('kradcompositor_canvas'));
  //s.addShape(new Shape(40,40,50,50)); // The default is gray
  //s.addShape(new Shape(32,32,640,100, 'lightskyblue'));
       
}


Kradradio.prototype.show_snapshot_buttons = function () {

  if (this.showing_snapshot_buttons == 0) {
  
    kradradio.jsnapshot();
  
    $('.kradcompositor').append("<br clear='both'/>");

    $('.kradcompositor').append("<div class='button_wrap'><div class='krad_button3' id='snapshot'>Snapshot</div>");
  
    $('.kradcompositor').append("<div class='button_wrap'><div class='krad_button3' id='jsnapshot'>JPEG Snapshot</div>");  
  
    $('.kradcompositor').append("<div class='button_wrap'><div class='krad_button3' id='view_snapshot'><a target='_new' href='/snapshot'>View Last</a></div>");  
  
    $('.kradcompositor').append("<br clear='both'/>");
  
    $( '#snapshot').bind( "click", function(event, ui) {
      kradradio.snapshot();
    });
  
    $( '#jsnapshot').bind( "click", function(event, ui) {
      kradradio.jsnapshot();
    });
  
    this.showing_snapshot_buttons = 1;  
  }
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
        kradradio.got_add_portgroup (msg_arr[m].portgroup_name, msg_arr[m].volume, msg_arr[m].crossfade_name, msg_arr[m].crossfade, msg_arr[m].xmms2 );
      }
      if (msg_arr[m].cmd == "remove_portgroup") {
        kradradio.got_remove_portgroup (msg_arr[m].portgroup_name);
      }
      if (msg_arr[m].cmd == "set_mixer_params") {
        kradradio.set_mixer_params (msg_arr[m]);
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
      if (msg_arr[m].info == "cpu") {
        kradradio.got_system_cpu_usage (msg_arr[m].system_cpu_usage);
      }
      if (msg_arr[m].info == "sysname") {
        kradradio.got_sysname (msg_arr[m].infoval);
      }
      if (msg_arr[m].info == "tag") {
        kradradio.got_tag (msg_arr[m].tag_item, msg_arr[m].tag_name, msg_arr[m].tag_value);
      }      
      
    }
    if (msg_arr[m].com == "kradlink") {
      if (msg_arr[m].cmd == "update_link") {
        kradradio.got_update_link (msg_arr[m].link_num, msg_arr[m].update_item, msg_arr[m].update_value);
      }    
      if (msg_arr[m].cmd == "add_link") {
        kradradio.got_add_link (msg_arr[m]);
      }
      if (msg_arr[m].cmd == "remove_link") {
        kradradio.got_remove_link (msg_arr[m]);
      }      
      if (msg_arr[m].cmd == "add_decklink_device") {
        kradradio.got_add_decklink_device (msg_arr[m]);
      }
    }      
  }

}

Kradradio.prototype.setsprite = function (x, y) {

  var cmd = {};  
  cmd.com = "kradcompositor";  
  cmd.cmd = "setsprite";
  cmd.x = x;  
  cmd.y = y;  
  
  var JSONcmd = JSON.stringify(cmd); 

  kradwebsocket.send(JSONcmd);

  //kradwebsocket.debug(JSONcmd);
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

  //kradwebsocket.debug(JSONcmd);
}

Kradradio.prototype.jsnapshot = function () {

  var cmd = {};  
  cmd.com = "kradcompositor";  
  cmd.cmd = "jsnap";
  
  var JSONcmd = JSON.stringify(cmd); 

  kradwebsocket.send(JSONcmd);

  //kradwebsocket.debug(JSONcmd);
}

Kradradio.prototype.snapshot = function () {

  var cmd = {};  
  cmd.com = "kradcompositor";  
  cmd.cmd = "snap";
  
  var JSONcmd = JSON.stringify(cmd); 

  kradwebsocket.send(JSONcmd);

  //kradwebsocket.debug(JSONcmd);
}

Kradradio.prototype.not_admin = function () {

  alert("Sorry your not an admin right now! But you could be one day!");

}

Kradradio.prototype.enable_admin = function () {

  this.admin = 1;
  kradwebsocket.debug("admin enabled");
  

}

Kradradio.prototype.check_kode = function () {

  if (kradradio.kode == kradradio.secretkode) {
    kradradio.enable_admin();
    kradradio.kode = "";    
  }

  for (i = 0; i < kradradio.kode.length; i++) {
    if (kradradio.kode.charAt(i) != kradradio.secretkode.charAt(i)) {
      kradradio.kode = "";
    }
  }

}

Kradradio.prototype.push_dtmf = function (value) {

  var cmd = {};  
  cmd.com = "kradmixer";  
  cmd.cmd = "push_dtmf";
  cmd.dtmf = value;
  
  var JSONcmd = JSON.stringify(cmd); 

  kradwebsocket.send(JSONcmd);

  //kradwebsocket.debug(JSONcmd);
  
  kradradio.kode += value;

  kradradio.check_kode();
  
}

Kradradio.prototype.got_control_portgroup = function (portgroup_name, control_name, value) {

  //kradwebsocket.debug("control portgroup " + portgroup_name + " " + value);

  if ($('#' + portgroup_name)) {
    if (control_name == "volume") {
      $('#' + portgroup_name).slider( "value" , value );
    } else {
      $('#' + portgroup_name + '_crossfade').slider( "value" , value );
    }
  }
}

Kradradio.prototype.got_update_portgroup = function (portgroup_name, control_name, value) {


}

Kradradio.prototype.got_add_portgroup = function (portgroup_name, volume, crossfade_name, crossfade, xmms2) {

  this.portgroup_count += 1;

  if (xmms2 == 1) {
    this.xmms2_portgroup_count += 1;
  }

  volume_div = "<div class='kradmixer_control volume_control' id='portgroup_" + portgroup_name + "_wrap'><div id='" + portgroup_name + "'></div><h2>" + portgroup_name + "</h2></div>";

  if (portgroup_name == "DTMF") {
  
    $('.kradmixer_cappa').append("<div class='kradmixer_control dtmf_pad'> <div id='" + portgroup_name + "_dtmf'> </div> </div>");
  
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
  
  if (portgroup_name == "DTMF") {
    $('.kradmixer_cappa').append(volume_div);
  } else {
    $('.kradmixer').append(volume_div);
  }

  $('#' + portgroup_name).slider({orientation: 'vertical', value: volume, step: 0.5 });

  $( '#' + portgroup_name ).bind( "slide", function(event, ui) {
    kradradio.update_portgroup (portgroup_name, "volume", ui.value);
  });

  if (crossfade_name.length > 0) {
  
    $('.kradmixer').append("<div class='kradmixer_control crossfade_control'> <div id='" + portgroup_name + "_crossfade'></div> <h2>" + portgroup_name + " - " + crossfade_name + "</h2></div>");

    $('#' + portgroup_name + '_crossfade').slider({orientation: 'horizontal', value: crossfade, min: -100, max: 100 });

    $( '#' + portgroup_name + '_crossfade' ).bind( "slide", function(event, ui) {
      kradradio.update_portgroup (portgroup_name, "crossfade", ui.value);
    });
  }
  
  if (xmms2 == 1) {
    xmms2_controls = "<div class='kradmixer_control xmms2_control'><div id='ktags_" + portgroup_name + "'></div><br clear='both'/><div id='" + portgroup_name + "_xmms2'></div></div>";

    if (this.xmms2_portgroup_count == 1) {
    
      //if ($('.dtmf_pad')) {
      //  $('.dtmf_pad').after(xmms2_controls);
      //} else {
        $('.kradmixer_beta').prepend(xmms2_controls);
      //}
    } else {
      $('.kradmixer_beta').append(xmms2_controls);
    }

    $('#' + portgroup_name + '_xmms2').append("<div class='button_wrap'><div class='krad_button_small' id='" + portgroup_name + "_xmms2_prev'>PREV</div></div>");
    $('#' + portgroup_name + '_xmms2').append("<div class='button_wrap'><div class='krad_button_small' id='" + portgroup_name + "_xmms2_play'>PLAY</div></div>");
    $('#' + portgroup_name + '_xmms2').append("<div class='button_wrap'><div class='krad_button_small' id='" + portgroup_name + "_xmms2_pause'>PAUSE</div></div>");
    $('#' + portgroup_name + '_xmms2').append("<div class='button_wrap'><div class='krad_button_small' id='" + portgroup_name + "_xmms2_stop'>STOP</div></div>");
    $('#' + portgroup_name + '_xmms2').append("<div class='button_wrap'><div class='krad_button_small' id='" + portgroup_name + "_xmms2_next'>NEXT</div></div>");        

    $( '#' + portgroup_name + '_xmms2_prev' ).bind( "click", function(event, ui) {
      kradradio.update_portgroup(portgroup_name, "xmms2", "prev");
    });

    $( '#' + portgroup_name + '_xmms2_play' ).bind( "click", function(event, ui) {
      kradradio.update_portgroup(portgroup_name, "xmms2", "play");
    });
    
    $( '#' + portgroup_name + '_xmms2_pause' ).bind( "click", function(event, ui) {
      kradradio.update_portgroup(portgroup_name, "xmms2", "pause");
    });
    
    $( '#' + portgroup_name + '_xmms2_stop' ).bind( "click", function(event, ui) {
      kradradio.update_portgroup(portgroup_name, "xmms2", "stop");
    });
    
    $( '#' + portgroup_name + '_xmms2_next' ).bind( "click", function(event, ui) {
      kradradio.update_portgroup(portgroup_name, "xmms2", "next");
    });
  } else {
    $('#portgroup_' + portgroup_name + '_wrap').append("<div id='ktags_" + portgroup_name + "'></div>");
  }

}

Kradradio.prototype.got_remove_portgroup = function (name) {

  $('#portgroup_' + name + '_wrap').remove();

}

Kradradio.prototype.remove_link = function (link_num) {

  if (!kradradio.admin) {
    kradradio.not_admin();
    return;
  }

  var cmd = {};  
  cmd.com = "kradlink";  
  cmd.cmd = "remove_link";
    cmd.link_num = link_num;  
  
  var JSONcmd = JSON.stringify(cmd); 

  kradwebsocket.send (JSONcmd);

  //kradwebsocket.debug(JSONcmd);
  
  $('#link_' + link_num + '_wrap').remove();
  
}

Kradradio.prototype.got_update_link = function (link_num, control_name, value) {

  kradwebsocket.debug("got_update_link");

  kradwebsocket.debug(link_num);
  kradwebsocket.debug(control_name);
  kradwebsocket.debug(value);    

  if ($('#link_' + link_num + '_wrap')) {
    if ((control_name == "opus_frame_size") || (control_name == "opus_signal") || (control_name == "opus_bandwidth")) {
      
      control_radio = "link_" + link_num + "_" + control_name;
      
      $('[name=' + control_radio + ']').prop('checked',false);
      $('[name=' + control_radio + '][value="' + value + '"]').prop('checked',true);
      
      $('#link_' + link_num + '_' + control_name + '_setting').buttonset( "refresh" );
            
    } else {
      $('#link_' + link_num + '_' + control_name + '_value').html(value);              
      $('#link_' + link_num + '_' + control_name + '_slider').slider( "value" , value );
    }
  }

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

  //kradwebsocket.debug(JSONcmd);
}
  

Kradradio.prototype.got_add_decklink_device = function (decklink_device) {


  this.decklink_devices[decklink_device.decklink_device_num] = decklink_device.decklink_device_name;

  //$('.kradlink').append("<div class='kradlink_link'>" + this.decklink_devices[decklink_device.decklink_device_num] + "</div>");


}


Kradradio.prototype.got_remove_link = function (link) {

  $('#link_' + link.link_num + '_wrap').remove();

}


Kradradio.prototype.got_add_link = function (link) {

  if (link.host == "transmitter") {
    link.host = window.document.location.hostname;
  }

  if (link.operation_mode == "capture") {
    this.show_snapshot_buttons();
  }

  $('.kradlink').append("<div class='kradlink_link' id='link_" + link.link_num + "_wrap'> <div id='link_" + link.link_num + "'></div></div>");

  $('#link_' + link.link_num).append("<h3>" + link.operation_mode.replace("capture","capturing").replace("transmit","transmitting").replace("record","recording") + " " + link.av_mode.replace(" only","") + "</h3>");

  $('#link_' + link.link_num).append("<div id='ktags_link" + link.link_num + "'></div>");

  if (link.operation_mode == "capture") {
    $('#link_' + link.link_num).append("<h5>Device: " + link.video_source + "</h5>");
  }

  if (link.operation_mode == "transmit") {

    if ((link.av_mode == "video only") || (link.av_mode == "audio and video")) {
      $('#link_' + link.link_num).append("<h5>Video Codec: " + link.video_codec + " Resolution: " + link.video_width + "x" + link.video_height + " Frame Rate: " + link.video_fps_numerator + " / " + link.video_fps_denominator + " Color Depth: " + link.video_color_depth + "</h5>");
    }
    if ((link.av_mode == "audio only") || (link.av_mode == "audio and video")) {
      $('#link_' + link.link_num).append("<h5>Audio Codec: " + link.audio_codec + " Channels: " + link.audio_channels + " Sample Rate: " + link.audio_sample_rate + "</h5>");
    }

    if ((link.av_mode == "audio only") || (link.av_mode == "audio and video")) {
    
      if (link.audio_codec == "FLAC") {
        $('#link_' + link.link_num).append("<h5>FLAC Bit Depth: " + link.flac_bit_depth + "</h5>");
      }    
    
      if (link.audio_codec == "Vorbis") {
        $('#link_' + link.link_num).append("<h5>Vorbis Quality: " + link.vorbis_quality + "</h5>");
      }
    }

    $('#link_' + link.link_num).append("<h5><a href='http://" + link.host + ":" + link.port + link.mount + "'>" + link.host + ":" + link.port + link.mount + "</a></h5>");
    
    if (link.av_mode == "audio only") {
      $('#link_' + link.link_num).append("<audio controls preload='none' src='http://" + link.host + ":" + link.port + link.mount + "'>Audio tag should be here.</audio>");    
    }

    if ((link.av_mode == "video only") || (link.av_mode == "audio and video")) {
      $('#link_' + link.link_num).append("<video controls poster='/snapshot' preload='none' width='480' height='270' src='http://" + link.host + ":" + link.port + link.mount + "'>Video tag should be here.</video>");    
    }

    $('#link_' + link.link_num).append("<br clear='both'/><br clear='both'/>");

    if ((link.av_mode == "video only") || (link.av_mode == "audio and video")) {
    
      if (link.video_codec == "Theora") {

        $('#link_' + link.link_num).append("<div class='kradlink_control kradlink_param_control'><div id='link_" + link.link_num + "_theora_quality_slider'></div><h2>Quality&nbsp;<span id='link_" + link.link_num + "_theora_quality_value'>" + link.theora_quality + "</span></h2></div>");

        $('#link_' + link.link_num + '_theora_quality_slider').slider({orientation: 'vertical', value: link.theora_quality, min: 0, max: 63 });

        $( '#link_' + link.link_num + '_theora_quality_slider' ).bind( "slide", function(event, ui) {
          $( '#link_' + link.link_num + '_theora_quality_value' ).html(ui.value);      
          kradradio.update_link (link.link_num, "theora_quality", ui.value);
        });

      }

      if (link.video_codec == "VP8") {
        $('#link_' + link.link_num).append("<div class='kradlink_control kradlink_param_control'><div id='link_" + link.link_num + "_vp8_bitrate_slider'></div><h2>Bitrate&nbsp;<span id='link_" + link.link_num + "_vp8_bitrate_value'>" + link.vp8_bitrate + "</span></h2></div>");

        $('#link_' + link.link_num + '_vp8_bitrate_slider').slider({orientation: 'vertical', value: link.vp8_bitrate, min: 100, max: 10000 });

        $( '#link_' + link.link_num + '_vp8_bitrate_slider' ).bind( "slide", function(event, ui) {
          $( '#link_' + link.link_num + '_vp8_bitrate_value' ).html(ui.value);      
          kradradio.update_link (link.link_num, "vp8_bitrate", ui.value);
        });
    
        $('#link_' + link.link_num).append("<div class='kradlink_control kradlink_param_control'><div id='link_" + link.link_num + "_vp8_min_quantizer_slider'></div><h2>Quantizer&nbsp;Min&nbsp;<span id='link_" + link.link_num + "_vp8_min_quantizer_value'>" + link.vp8_min_quantizer + "</span></h2></div>");

        $('#link_' + link.link_num + '_vp8_min_quantizer_slider').slider({orientation: 'vertical', value: link.vp8_min_quantizer, min: 0, max: 33 });

        $( '#link_' + link.link_num + '_vp8_min_quantizer_slider' ).bind( "slide", function(event, ui) {
          $( '#link_' + link.link_num + '_vp8_min_quantizer_value' ).html(ui.value);      
          kradradio.update_link (link.link_num, "vp8_min_quantizer", ui.value);
        });
        
        $('#link_' + link.link_num).append("<div class='kradlink_control kradlink_param_control'><div id='link_" + link.link_num + "_vp8_max_quantizer_slider'></div><h2>Quantizer&nbsp;Max&nbsp;<span id='link_" + link.link_num + "_vp8_max_quantizer_value'>" + link.vp8_max_quantizer + "</span></h2></div>");

        $('#link_' + link.link_num + '_vp8_max_quantizer_slider').slider({orientation: 'vertical', value: link.vp8_max_quantizer, min: 34, max: 63 });

        $( '#link_' + link.link_num + '_vp8_max_quantizer_slider' ).bind( "slide", function(event, ui) {
          $( '#link_' + link.link_num + '_vp8_max_quantizer_value' ).html(ui.value);      
          kradradio.update_link (link.link_num, "vp8_max_quantizer", ui.value);
        });
        
        $('#link_' + link.link_num).append("<div class='kradlink_control kradlink_param_control'><div id='link_" + link.link_num + "_vp8_deadline_slider'></div><h2>Deadline&nbsp;<span id='link_" + link.link_num + "_vp8_deadline_value'>" + link.vp8_deadline + "</span></h2></div>");

        $('#link_' + link.link_num + '_vp8_deadline_slider').slider({orientation: 'vertical', value: link.vp8_deadline, min: 1, max: 33000 });

        $( '#link_' + link.link_num + '_vp8_deadline_slider' ).bind( "slide", function(event, ui) {
          $( '#link_' + link.link_num + '_vp8_deadline_value' ).html(ui.value);      
          kradradio.update_link (link.link_num, "vp8_deadline", ui.value);
        });    
    
      }
    }  
    
    if ((link.av_mode == "audio only") || (link.av_mode == "audio and video")) {
  
      if (link.audio_codec == "Opus") {
    
    
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
      
        $('#link_' + link.link_num).append("<br clear='both'/>");      
    
        $('#link_' + link.link_num).append("<div class='kradlink_control kradlink_param_control'><div id='link_" + link.link_num + "_opus_bitrate_slider'></div><h2>Bitrate&nbsp;<span id='link_" + link.link_num + "_opus_bitrate_value'>" + link.opus_bitrate + "</span></h2></div>");
        $('#link_' + link.link_num).append("<div class='kradlink_control kradlink_param_control'><div id='link_" + link.link_num + "_opus_complexity_slider'></div><h2>Complexity&nbsp;<span id='link_" + link.link_num + "_opus_complexity_value'>" + link.opus_complexity + "</span></h2></div>");


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
  }
  
  $('#link_' + link.link_num).append("<br clear='both'/>");

  $('#link_' + link.link_num).append("<div class='button_wrap'><div class='krad_button6' id='" + link.link_num + "_remove'>Remove</div>");

  $( '#' + link.link_num + '_remove').bind( "click", function(event, ui) {
    kradradio.remove_link(link.link_num);
  });
  
  $('#link_' + link.link_num).append("<br clear='both'/><br clear='both'/>");  

}
  
  
  
