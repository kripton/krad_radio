use core::*;
use std::*;
use krad_radio_client::*;

fn kr_client_create_rust(client_name: &str) -> *kr_client_t unsafe {
  let client_name_bytes = str::to_bytes(client_name);
  return kr_client_create( vec::raw::to_ptr(client_name_bytes) as *c_schar);
}

fn kr_connect_rust(client: *kr_client_t, station_name: &str) -> int unsafe {
  let station_name_bytes = str::to_bytes(station_name);
  return kr_connect( client, vec::raw::to_ptr(station_name_bytes) as *c_schar) as int;
}

fn main() {

  if (os::args().len() < 2) {
    io::print("No station specified.\n");
    return;
  }

  let arg1 = os::args()[1].to_str();

  let client: *kr_client_t;
  let client_name = "rusted krad";
  let station_name = arg1;

  client = kr_client_create_rust(client_name);

  if (kr_connect_rust(client, station_name) != 1) {
    io::print(fmt!("%s could not connect to %s\n", client_name, station_name));
    kr_client_destroy(&client);
    return;
  }
  
  kr_system_info (client);
  kr_client_response_wait_print (client);

  //kr_logname (client);
  //kr_client_response_wait_print (client);

  kr_uptime (client);
  kr_client_response_wait_print (client);
  
  kr_system_cpu_usage (client);  
  kr_client_response_wait_print (client);
    
  kr_remote_status (client);
  kr_client_response_wait_print (client);


  kr_client_destroy(&client);

}
