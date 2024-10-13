#include "WaylandCairoWindow.h"
#include "WaylandDisplay.h"
#include "CompoundWidget.h"
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

WaylandCairoWindow::Style WaylandCairoWindow::style;


struct SHMBackend : public WaylandCairoWindow::Backend {
	SHMBackend(struct wl_surface* wayland_surface_in)
		: wayland_surface(wayland_surface_in) {}
	~SHMBackend();
	cairo_surface_t* prepare(int width, int height);
	void swap();
	void idle(int width, int height);
	TimeSeconds next_idle_time() { return next_check_time; }

	struct Pool {
		Pool(size_t size);
		~Pool();
		bool is_valid() { return wayland_pool != nullptr; }

		struct wl_shm_pool* wayland_pool = nullptr;
		size_t size = 0;
		void* data = nullptr;
		};
	struct Buffer {
		~Buffer() { deallocate(); }
		bool is_valid() { return wayland_buffer != nullptr; }
		void allocate(int width, int height);
		void deallocate();
		bool is_oversize(int width, int height);
		void cairo_surface_destroyed();
		void wayland_release();
		int width() { return cairo_surface ? cairo_image_surface_get_width(cairo_surface) : 0; }
		int height() { return cairo_surface ? cairo_image_surface_get_height(cairo_surface) : 0; }

		bool busy = false;
		cairo_surface_t* cairo_surface = nullptr;
		struct wl_buffer* wayland_buffer = nullptr;
		Pool* pool = nullptr;
		};

	struct wl_surface* wayland_surface;
	Buffer buffers[2];
	Buffer* cur_buffer = nullptr;
	TimeSeconds next_check_time = { 0, 0 };
	static constexpr int check_seconds = 5;
	};


