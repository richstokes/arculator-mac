#include <sys/time.h>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/rawbmp.h>
#endif
#ifdef __APPLE__
#include <wx/display.h>
#endif

#include <SDL.h>

#include "wx-app.h"
#include "wx-mainwindow.h"
#include "wx_keytable.h"

extern "C"
{
	#include "arc.h"
	#include "disc.h"
	#include "ioc.h"
	#include "plat_input.h"
	#include "plat_video.h"
	#include "vidc.h"
	#include "video.h"
}

static int vidx = 0, vidy = 0;

extern "C" BITMAP *wxbuffer;

static SDL_mutex *main_thread_mutex = NULL;
static SDL_Thread *main_thread = NULL;

static volatile int quited = 0;
static volatile int pause_main_thread = 0;


class MyPlotEvent: public wxCommandEvent
{
public:
	MyPlotEvent(wxEventType eventType, int src_x, int src_y, int src_w, int src_h, int dblscan)
		: wxCommandEvent(eventType, 0),
		  src_x(src_x),
		  src_y(src_y),
		  src_w(src_w),
		  src_h(src_h),
		  dblscan(dblscan)
	{
	}

	virtual wxEvent *Clone() const { return new MyPlotEvent(*this); }

	int src_x, src_y, src_w, src_h, dblscan;
};

typedef void (wxEvtHandler::*MyPlotEventFunction)(MyPlotEvent&);

#define MyPlotEventHandler(func) wxEVENT_HANDLER_CAST(MyPlotEventFunction, func)

#define EVT_PLOT(func) \
    wx__DECLARE_EVT1(myEVT_PLOT, wxID_ANY, MyPlotEventHandler(func))

wxDEFINE_EVENT(myEVT_PLOT, MyPlotEvent);

class MyResizeEvent: public wxCommandEvent
{
public:
	MyResizeEvent(int video_width, int video_height, int window_width, int window_height)
		: wxCommandEvent(myEVT_RESIZE),
		  video_width(video_width),
		  video_height(video_height),
		  window_width(window_width),
		  window_height(window_height)
	{
	}

	virtual wxEvent *Clone() const { return new MyResizeEvent(*this); }

	int video_width, video_height;
	int window_width, window_height;
};

wxDEFINE_EVENT(myEVT_RESIZE, MyResizeEvent);
wxDEFINE_EVENT(myEVT_TITLE, wxThreadEvent);


void MainCanvas::OnPaint(wxPaintEvent &event)
{
	wxPaintDC dc(this);

	if (video_scale == 1)
		dc.DrawBitmap(bmp, wxPoint(0, 0));
	else
	{
		wxSize client_size = GetClientSize();
		wxMemoryDC temp_dc;
		temp_dc.SelectObjectAsSource(bmp);
		dc.StretchBlit(0, 0, client_size.x, client_size.y, &temp_dc, 0, 0, vidx, vidy);
	}
}

void MainCanvas::OnMyPlot(MyPlotEvent &event)
{
	wxNativePixelData data(bmp);
	wxNativePixelData::Iterator p(data);

	if (!wxbuffer)
		return;

	//printf("OnMyPlot - %i,%i  %i,%i  %i\n", event.src_x, event.src_y, event.src_w, event.src_h, event.dblscan);

	int h = event.dblscan ? event.src_h * 2 : event.src_h;

	for (int y = 0; y < h; y++)
	{
		wxNativePixelData::Iterator rowStart = p;
		uint32_t *src_p;

		if (event.dblscan)
			src_p = &((uint32_t *)wxbuffer->line[(y + event.src_y) / 2])[event.src_x];
		else
			src_p = &((uint32_t *)wxbuffer->line[y + event.src_y])[event.src_x];

		for (int x = 0; x < event.src_w; x++)
		{
			p.Red() = (src_p[x] >> 16) & 0xff;
			p.Green() = (src_p[x] >> 8) & 0xff;
			p.Blue() = src_p[x] & 0xff; //x & 255;
			p++;
		}

		p = rowStart;
   		p.OffsetY(data, 1);
	}

	Refresh();
	Update();
}

