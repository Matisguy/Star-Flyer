#ifndef STARFLYER_IOS_PLATFORM_H
#define STARFLYER_IOS_PLATFORM_H

#include <SDL3/SDL.h>

/** Adds the UIKit menus over the game view and starts motion updates. */
void iosPlatformInstall(SDL_Window *window);

/** Stops motion updates while the app is in the background, and restarts them. */
void iosPlatformSetActive(bool active);

#endif // STARFLYER_IOS_PLATFORM_H
