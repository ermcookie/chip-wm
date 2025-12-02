#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    Display *dpy;
    XWindowAttributes attr;
    XEvent ev;
    Window window, root, parents;
    Window *children;
    unsigned int nchildren;
    int focus = 0;

    // Start X display
    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Failed to open display\n");
        return EXIT_FAILURE;
    }

    // Get dimensions of root window
    root = DefaultRootWindow(dpy);
    if (XQueryTree(dpy, root, &root, &parents, &children, &nchildren) == 0) {
        fprintf(stderr, "Failed to query window tree\n");
        XCloseDisplay(dpy);
        return EXIT_FAILURE;
    }

    window = root;
    XGetWindowAttributes(dpy, window, &attr);

    // Select input events
    XSelectInput(dpy, window, ExposureMask | KeyPressMask | ButtonPressMask);
    // Map window to display
    XMapWindow(dpy, window);

    // Event loop
    while (1) {
        XNextEvent(dpy, &ev);
        switch (ev.type) {
            case Expose:

                break;

            case KeyPress: {
                KeySym keysym = XLookupKeysym(&ev.xkey, 0);
                int Ctrl = ev.xkey.state & ControlMask;
                int Alt = ev.xkey.state & Mod1Mask;
                int Super = ev.xkey.state & Mod4Mask;

                if (Super && keysym == XStringToKeysym("q")) {
                    XDestroyWindow(dpy, window);
                    XCloseDisplay(dpy);
                    exit(EXIT_SUCCESS);
                }

                if (Super && keysym == XStringToKeysym("Return")) {
                    system("dbus-launch alacritty &");
                }
                if (Super && keysym == XStringToKeysym("Left")) {
                        if (nchildren != 0) {
                        focus = (focus - 1 );
                        XRaiseWindow(dpy, children[focus]);
                        XSetInputFocus(dpy, children[focus], RevertToParent, CurrentTime);
                        }
                }
                if (Super && keysym == XStringToKeysym("Right")) {
                        if (nchildren != 0) {
                        focus = (focus + 1 );
                        XRaiseWindow(dpy, children[focus]);
                        XSetInputFocus(dpy, children[focus], RevertToParent, CurrentTime);
                        }
                }
                break;
            }

            case ButtonPress:
                // Handle button press events here
                break;

            default:
                break;
        }
    }

    // Cleanup (not reached due to infinite loop)
    XFree(children); // Free the children array
    XDestroyWindow(dpy, window);
    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}
