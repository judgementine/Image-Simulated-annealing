#include <iostream>
#include <assert.h>
#include <time.h>
using namespace std;

//include SDL header
#include "SDL2-2.0.8\include\SDL.h"
#include "SDL2-2.0.8\include\SDL_image.h"

//load libraries
#pragma comment(lib,"SDL2-2.0.8\\lib\\x86\\SDL2.lib")
#pragma comment(lib,"SDL2-2.0.8\\lib\\x86\\SDL2main.lib")
#pragma comment(lib,"SDL2-2.0.8\\lib\\x86\\SDL2_image.lib")

#pragma comment(linker,"/subsystem:console")

struct Pixel
{
	unsigned char r, g, b;
};

struct Square
{
	int x, y, w, h;

	Pixel color;

};

namespace Game
{
	int CHEAT = 1;//0 NO CHEAT, 1 SOME CHEAT, 2 FULL CHEAT

	SDL_Renderer *renderer = NULL;
	int screen_width = 800;
	int screen_height = 600;

	unsigned char prev_key_state[256];
	unsigned char *keys = NULL;
	SDL_Window *window = NULL;

	SDL_Surface *original;
	SDL_Surface *image;
	SDL_Surface *current_solution_surface;
	SDL_Surface *permuted_solution_surface;
	SDL_Surface *screen;

	Square * current_squares;
	Square * permuted_squares;
	

	//EXPERIMENT WITH THESE
	int n_boxes = 600;
	int min_size = 10;
	int max_size = 100;

	double current_error = 0.0;
	float temperature = 1.0;
	float position_permute_amount = 10.0;//<-- VERY IMPORTANT RANK 3
	float color_permute_amount = 10.0;//<-- VERY IMPORTANT RANK 3
	float size_permute_amount = 4.0;//<-- VERY IMPORTANT RANK 3
	float temperature_decay = 0.99;//<-- VERY IMPORTANT RANK 1
	int n_permute_same_temp = 15;//<-- VERY IMPORTANT RANK 2
	int n_permuted_boxes_per_iteration = n_boxes*0.1;//<-- VERY IMPORTANT RANK 2

	//EXPERIMENT WITH THIS
	void permute_Boxes(Square *s, int n_boxes, int n_permute)
	{
		for (int i = 0; i < n_permute; i++)
		{
			int k = rand() % n_boxes;

			//permute position
			float x_amount = position_permute_amount * (1.0 - 2.0*rand() / RAND_MAX);
			float y_amount = position_permute_amount * (1.0 - 2.0*rand() / RAND_MAX);

			s[k].x += x_amount;
			s[k].y += y_amount;

			if (s[k].x < 0) s[k].x = 0;
			if (s[k].y < 0) s[k].y = 0;
			if (s[k].x > screen_width - 1) s[k].x = screen_width - 1;
			if (s[k].y > screen_height - 1) s[k].y = screen_height - 1;

			//permute color
			float r = (float)s[k].color.r + color_permute_amount * (1.0 - 2.0*rand() / RAND_MAX);
			float g = (float)s[k].color.g + color_permute_amount * (1.0 - 2.0*rand() / RAND_MAX);
			float b = (float)s[k].color.b + color_permute_amount * (1.0 - 2.0*rand() / RAND_MAX);

			if (r < 0) r = 0;
			if (g < 0) g = 0;
			if (b < 0) b = 0;
			if (r > 255) r = 255;
			if (g > 255) g = 255;
			if (b > 255) b = 255;

			s[k].color.r = r;
			s[k].color.g = g;
			s[k].color.b = b;
			
			//permute size
			float w = (float)s[k].w + size_permute_amount * (1.0 - 2.0*rand() / RAND_MAX);
			float h = (float)s[k].h + size_permute_amount * (1.0 - 2.0*rand() / RAND_MAX);
			if (w < min_size) w = min_size;
			if (w > max_size) w = max_size;
			if (h < min_size) h = min_size;
			if (h > max_size) h = max_size;

			s[k].w = w;
			s[k].h = h;
		}
	}

	double difference(SDL_Surface * a, SDL_Surface * b)
	{
		double difference = 0.0;
		Pixel * o = (Pixel *)a->pixels;
		Pixel * p = (Pixel *)b->pixels;
		int size = a->w*a->h;
		for (int i = 0; i < size; i++)
		{
			int r = (int)o[i].r - (int)p[i].r;
			int g = (int)o[i].g - (int)p[i].g;
			int b = (int)o[i].b - (int)p[i].b;
			difference += r * r + g * g + b * b;
		}
		difference /= size;
		return difference;

	}

	void blend(SDL_Surface * a, Square * sq)
	{
		Pixel *p = (Pixel*)a->pixels;
		for (int i = 0; i < n_boxes; i++)
		{
			for (int y = 0; y < sq[i].h;y++)
			{
				int y_offset = (y + sq[i].y)*a->w;
				for (int x = 0; x < sq[i].w; x++)
				{
					int x_offset = sq[i].x + x;
					Pixel *s = &p[y_offset];
					s[x_offset].r = ((float)s[x_offset].r + (float)sq[i].color.r) / 2.0;
					s[x_offset].g = ((float)s[x_offset].g + (float)sq[i].color.g) / 2.0;
					s[x_offset].b = ((float)s[x_offset].b + (float)sq[i].color.b) / 2.0;
					
				}
			}
		}
	}


