#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> // For sleep()

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main(void) {
    Display *dpy;
    XWindowAttributes attr;
    XEvent ev;
    Window window, root, parent;
    Window *children;
    unsigned int num_children;
    int currentIndex = 0;

    // Start X display
    if (!(dpy = XOpenDisplay(NULL))) return EXIT_FAILURE;

    // Get dimensions of root window
    root = DefaultRootWindow(dpy);
    XGetWindowAttributes(dpy, root, &attr);

    // Create simple window
    window = XCreateSimpleWindow(dpy, root, 0, 0,
                                  attr.width, attr.height, 0,
                                  BlackPixel(dpy, 0), WhitePixel(dpy, 0));

    // Select input events
    XSelectInput(dpy, window, ExposureMask | KeyPressMask | ButtonPressMask);

    // Map window to display
    XMapWindow(dpy, window);

    // Query the window tree to get child window IDs
    if (XQueryTree(dpy, root, &root, &parent, &children, &num_children) == 0) {
        fprintf(stderr, "Failed to query window tree\n");
        XDestroyWindow(dpy, window);
        XCloseDisplay(dpy);
        return EXIT_FAILURE;
    }

    // Main loop
    while (1) {
        XNextEvent(dpy, &ev);
        switch (ev.type) {
            case Expose:
                // Handle expose events if needed
                break;

            case KeyPress: {
                KeySym keysym = XLookupKeysym(&ev.xkey, 0);
                int Ctrl = ev.xkey.state & ControlMask;
                int Alt = ev.xkey.state & Mod1Mask;
                int Mod = ev.xkey.state & Mod4Mask;

                if (Mod && keysym == XStringToKeysym("q")) {
			// Quit application
                    XDestroyWindow(dpy, window);
                    XFree(children); // Free children array
                    XCloseDisplay(dpy);
                    return EXIT_SUCCESS;
                }
                if (Mod && keysym == XStringToKeysym("Return")) {
                    // Launch Alacritty terminal
                    system("alacritty &");
                }
                if (Mod && keysym == XStringToKeysym("d")) {
                    // Launch Dmenu
                    system("dmenu_run &");
                }
		if (Mod && keysym == XStringToKeysym("l")) {
			XMoveWindow(dpy, window, 1, 0 );
		}

                // Cycle focus through windows
                if (Mod && keysym == XStringToKeysym("KP_Right") && num_children > 0) {
                    // Set focus to current window
                    XSetInputFocus(dpy, children[currentIndex], RevertToPointerRoot, CurrentTime);
                    XRaiseWindow(dpy, children[currentIndex]); // Optionally, raise window
                    XFlush(dpy);
                    printf("Focusing on window ID: %lu\n", (unsigned long)children[currentIndex]);

                    // Move to the next window
                    currentIndex = (currentIndex + 1) % num_children;
                }
		                if (Mod && keysym == XStringToKeysym("KP_Left") && num_children > 0) {
                    // Set focus to the current window
                    XSetInputFocus(dpy, children[currentIndex], RevertToPointerRoot, CurrentTime);
                    XRaiseWindow(dpy, children[currentIndex]); // Optionally, raise window
                    XFlush(dpy);
                    printf("Focusing on window ID: %lu\n", (unsigned long)children[currentIndex]);

                    // Move to the next window
                    currentIndex = (currentIndex - 1) % num_children;
                }
                break;
            }

            case ButtonPress:
                // Handle button press events here
                break;

            default:
                break;
        }

        // Optional: Add a delay for visual clarity when cycling through windows
        usleep(200000); // Sleep for 200 milliseconds
    }

    // Clean up
    XFree(children); // Free the array of child windows
    XDestroyWindow(dpy, window);
    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}