void MainCanvas::OnMotion(wxMouseEvent &event)
{
	if (mousecapture)
	{
		wxPoint pos = event.GetPosition();
		//printf("OnMotion: %i,%i\n", pos.x, pos.y);

		if (pos.x != GetClientSize().x / 2 || pos.y != GetClientSize().y / 2)
		{
			dx += (pos.x - last_mouse_pos.x);
			dy += (pos.y - last_mouse_pos.y);

			WarpPointer(GetClientSize().x / 2, GetClientSize().y / 2);

			last_mouse_pos.x = GetClientSize().x / 2;
			last_mouse_pos.y = GetClientSize().y / 2;
		}


	}
}

void MainCanvas::OnKeyDown(wxKeyEvent &event)
{
	int kc = event.GetKeyCode();
//	printf("keycode %i\n", kc);

#ifdef __APPLE__
	if ((kc == WXK_BACK || kc == WXK_DELETE) && event.CmdDown())
#else
	if (kc == WXK_END && event.GetModifiers() == wxMOD_CONTROL)
#endif
	{
//		printf("CTRL+END\n");
		if (fullscreen)
			static_cast<MainFrame *>(wxGetTopLevelParent(this))->LeaveFullScreen();
		if (mousecapture)
		{
			mouse_capture_disable();
			ReleaseMouse();
			SetCursor(*wxSTANDARD_CURSOR);
			mousecapture = 0;
			updatemips = 1;
		}
	}
	else
	{
		for (int i = 0; i < (sizeof(wxk_to_key) / sizeof(wxk_to_key[0])); i++)
		{
			if (wxk_to_key[i].wxk == kc)
			{
				key[wxk_to_key[i].key] = 1;
				break;
			}
		}
	}
}

void MainCanvas::OnKeyUp(wxKeyEvent &event)
{
	int kc = event.GetKeyCode();
//	printf("up keycode %i\n", kc);

	for (int i = 0; i < (sizeof(wxk_to_key) / sizeof(wxk_to_key[0])); i++)
	{
		if (wxk_to_key[i].wxk == kc)
		{
			key[wxk_to_key[i].key] = 0;
			break;
		}
	}
}

wxBEGIN_EVENT_TABLE(MainCanvas, wxWindow)
	EVT_PAINT(MainCanvas::OnPaint)
	EVT_PLOT(MainCanvas::OnMyPlot)
	EVT_MOTION(MainCanvas::OnMotion)
	EVT_LEFT_DOWN(MainCanvas::OnLeftDown)
	EVT_LEFT_DCLICK(MainCanvas::OnLeftDown)
	EVT_LEFT_UP(MainCanvas::OnLeftUp)
	EVT_MIDDLE_DOWN(MainCanvas::OnMiddleDown)
	EVT_MIDDLE_DCLICK(MainCanvas::OnMiddleDown)
	EVT_MIDDLE_UP(MainCanvas::OnMiddleUp)
	EVT_RIGHT_DOWN(MainCanvas::OnRightDown)
	EVT_RIGHT_DCLICK(MainCanvas::OnRightDown)
	EVT_RIGHT_UP(MainCanvas::OnRightUp)
    	EVT_KEY_DOWN(MainCanvas::OnKeyDown)
	EVT_KEY_UP(MainCanvas::OnKeyUp)
wxEND_EVENT_TABLE()


