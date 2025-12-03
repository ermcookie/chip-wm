#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <stdio.h>

typedef struct Key {
    unsigned int modifier;
    KeySym keysym;
    void (*function)(XEvent *event, char *command);
    char *command;
} Key;

typedef void (*Events)(XEvent *event);

/* Function Prototypes */
void launch(XEvent *event, char *command);
void destroy(XEvent *event, char *command);
void closedpy(XEvent *, char *);
void refresh(XEvent *event, char *command);
void focus(XEvent *event, char *command);
void configure(XEvent *event);
void enter(XEvent *event);
void key(XEvent *event);
void map(XEvent *event);
int ignore(Display *display, XErrorEvent *event);
void grab(void);
void size(void);
void scan(void);
void loop(void);
/* Globals */
static Display *display;
static Window root;
static int screen, width, height;
static unsigned int numlockmask = 0;
static unsigned int lockmask = LockMask;

/* Key Bindings */
static Key keys[] = {
    { Mod4Mask, XK_Return, launch, "xterm" },
    { Mod4Mask, XK_d, launch, "dmenu_run" },
    { Mod4Mask, XK_b, launch, "firefox" },
    { Mod4Mask, XK_p, launch, "scr" },
    { Mod4Mask | ShiftMask, XK_e, destroy, 0 },
    { Mod4Mask | ShiftMask, XK_q, closedpy, "display"},
    { Mod4Mask | ShiftMask, XK_r, refresh, 0 },
    { Mod4Mask, XK_Right, focus, "next" },
    { Mod4Mask, XK_Left, focus, "prev" },
    { 0, XF86XK_AudioMute, launch, "pamixer -t" },
    { 0, XF86XK_AudioLowerVolume, launch, "pamixer -d 5" },
    { 0, XF86XK_AudioRaiseVolume, launch, "pamixer -i 5" },
    { 0, XF86XK_MonBrightnessDown, launch, "xbacklight -dec 5" },
    { 0, XF86XK_MonBrightnessUp, launch, "xbacklight -inc 5" },
};

/* Event Handlers */
static const Events events[LASTEvent] = {
    [ConfigureRequest] = configure,
    [EnterNotify] = enter,
    [KeyPress] = key,
    [MapRequest] = map,
};

/* --------- NumLock Mask Helper ----------- */
unsigned int get_numlock_mask(Display *dpy) {
    XModifierKeymap *modmap = XGetModifierMapping(dpy);
    KeyCode numlock = XKeysymToKeycode(dpy, XStringToKeysym("Num_Lock"));
    unsigned int mask = 0;
    int mods[] = {Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask};
    for (int mod = 0; mod < 5; ++mod)
        for (int k = 0; k < modmap->max_keypermod; ++k)
            if (modmap->modifiermap[mod * modmap->max_keypermod + k] == numlock) mask = mods[mod];
    XFreeModifiermap(modmap);
    return mask;
}

void grab(void)
{
    unsigned int i;
    numlockmask = get_numlock_mask(display);
    unsigned int masks[] = {0, lockmask, numlockmask, numlockmask | lockmask};
    for (i = 0; i < sizeof(keys) / sizeof(Key); i++) {
        int keycode = XKeysymToKeycode(display, keys[i].keysym);
        for (int j = 0; j < 4; j++)
            XGrabKey(display, keycode, keys[i].modifier | masks[j], root, True, GrabModeAsync, GrabModeAsync);
    }
}

void size(void)
{
    XWindowAttributes attributes;
    if (XGetWindowAttributes(display, root, &attributes)) {
        width = attributes.width;
        height = attributes.height;
    } else {
        width = XDisplayWidth(display, screen);
        height = XDisplayHeight(display, screen);
    }
}

void scan()
{
    unsigned int i, n;
    Window r, p, *c;
    if (XQueryTree(display, root, &r, &p, &c, &n)) {
        for (i = 0; i < n; i++)
            XMoveResizeWindow(display, c[i], 0, 0, width, height);
        if (c)
            XFree(c);
    }
}

