hex			x,y,size-radis,color
selector    x,y,size
viper		x,y,size,dir,color
grid        x,y,w,h,lines
meter		y, x, size, pos
rect		x,y,w,h,color
circle		x,y,size-radis
triangle?	a,b,c, color



void krad_gui_render_hex (krad_gui_t *krad_gui, int x, int y, int w) {
void krad_gui_render_selector (krad_gui_t *krad_gui, int x, int y, int w) {
void krad_gui_render_viper (krad_gui_t *krad_gui, int x, int y, int size, int direction) {
void krad_gui_render_grid (krad_gui_t *krad_gui, int x, int y, int w, int h, int lines) {
void krad_gui_render_meter (krad_gui_t *krad_gui, int x, int y, int size, float pos) {
void krad_gui_render_tri (krad_gui_t *krad_gui, int a, int b, int c) {
void krad_gui_render_circles (krad_gui_t *krad_gui, int w, int h) {
void krad_gui_render_rect (krad_gui_t *krad_gui, int w, int h) {
