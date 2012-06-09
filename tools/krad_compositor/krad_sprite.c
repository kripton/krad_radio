/*

#!/usr/bin/env ruby

# converting a animated gif to a png sprite sheet

input = ARGV[0]
frames = `identify #{input} | wc -l`.chomp
output = input.downcase.sub(".gif", "") + "_" + frames + ".png"

puts input + " -> " + output

`montage #{input} -tile x1 -geometry '1x1+0+0<' -alpha On -background "rgba(0, 0, 0, 0.0)" -quality 100 #{output}`

*/

krad_sprite_t *krad_sprite_create (char *filename);
void krad_sprite_destroy (krad_sprite_t *krad_sprite);

void krad_sprite_set_xy (krad_sprite_t *krad_sprite, int x, int y);
void krad_sprite_set_tickrate (krad_sprite_t *krad_sprite, int skip);
void krad_sprite_render (krad_sprite_t *krad_sprite, ...);