WaylandCairoWindow::WaylandCairoWindow(double initial_width, double initial_height)
	: CairoWindow(initial_width, initial_height), cairo_gui(this)
{
	wayland_surface = wl_compositor_create_surface(wayland_display.compositor);
	if (wayland_surface == nullptr)
		return;
	wl_surface_set_user_data(wayland_surface, this);

	// Surface enter/leave... but we don't actually use this.
	static const struct wl_surface_listener surface_listener = {
		.enter = [](void* data, wl_surface* wayland_surface, wl_output* output) {
			((WaylandCairoWindow*) data)->surface_enter(output);
			},
		.leave = [](void* data, wl_surface* wayland_surface, wl_output* output) {
			((WaylandCairoWindow*) data)->surface_leave(output);
			}
		};
	wl_surface_add_listener(wayland_surface, &surface_listener, this);

	// xdg_wm surface.
	if (wayland_display.xdg_wm_base) {
		xdg_surface = xdg_wm_base_get_xdg_surface(wayland_display.xdg_wm_base, wayland_surface);
		static const struct xdg_surface_listener xdg_surface_listener = {
			.configure = [](void* data, struct xdg_surface* xdg_surface, uint32_t serial) {
				xdg_surface_ack_configure(xdg_surface, serial);
				((WaylandCairoWindow*) data)->redraw();
				},
			};
		xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, this);
		xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
		static const struct xdg_toplevel_listener toplevel_listener = {
			.configure = [](void* data, struct xdg_toplevel* toplevel, int32_t width, int32_t height, struct wl_array* states) {
				((WaylandCairoWindow*) data)->resize(width, height, states);
				},
			.close = [](void* data, struct xdg_toplevel* toplevel) {
				((WaylandCairoWindow*) data)->close_requested();
				},
			};
		xdg_toplevel_add_listener(xdg_toplevel, &toplevel_listener, this);
		wl_surface_commit(wayland_surface);
		}

	// Decoration manager surface.
	// TODO: when the decoration manager goes "stable", try that one first.
	if (wayland_display.xdg_decoration_manager_v1) {
		xdg_decoration_v1 =
			zxdg_decoration_manager_v1_get_toplevel_decoration(wayland_display.xdg_decoration_manager_v1, xdg_toplevel);
		static const struct zxdg_toplevel_decoration_v1_listener decoration_listener = {
			.configure = [](void *data, struct zxdg_toplevel_decoration_v1* decoration, uint32_t mode) {
				((WaylandCairoWindow*) data)->server_side_decorations =
					(mode == ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
				},
			};
		zxdg_toplevel_decoration_v1_add_listener(xdg_decoration_v1, &decoration_listener, this);
		zxdg_toplevel_decoration_v1_set_mode(xdg_decoration_v1, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
		}

	backend = new SHMBackend(wayland_surface);
	if (backend == nullptr)
		return;

	frame_buttons.emplace_back(FrameButton::Close);
	frame_buttons.emplace_back(FrameButton::Maximize);
	frame_buttons.emplace_back(FrameButton::Minimize);
	layout_frame_buttons();

	wayland_display.add_window(this);
}


WaylandCairoWindow::~WaylandCairoWindow()
{
	wayland_display.remove_window(this);

	delete backend;
	if (xdg_toplevel)
		xdg_toplevel_destroy(xdg_toplevel);
	if (xdg_surface)
		xdg_surface_destroy(xdg_surface);
	if (wayland_surface)
		wl_surface_destroy(wayland_surface);
}


void WaylandCairoWindow::set_title(const std::string& new_title)
{
	if (xdg_toplevel == nullptr)
		return;

	title = new_title;
	xdg_toplevel_set_title(xdg_toplevel, title.c_str());
	wl_surface_commit(wayland_surface);
}


void WaylandCairoWindow::resized(double new_width, double new_height)
{
	width = new_width;
	height = new_height;
	if (widget) {
		widget->rect = { 0, 0, width, height };
		widget->layout();
		}
}


void WaylandCairoWindow::mouse_pressed(double x, double y, int button, uint32_t serial)
{
	if (!server_side_decorations) {
		bool is_in_frame =
			x < style.frame_left ||
			x >= style.frame_left + width ||
			y < style.frame_top ||
			y >= style.frame_top + height;
		if (is_in_frame) {
			frame_mouse_pressed(x, y, serial);
			return;
			}
		x -= style.frame_left;
		y -= style.frame_top;
		}

	if (button != 1)
		return;

	if (widget)
		widget->mouse_pressed(x, y);
	redraw();
}

void WaylandCairoWindow::mouse_released(double x, double y, int button)
{
	if (!server_side_decorations) {
		bool is_in_frame =
			x < style.frame_left ||
			x >= style.frame_left + width ||
			y < style.frame_top ||
			y >= style.frame_top + height;
		if (is_in_frame) {
			if (tracking_frame_button && button == 1)
				frame_mouse_released(x, y);
			return;
			}
		x -= style.frame_left;
		y -= style.frame_top;
		}

	if (button == 1) {
		if (widget)
			widget->mouse_released(x, y);
		redraw();
		}
}


void WaylandCairoWindow::mouse_moved(int32_t x, int32_t y)
{
	if (get_cairo() == nullptr)
		return;
	if (!server_side_decorations) {
		bool is_in_frame =
			x < style.frame_left ||
			x >= style.frame_left + width ||
			y < style.frame_top ||
			y >= style.frame_top + height;
		if (is_in_frame) {
			frame_mouse_moved(x, y);
			return;
			}
		x -= style.frame_left;
		y -= style.frame_top;
		}

	if (widget)
		widget->mouse_moved(x, y);
	redraw();

	int new_cursor = widget ? widget->preferred_cursor(x, y) : Widget::PointerCursor;
	if (new_cursor != last_cursor) {
		last_cursor = new_cursor;
		wayland_display.use_cursor(new_cursor);
		}
}


void WaylandCairoWindow::entered(double x, double y)
{
	last_cursor = -1;
	mouse_moved(x, y);
}

void WaylandCairoWindow::scroll_up(double x, double y)
{
	if (widget)
		widget->scroll_up(x, y);
}

void WaylandCairoWindow::scroll_down(double x, double y)
{
	if (widget)
		widget->scroll_down(x, y);
}


void WaylandCairoWindow::refresh()
{
	if (cairo_surface == nullptr)
		return;

	cairo_surface_flush(cairo_surface);
	backend->swap();
	cairo_destroy(cairo);
	cairo = nullptr;
	cairo_surface_destroy(cairo_surface);
	cairo_surface = nullptr;
}


cairo_t* WaylandCairoWindow::get_cairo()
{
	if (cairo)
		return cairo;

	auto total_width = width;
	auto total_height = height;
	if (!server_side_decorations) {
		total_width += style.frame_left + style.frame_right;
		total_height += style.frame_top + style.frame_bottom;
		}
	cairo_surface = backend->prepare(total_width, total_height);
	if (cairo_surface == nullptr)
		return nullptr;
	cairo = cairo_create(cairo_surface);
	return cairo;
}


void WaylandCairoWindow::resize(int32_t new_width, int32_t new_height, struct wl_array* states)
{
	if (new_width <= 0 || new_height <= 0)
		return;

	auto contents_width = new_width;
	auto contents_height = new_height;
	if (!server_side_decorations) {
		contents_width -= style.frame_left + style.frame_right;
		contents_height -= style.frame_top + style.frame_bottom;
		}
	if (contents_width != width || contents_height != height) {
		width = contents_width;
		height = contents_height;
		if (widget)
			widget->layout();
		layout_frame_buttons();
		}

	// Get the states.
	is_maximized = is_fullscreen = is_focused = is_resizing = false;
	// wl_array_for_each() doesn't work in C++!!!!
	auto p = (uint32_t*) states->data;
	auto end = (uint32_t*) ((const char*) states->data + states->size);
	for (; p < end; ++p) {
		switch (*p) {
			case XDG_TOPLEVEL_STATE_MAXIMIZED: is_maximized = true; break;
			case XDG_TOPLEVEL_STATE_FULLSCREEN: is_fullscreen = true; break;
			case XDG_TOPLEVEL_STATE_ACTIVATED: is_focused = true; break;
			case XDG_TOPLEVEL_STATE_RESIZING: is_resizing = true; break;
			}
		}
}


void WaylandCairoWindow::close_requested()
{
	// Default: just go ahead and close.
	delete this;
}


void WaylandCairoWindow::draw_frame()
{
	auto cairo = cairo_gui.cairo();

	auto frame_width = width + style.frame_left + style.frame_right;
	auto frame_height = height + style.frame_top + style.frame_bottom;

	// Clear the corners to alpha.
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
	auto frame_right_x = style.frame_left + frame_width;
	auto frame_bottom_y = style.frame_top + frame_height;
	cairo_set_source_rgba(cairo, 0, 1, 1, 0.0);
	cairo_rectangle(cairo, 0, 0, style.frame_left, style.frame_top);
	cairo_fill(cairo);
	cairo_rectangle(cairo, frame_right_x, 0, style.frame_right, style.frame_top);
	cairo_fill(cairo);
	cairo_rectangle(cairo, 0, frame_bottom_y, style.frame_left, style.frame_bottom);
	cairo_fill(cairo);
	cairo_rectangle(cairo, frame_right_x, frame_bottom_y, style.frame_right, style.frame_bottom);
	cairo_fill(cairo);
	cairo_restore(cairo);

	// Draw the frame.
	// Rounded rect.
	double corner_size = style.frame_left;
	cairo_new_sub_path(cairo);
	cairo_arc(cairo, width - corner_size, corner_size, corner_size, -M_PI / 2.0, 0);
	cairo_arc(cairo, width - corner_size, height - corner_size, corner_size, 0, M_PI / 2.0);
	cairo_arc(cairo, corner_size, height - corner_size, corner_size, M_PI / 2.0, M_PI);
	cairo_arc(cairo, corner_size, corner_size, corner_size, M_PI, 1.5 * M_PI);
	cairo_close_path(cairo);
	// Punch a hole for the contents (no need to touch those pixels yet).
	cairo_move_to(cairo, style.frame_left, style.frame_top);
	cairo_rel_line_to(cairo, 0, height);
	cairo_rel_line_to(cairo, width, 0);
	cairo_rel_line_to(cairo, 0, -height);
	cairo_close_path(cairo);
	cairo_set_source_rgb(cairo, style.frame_color.red, style.frame_color.green, style.frame_color.blue);
	cairo_fill(cairo);

	// Title.
	cairo_select_font_face(
		cairo,
		style.title_font ? style.title_font : cairo_gui.default_font(),
		CAIRO_FONT_SLANT_NORMAL, style.title_font_weight);
	cairo_set_font_size(cairo, style.title_font_size);
	cairo_text_extents_t text_extents;
	cairo_text_extents(cairo, "M", &text_extents);
	auto real_ascent = text_extents.height;
	cairo_text_extents(cairo, title.c_str(), &text_extents);
	cairo_font_extents_t font_extents;
	cairo_font_extents(cairo, &font_extents);
	cairo_set_source_rgb(cairo, style.title_color.red, style.title_color.green, style.title_color.blue);
	cairo_move_to(cairo, style.frame_left + (width - text_extents.width) / 2, (style.frame_top + real_ascent) / 2);
	cairo_show_text(cairo, title.c_str());

	// Buttons.
	for (auto& button: frame_buttons)
		button.draw(cairo);
}

void WaylandCairoWindow::frame_mouse_pressed(double x, double y, uint32_t serial)
{
	uint32_t resize_edges = 0;
	double frame_right_x = style.frame_left + width;
	double frame_bottom_y = style.frame_top + height;

	if (y < style.frame_top) {
		for (auto& button: frame_buttons) {
			if (button.rect.contains(x, y)) {
				tracking_frame_button = &button;
				button.state = FrameButton::Clicked;
				}
			}
		if (tracking_frame_button)
			redraw();
		else
			xdg_toplevel_move(xdg_toplevel, wayland_display.seat, serial);
		}
	else if (x < style.frame_left)
		resize_edges = (y >= frame_bottom_y ? XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT : XDG_TOPLEVEL_RESIZE_EDGE_LEFT);
	else if (x >= frame_right_x)
		resize_edges = (y >= frame_bottom_y ? XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT : XDG_TOPLEVEL_RESIZE_EDGE_RIGHT);
	else if (y >= frame_bottom_y)
		resize_edges = XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;

	if (resize_edges)
		xdg_toplevel_resize(xdg_toplevel, wayland_display.seat, serial, resize_edges);
}

void WaylandCairoWindow::frame_mouse_moved(double x, double y)
{
	if (tracking_frame_button) {
		auto new_state = (tracking_frame_button->rect.contains(x, y) ? FrameButton::Clicked : FrameButton::Idle);
		if (tracking_frame_button->state != new_state) {
			tracking_frame_button->state = new_state;
			redraw();
			}
		}

	int new_cursor = Widget::PointerCursor;
	double frame_right_x = style.frame_left + width;
	double frame_bottom_y = style.frame_top + height;
	if (y < style.frame_top) {}
	else if (x < style.frame_left)
		new_cursor = (y >= frame_bottom_y ? WaylandDisplay::ResizeBottomLeftCursor : WaylandDisplay::ResizeLeftCursor);
	else if (x >= frame_right_x)
		new_cursor = (y >= frame_bottom_y ? WaylandDisplay::ResizeBottomRightCursor : WaylandDisplay::ResizeRightCursor);
	else if (y >= frame_bottom_y)
		new_cursor = WaylandDisplay::ResizeBottomCursor;
	if (new_cursor != last_cursor) {
		last_cursor = new_cursor;
		wayland_display.use_cursor(new_cursor);
		}
}

void WaylandCairoWindow::frame_mouse_released(double x, double y)
{
	if (tracking_frame_button == nullptr)
		return;

	tracking_frame_button->state = FrameButton::Idle;
	bool closed = false;
	if (tracking_frame_button->rect.contains(x, y)) {
		switch (tracking_frame_button->type) {
			case FrameButton::Close:
				closed = true;
				break;
			case FrameButton::Maximize:
				if (!is_maximized)
					xdg_toplevel_set_maximized(xdg_toplevel);
				else
					xdg_toplevel_unset_maximized(xdg_toplevel);
				break;
			case FrameButton::Minimize:
				xdg_toplevel_set_minimized(xdg_toplevel);
				break;
			default: break;
			}
		}

	tracking_frame_button = nullptr;

	// Don't do this until the end of the function!
	if (closed)
		close_requested();
}

void WaylandCairoWindow::layout_frame_buttons()
{
	auto frame_width = width + style.frame_left + style.frame_right;
	auto x = frame_width - style.frame_button_margin - style.frame_button_size;
	auto y = (style.frame_top - style.frame_button_size) / 2;
	for (auto& button: frame_buttons) {
		button.rect = { x, y, style.frame_button_size, style.frame_button_size };
		x -= style.frame_button_size + style.frame_button_spacing;
		}
}


// ---------------- //


static const cairo_user_data_key_t shm_surface_data_key = {};

SHMBackend::~SHMBackend()
{
}


cairo_surface_t* SHMBackend::prepare(int width, int height)
{
	// Pick a buffer.
	cur_buffer = nullptr;
	if (!buffers[0].busy)
		cur_buffer = &buffers[0];
	else if (!buffers[1].busy)
		cur_buffer = &buffers[1];
	else
		return nullptr;

	// Resize it if needed.
	if (cur_buffer->width() != width || cur_buffer->height() != height) {
		cur_buffer->allocate(width, height);
		next_check_time = TimeSeconds::now() + TimeSeconds{ check_seconds, 0 };
		if (!cur_buffer->is_valid())
			cur_buffer = nullptr;
		}

	return
		(cur_buffer && cur_buffer->is_valid()) ?
		cairo_surface_reference(cur_buffer->cairo_surface) :
		nullptr;
}


void SHMBackend::swap()
{
	if (cur_buffer == nullptr)
		return;

	wl_surface_attach(wayland_surface, cur_buffer->wayland_buffer, 0, 0);
	wl_surface_damage(wayland_surface, 0, 0, cur_buffer->width(), cur_buffer->height());
	wl_surface_commit(wayland_surface);

	cur_buffer->busy = true;
	cur_buffer = nullptr;
}


void SHMBackend::idle(int width, int height)
{
	next_check_time.clear();

	for (int which_buffer = 0; which_buffer < 2; ++which_buffer) {
		auto buffer = &buffers[which_buffer];
		if (!buffer->is_valid())
			continue;
		if (buffer->busy) {
			// Wayland tends to hold on to the buffer, so this is normal.
			continue;
			next_check_time = TimeSeconds::now() + TimeSeconds{ check_seconds, 0 };
			break;
			}
		if (buffer->is_oversize(width, height))
			buffer->deallocate();
		}
}


SHMBackend::Pool::Pool(size_t size)
{
	// Make the temporary file.
	auto runtime_dir = getenv("XDG_RUNTIME_DIR");
	if (runtime_dir == nullptr)
		return;
	auto path_template = std::string(runtime_dir) + "/cairo-widgets-shared-XXXXXX";
	std::vector<char> path(path_template.size() + 1);
	memcpy(path.data(), path_template.data(), path_template.size() + 1);
	int fd = mkstemp(path.data());
	if (fd < 0)
		return;
	unlink(path.data());
	auto result = ftruncate(fd, size);
	if (result < 0) {
		close(fd);
		return;
		}

	// Mmap it.
	data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		data = nullptr;
		close(fd);
		return;
		}

	// Make the Wayland pool.
	wayland_pool = wl_shm_create_pool(wayland_display.shm, fd, size);
	if (wayland_pool == nullptr)
		return;

	this->size = size;
}