void loop(void)
{
    XEvent event;
    while (1 && !XNextEvent(display, &event))
        if (events[event.type])
            events[event.type](&event);
}

void enter(XEvent *event)
{
    Window window = event->xcrossing.window;
    XSetInputFocus(display, window, RevertToParent, CurrentTime);
    XRaiseWindow(display, window);
}

void configure(XEvent *event)
{
    XConfigureRequestEvent *request = &event->xconfigurerequest;
    XWindowChanges changes = {
        .x = request->x,
        .y = request->y,
        .width = request->width,
        .height = request->height,
        .border_width = request->border_width,
        .sibling = request->above,
        .stack_mode = request->detail,
    };
    XConfigureWindow(display, request->window, request->value_mask, &changes);
}

void key(XEvent *event)
{
    unsigned int i;
    KeySym keysym = XkbKeycodeToKeysym(display, event->xkey.keycode, 0, 0);
    unsigned int eventmask = event->xkey.state & ~(numlockmask | lockmask);

    for (i = 0; i < sizeof(keys) / sizeof(Key); i++)
        if (keysym == keys[i].keysym && eventmask == keys[i].modifier)
            keys[i].function(event, keys[i].command);
}

void map(XEvent *event)
{
    Window window = event->xmaprequest.window;
    XWindowChanges changes = { .border_width = 0 };

    XSelectInput(display, window, StructureNotifyMask | EnterWindowMask);
    XConfigureWindow(display, window, CWBorderWidth, &changes);
    XMoveResizeWindow(display, window, 0, 0, width, height);
    XMapWindow(display, window);

}
void focus(XEvent *event, char *command)
{
    (void)event;
    int next = command[0] == 'n';
    XCirculateSubwindows(display, root, next ? RaiseLowest : LowerHighest);
}
void launch(XEvent *event, char *command)
{
    (void)event;
    if (!command) return;
    if (fork() == 0) {
        if (fork() == 0) {
            if (display)
                close(XConnectionNumber(display));
            setsid();
            execl("/bin/sh", "sh", "-c", command, (char *)NULL);
            exit(1);
        }
        else {
            exit(0);
        }
    }
}

void destroy(XEvent *event, char *command)
{
    (void)command;
    Window w = event->xkey.subwindow;
    if (!w) {
        Window focused;
        int revert;
        XGetInputFocus(display, &focused, &revert);
        w = focused;
    }
    if (w != None && w != root)
        XKillClient(display, w);
}

void closedpy(XEvent *, char *)
{
        XDestroyWindow(display, root);
        XCloseDisplay(display);
}

void refresh(XEvent *event, char *command)
{
    (void)event;
    (void)command;
    size();
    scan();
}

int ignore(Display *display, XErrorEvent *event)
{
    (void)display;
    (void)event;
    return 0;
}

void set_window_manager_name(Display *display, Window root) {
    const char *wm_name = "chip-wm";

    // Set window manager name for legacy and modern tools
    XStoreName(display, root, wm_name);
    Atom net_wm_name = XInternAtom(display, "_NET_WM_NAME", False);
    Atom utf8_string = XInternAtom(display, "UTF8_STRING", False);
    XChangeProperty(display, root, net_wm_name, utf8_string, 8, PropModeReplace,
                    (unsigned char *)wm_name, strlen(wm_name));
    XFlush(display);
}

int main(void)
{
    if (!(display = XOpenDisplay(NULL)))
        exit(1);

    signal(SIGCHLD, SIG_IGN);
    XSetErrorHandler(ignore);

    screen = XDefaultScreen(display);
    root = XDefaultRootWindow(display);

    set_window_manager_name(display, root);

    XSelectInput(display, root, SubstructureRedirectMask);
    XDefineCursor(display, root, XCreateFontCursor(display, 68)); // left_ptr

    size();
    grab();
    scan();
    loop();
}
