#include "common/runtime.h"
#include "modules/window.h"

SDL_Window * WINDOW;
SDL_Renderer * RENDERER;

void Window::Initialize()
{
    Uint32 windowFlags = SDL_WINDOW_FULLSCREEN;
    WINDOW = SDL_CreateWindow(NULL, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, windowFlags);

    if (!WINDOW)
        SDL_Quit();

    Uint32 rendererFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    RENDERER = SDL_CreateRenderer(WINDOW, 0, rendererFlags);

    if (!RENDERER)
        SDL_Quit();

    SDL_SetRenderDrawBlendMode(RENDERER, SDL_BLENDMODE_BLEND);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
}

SDL_Renderer * Window::GetRenderer()
{
    return RENDERER;
}

void Window::Exit()
{
    SDL_DestroyRenderer(RENDERER);
    SDL_DestroyWindow(WINDOW);
}

//Löve2D Functions
int Window::SetMode(lua_State * L)
{
    return 0;
}

int Window::GetFullscreenModes(lua_State * L)
{
    lua_createtable(L, 1, 0);
    lua_pushnumber(L, 1);

    lua_createtable(L, 0, 2);

    //Inner table attributes
    lua_pushnumber(L, 1280);
    lua_setfield(L, -2, "width");
    
    lua_pushnumber(L, 720);
    lua_setfield(L, -2, "height");

    //End table attributes

    lua_settable(L, -3);

    return 1;
}

//End Löve2D Functions

int Window::Register(lua_State * L)
{
    luaL_Reg reg[] = 
    {
        { "setMode",            SetMode            },
        { "getFullscreenModes", GetFullscreenModes },
        { 0, 0 }
    };

    luaL_newlib(L, reg);
    
    return 1;
}