SHMBackend::Pool::~Pool()
{
	if (data)
		munmap(data, size);
	if (wayland_pool)
		wl_shm_pool_destroy(wayland_pool);
}


void SHMBackend::Buffer::allocate(int width, int height)
{
	// Delete everything except the pool.
	if (cairo_surface)
		cairo_surface_destroy(cairo_surface);

	// Pool.
	// If the pool is already big enough, we won't reallocate it.  This means
	// using extra shared memory in some circumstances, but during resizing,
	// doing too many reallocations can end up crashing Wayland.  Probably
	// Wayland can't keep up with deleting the shared memory for pools we've
	// already released.  Only growing pools, not shrinking them, largely
	// prevents that problem.
	size_t stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	auto needed_size = stride * height;
	if (pool == nullptr || pool->size < needed_size) {
		if (pool)
			delete pool;
		pool = new Pool(stride * height);
		if (!pool->is_valid()) {
			delete pool;
			pool = nullptr;
			return;
			}
		}

	// Cairo surface.
	cairo_surface =
		cairo_image_surface_create_for_data(
			(unsigned char*) pool->data, CAIRO_FORMAT_ARGB32,
			width, height, stride);
	if (cairo_surface == nullptr) {
		delete pool;
		pool = nullptr;
		return;
		}
	cairo_surface_set_user_data(
		cairo_surface,
		&shm_surface_data_key, this,
		[](void* p) { ((SHMBackend::Buffer*) p)->cairo_surface_destroyed(); });

	// Wayland buffer.
	wayland_buffer =
		wl_shm_pool_create_buffer(
			pool->wayland_pool, 0,
			width, height, stride,
			WL_SHM_FORMAT_ARGB8888);
	if (wayland_buffer == nullptr) {
		deallocate();
		return;
		}
	static const struct wl_buffer_listener wayland_buffer_listener = {
		[](void* data, struct wl_buffer* buffer) { ((Buffer*) data)->wayland_release(); },
		};
	wl_buffer_add_listener(wayland_buffer, &wayland_buffer_listener, this);
}


