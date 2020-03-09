#include <conio.h>
#include <mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <file.h>
#include <graph.h>
#include <time.h>
#include <taskscan.h>

#include "SDL2.h"



SDL_Window *SDL_CreateWindow(char *title, int x, int y,int  width,int  height, int flag){
    SDL_Window window={title,x,y,width,height,flag};
    return &window;
}
SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int  index,int flags){

    int pixels[window->w*window->h];

    SDL_Renderer render={&pixels,NULL,w,h};
    
    return &render;
}
void SDL_SetRenderDrawColor(SDL_Renderer* renderer,uint r,uint g,uint b, uint a){
    renderer->color.r=r;
    renderer->color.g=g;
    renderer->color.b=b;
    renderer->color.a=a;
}
void SDL_RenderDrawPoint(SDL_Renderer* renderer,int x,int y){
    uint color=(renderer->color.a << 24) | (renderer->color.r << 16) | 
    (renderer->color.g << 8) | (renderer->color.b);
   
    GUI_DrawPixel(x,y,color)
}
void SDL_RenderPresent(SDL_renderer *renderer){
    GUI_Update(0,0,renderer->w,renderer->h);
}

int main(int argc, char *argv[])
{
    SDL_Window *win = NULL;
    SDL_Renderer *renderer= NULL;

 win = SDL_CreateWindow("Hello World", 0, 0, 32, 32, 0);

    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    SDL_SetRenderDrawColor(renderer, r, g, b, 255);

	SDL_RenderDrawPoint(renderer, x, y);
    printf("hello, sdl2!\n");
	return 0;
}