MainFrame::MainFrame(Frame *parent, const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(NULL, wxID_ANY, title, pos, size, wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS),
	  canvas(NULL), parent(parent), initial_size_set(false)
{
	canvas = new MainCanvas(this, "editor", wxDefaultPosition, wxDefaultSize);
	SetMenuBar((wxMenuBar *)main_menu);
	Bind(myEVT_RESIZE, &MainFrame::OnResize, this);
	Bind(myEVT_TITLE, &MainFrame::OnTitle, this);
}

MainFrame::~MainFrame()
{
	delete canvas;
}

void MainFrame::OnClose(wxCloseEvent& event)
{
	arc_stop_emulation();
}

void MainFrame::OnMenu(wxCommandEvent& event)
{
	OnMenuCommandCommon(event, this);
}

void MainFrame::OnResize(MyResizeEvent& event)
{
	// VIDC reports incomplete dimensions while the first mode is being set.
	if (event.video_width <= 0 || event.video_height <= 0 ||
	    event.window_width <= 0 || event.window_height <= 0)
		return;

	vidx = event.video_width;
	vidy = event.video_height;
	if (!fullscreen)
	{
#ifdef __APPLE__
		// Cocoa can move the frame when its client size changes. Avoid repeated
		// resizes and preserve the current position across genuine mode changes.
		wxSize requested_size(event.window_width, event.window_height);
		if (requested_size != last_window_size)
		{
			wxPoint window_position = GetPosition();
			SetClientSize(requested_size);
			last_window_size = requested_size;
			if (initial_size_set)
				Move(window_position);
		}
		if (!initial_size_set)
		{
			// Place the first usable mode in the centre of its current display.
			int display_index = wxDisplay::GetFromWindow(this);
			if (display_index == wxNOT_FOUND)
				display_index = 0;
			wxRect display_area = wxDisplay(display_index).GetClientArea();
			Move(display_area.x + (display_area.width - event.window_width) / 2,
			     display_area.y + (display_area.height - event.window_height) / 2);
			initial_size_set = true;
		}
#else
		SetClientSize(event.window_width, event.window_height);
#endif
	}
}

void MainFrame::OnTitle(wxThreadEvent& event)
{
	SetLabel(event.GetString());
}

void MainFrame::EnterFullScreen()
{
	if (fullscreen)
		return;

	windowed_rect = GetRect();
	fullscreen = 1;
	ShowFullScreen(true);
	canvas->SetFocus();
}

void MainFrame::LeaveFullScreen()
{
	if (!fullscreen)
		return;

	ShowFullScreen(false);
	fullscreen = 0;
	SetSize(windowed_rect);
}

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_CLOSE(MainFrame::OnClose)

    EVT_MENU(wxID_ANY, MainFrame::OnMenu)
wxEND_EVENT_TABLE()

static MainFrame *arcFrame = NULL;

extern "C" int arc_main_thread(void *p)
{
	rpclog("Arculator startup\n");

	arc_init();

	if (!video_renderer_init(NULL))
	{
		fatal("Video renderer init failed");
	}

	arc_update_menu();

	struct timeval tp;
	time_t last_seconds = 0;

	while (!quited)
	{
		LOG_EVENT_LOOP("event loop\n");
		if (gettimeofday(&tp, NULL) == -1)
		{
			perror("gettimeofday");
			fatal("gettimeofday failed\n");
		}
		else if (!last_seconds)
		{
			last_seconds = tp.tv_sec;
			rpclog("start time = %d\n", last_seconds);
		}
		else if (last_seconds != tp.tv_sec)
		{
			updateins();
			last_seconds = tp.tv_sec;
		}


		// Run for 10 ms of processor time
		SDL_LockMutex(main_thread_mutex);
		if (!pause_main_thread)
			arc_run();
		SDL_UnlockMutex(main_thread_mutex);

		// Sleep to make it up to 10 ms of real time
		static Uint32 last_timer_ticks = 0;
		static int timer_offset = 0;
		Uint32 current_timer_ticks = SDL_GetTicks();
		Uint32 ticks_since_last = current_timer_ticks - last_timer_ticks;
		last_timer_ticks = current_timer_ticks;
		timer_offset += 10 - (int)ticks_since_last;
		// rpclog("timer_offset now %d; %d ticks since last; delaying %d\n", timer_offset, ticks_since_last, 10 - ticks_since_last);
		if (timer_offset > 100 || timer_offset < -100)
		{
			timer_offset = 0;
		}
		else if (timer_offset > 0)
		{
			SDL_Delay(timer_offset);
		}

		if (updatemips)
		{
			char s[80];

#ifdef __APPLE__
			snprintf(s, sizeof(s), "Arculator %s - %i%% - %s", VERSION_STRING, inssec, mousecapture ? "Press CMD-BACKSPACE to release mouse" : "Click to capture mouse");
#else
			snprintf(s, sizeof(s), "Arculator %s - %i%% - %s", VERSION_STRING, inssec, mousecapture ? "Press CTRL-END to release mouse" : "Click to capture mouse");
#endif
			vidc_framecount = 0;
			if (!fullscreen)
			{
				wxThreadEvent *event = new wxThreadEvent(myEVT_TITLE);
				event->SetString(s);
				wxQueueEvent(arcFrame, event);
			}
			updatemips=0;
		}
	}

	arc_close();

	return 0;
}

extern "C" void updatewindowsize(int x, int y)
{
	int window_width = (x*(video_scale + 1)) / 2;
	int window_height = (y*(video_scale + 1)) / 2;

	MyResizeEvent *event = new MyResizeEvent(x, y, window_width, window_height);
	wxQueueEvent(arcFrame, event);
}

extern "C" void arc_start_main_thread(void *wx_window, void *wx_menu)
{
	quited = 0;
	pause_main_thread = 0;

	main_thread_mutex = SDL_CreateMutex();
	arcFrame = new MainFrame((Frame *)wx_window, "Arculator", wxPoint(50, 50),
				wxSize(768, 576));
	arcFrame->Show(TRUE);
	main_thread = SDL_CreateThread(arc_main_thread, "Main Thread", (void *)NULL);
}

extern "C" void arc_stop_main_thread()
{
	quited = 1;
	SDL_WaitThread(main_thread, NULL);
	SDL_DestroyMutex(main_thread_mutex);
	main_thread_mutex = NULL;

	delete arcFrame;
}

extern "C" void arc_pause_main_thread()
{
	SDL_LockMutex(main_thread_mutex);
	pause_main_thread = 1;
	SDL_UnlockMutex(main_thread_mutex);
}

extern "C" void arc_resume_main_thread()
{
	SDL_LockMutex(main_thread_mutex);
	pause_main_thread = 0;
	SDL_UnlockMutex(main_thread_mutex);
}

extern "C" void arc_do_reset()
{
	debugger_start_reset();
	SDL_LockMutex(main_thread_mutex);
	arc_reset();
	SDL_UnlockMutex(main_thread_mutex);
	debugger_end_reset();
}

extern "C" void arc_disc_change(int drive, char *fn)
{
	int is_indebug = indebug;

	if (!is_indebug)
		SDL_LockMutex(main_thread_mutex);

	disc_close(drive);
	strcpy(discname[drive], fn);
	disc_load(drive, discname[drive]);
	ioc_discchange(drive);

	if (!is_indebug)
		SDL_UnlockMutex(main_thread_mutex);
}

extern "C" void arc_disc_eject(int drive)
{
	int is_indebug = indebug;

	if (!is_indebug)
		SDL_LockMutex(main_thread_mutex);

	ioc_discchange(drive);
	disc_close(drive);
	discname[drive][0] = 0;

	if (!is_indebug)
		SDL_UnlockMutex(main_thread_mutex);
}

extern "C" void arc_renderer_reset()
{
	//win_renderer_reset = 1;
}

extern "C" void arc_set_display_mode(int new_display_mode)
{
	int is_indebug = indebug;

	if (!is_indebug)
		SDL_LockMutex(main_thread_mutex);

	display_mode = new_display_mode;
	clearbitmap();
	setredrawall();

	if (!is_indebug)
		SDL_UnlockMutex(main_thread_mutex);
}

extern "C" void arc_set_dblscan(int new_dblscan)
{
	int is_indebug = indebug;

	if (!is_indebug)
		SDL_LockMutex(main_thread_mutex);

	dblscan = new_dblscan;
	clearbitmap();

	if (!is_indebug)
		SDL_UnlockMutex(main_thread_mutex);
}

extern "C" void arc_set_resizeable()
{
	//win_dosetresize = 1;
}

extern "C" void arc_enter_fullscreen()
{
	arcFrame->EnterFullScreen();
}

extern "C" void video_renderer_present(int src_x, int src_y, int src_w, int src_h, int dblscan)
{
	//LOG_VIDEO_FRAMES
//	printf("video_renderer_present: %d,%d + %d,%d\n", src_x, src_y, src_w, src_h);
	MyPlotEvent event(myEVT_PLOT, src_x, src_y, src_w, src_h, dblscan);
	event.SetEventObject(arcFrame->GetCanvas());
	wxPostEvent(arcFrame->GetCanvas(), event);
}

extern "C" void get_mouse_deltas(int *x, int *y, int *b)
{
	MainCanvas *canvas = arcFrame->GetCanvas();

	*x = canvas->dx;
	*y = canvas->dy;
	canvas->dx = 0;
	canvas->dy = 0;

	*b = canvas->b;
//	printf("get_mouse_deltas: x=%i y=%i\n", *x, *y);
}