void SHMBackend::Buffer::deallocate()
{
	if (cairo_surface)
		cairo_surface_destroy(cairo_surface);
	cairo_surface = nullptr;
	delete pool;
	pool = nullptr;
}


bool SHMBackend::Buffer::is_oversize(int width, int height)
{
	size_t stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	auto needed_size = stride * height;
	return pool != nullptr && pool->size > needed_size;
}


void SHMBackend::Buffer::cairo_surface_destroyed()
{
	wl_buffer_destroy(wayland_buffer);
	wayland_buffer = nullptr;
	cairo_surface = nullptr;
}


void SHMBackend::Buffer::wayland_release()
{
	busy = false;
}


/* --------------------- */


void WaylandCairoWindow::FrameButton::draw(cairo_t* cairo)
{
	const auto& style = WaylandCairoWindow::style;

	// Button frame.
	cairo_rectangle(cairo, 0, 0, rect.width, rect.height);
	Color color = (state == Idle ? style.frame_button_idle_color : style.frame_button_clicked_color);
	cairo_set_source_rgba(cairo, color.red, color.green, color.blue, color.alpha);
	cairo_fill_preserve(cairo);
	cairo_set_line_width(cairo, style.frame_button_border_size);
	color = style.frame_button_border_color;
	cairo_set_source_rgba(cairo, color.red, color.green, color.blue, color.alpha);
	cairo_stroke(cairo);

	// Contents.
	color = style.frame_button_icon_color;
	cairo_set_source_rgba(cairo, color.red, color.green, color.blue, color.alpha);
	cairo_set_line_width(cairo, style.frame_button_icon_line_width);
	auto icon_left = style.frame_button_icon_inset;
	auto icon_top = style.frame_button_icon_inset;
	auto icon_right = rect.width - style.frame_button_icon_inset;
	auto icon_bottom = rect.height - style.frame_button_icon_inset;
	switch (type) {
		case Close:
			cairo_move_to(cairo, icon_left, icon_top);
			cairo_line_to(cairo, icon_right, icon_bottom);
			cairo_stroke(cairo);
			cairo_move_to(cairo, icon_left, icon_bottom);
			cairo_line_to(cairo, icon_right, icon_top);
			cairo_stroke(cairo);
			break;
		case Minimize:
			cairo_move_to(cairo, icon_left, icon_bottom);
			cairo_line_to(cairo, icon_right, icon_bottom);
			cairo_stroke(cairo);
			break;
		case Maximize:
			cairo_rectangle(cairo, icon_left, icon_top, icon_right - icon_left, icon_bottom - icon_top);
			cairo_stroke(cairo);
			break;
		default: break;
		}
}