	void init(const char *filename)
	{
		SDL_Init(SDL_INIT_VIDEO);

		prev_key_state[256];
		keys = (unsigned char*)SDL_GetKeyboardState(NULL);

		window = SDL_CreateWindow(
			"SDL Image Simulated Annealing",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			screen_width, screen_height, SDL_WINDOW_SHOWN);

		renderer = SDL_CreateRenderer(window,-1, SDL_RENDERER_SOFTWARE);

		original = IMG_Load(filename);
		printf("bits: %d pitch: %d w: %d h: %d\n", original->format->BitsPerPixel, original->pitch, original->w, original->h);

		image = SDL_CreateRGBSurfaceWithFormat(0, screen_width, screen_height, 24, SDL_PIXELFORMAT_RGB24);
		SDL_BlitSurface(original, NULL, image, NULL);

		current_solution_surface = SDL_CreateRGBSurfaceWithFormat(0, screen_width, screen_width, 24, SDL_PIXELFORMAT_RGB24);

		permuted_solution_surface = SDL_CreateRGBSurfaceWithFormat(0, screen_width, screen_width, 24, SDL_PIXELFORMAT_RGB24);

		screen = SDL_GetWindowSurface(window);

		permuted_squares = (Square*)malloc(n_boxes * sizeof(Square));
		current_squares = (Square*)malloc(n_boxes * sizeof(Square));
		
		for (int i = 0; i < n_boxes; i++)
		{
			current_squares[i].w = Game::min_size + rand() % (Game::max_size - Game::min_size);
			current_squares[i].h = Game::min_size + rand() % (Game::max_size - Game::min_size);
			current_squares[i].x = rand() % (image->w - current_squares[i].w);
			current_squares[i].y = rand() % (image->h - current_squares[i].h);
			
			if (CHEAT == 0)
			{
				current_squares[i].color.r = rand() % 256;
				current_squares[i].color.g = rand() % 256;
				current_squares[i].color.b = rand() % 256;
			}
			else if(CHEAT == 1)
			{
				int x = rand() % image->w;
				int y = rand() % image->h;
				Pixel *s = (Pixel *)image->pixels;
				current_squares[i].color.r = s[y*image->w+x].r;
				current_squares[i].color.g = s[y*image->w + x].g;
				current_squares[i].color.b = s[y*image->w + x].b;
			}
			else if (CHEAT == 2)
			{
				Pixel *s = (Pixel *)image->pixels;
				current_squares[i].color.r = s[current_squares[i].y*image->w + current_squares[i].x].r;
				current_squares[i].color.g = s[current_squares[i].y*image->w + current_squares[i].x].g;
				current_squares[i].color.b = s[current_squares[i].y*image->w + current_squares[i].x].b;
			}
			
		}

		memset(current_solution_surface->pixels, 0, current_solution_surface->pitch*current_solution_surface->h);
		blend(current_solution_surface, current_squares);
		current_error = difference(image, current_solution_surface);
		
	}


	void update()
	{

		memcpy(prev_key_state, keys, 256);

		//consume all window events first
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				exit(0);
			}
		}

		for (int i = 0; i < Game::n_permute_same_temp; i++)
		{

			
			memcpy(permuted_squares, current_squares, sizeof(Square)*n_boxes);

			permute_Boxes(permuted_squares, n_boxes, n_permuted_boxes_per_iteration);

			memset(permuted_solution_surface->pixels, 0, permuted_solution_surface->pitch*permuted_solution_surface->h);
			blend(permuted_solution_surface, permuted_squares);

			double permuted_error = difference(image, permuted_solution_surface);

			float p = exp((current_error - permuted_error) / temperature);
			float r = (double)rand() / RAND_MAX;
			if (permuted_error < current_error)
			{
				memcpy(current_squares, permuted_squares, sizeof(Square)*n_boxes);
				current_error = permuted_error;
			}
			else if (r < p)
			{
				memcpy(current_squares, permuted_squares, sizeof(Square)*n_boxes);
				current_error = permuted_error;
			}

			//printf("temp: %f current error: %f permuted error: %f\n", temperature, current_error, permuted_error);
			
		}
		printf("temp: %f current error: %f\n", temperature, current_error);
		temperature *= temperature_decay;
		
		
	}


	void draw()
	{
		memset(current_solution_surface->pixels, 0, current_solution_surface->pitch*current_solution_surface->h);
		blend(current_solution_surface, current_squares);
		SDL_BlitScaled(current_solution_surface, NULL, screen, NULL);

		SDL_UpdateWindowSurface(window);

	}



}
int main(int argc, char **argv)
{

	srand(time(0));

	Game::init("test.png");


	for(;;)
	{
		Game::update();
		
		Game::draw();
	}



	return 0;
}