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
	unsigned char r, g, b, a;
};

struct Square
{
	int x, y, w, h;

	Pixel color;

};

namespace Game
{
	SDL_Renderer *renderer = NULL;
	int screen_width = 800;
	int screen_height = 600;

	unsigned char prev_key_state[256];
	unsigned char *keys = NULL;
	SDL_Window *window = NULL;

	SDL_Surface *original;
	SDL_Surface *Image;
	SDL_Surface *scratch;
	SDL_Surface *tmp;
	SDL_Surface *screen;

	int *grid = NULL;
	int *px, *py;
	Pixel *color;
	int n_walkers = 100;
	Square * squares;
	int n_boxes = 1000;
	double fitness = 0.0;
	float temperature = 1;
	float permute_amount = 204.0;//<-- VERY IMPORTANT RANK 3
	float temperature_decay = 0.95;//<-- VERY IMPORTANT RANK 1
	int n_permute_same_temp = 80;//<-- VERY IMPORTANT RANK 2


	void permuteBoxes()
	{

		int n_permute = 1;
		//for (int i = 1; i < n_shapes-1; i++)
		for (int i = 0; i < n_permute; i++)
		{
			int k = rand() % (n_boxes - 1);
			float x_amount = permute_amount * (1.0 - 2.0*rand() / RAND_MAX);
			float y_amount = permute_amount * (1.0 - 2.0*rand() / RAND_MAX);

			squares[k].x += x_amount;
			squares[k].y += y_amount;

			if (squares[k].x < 0) squares[k].x = 0;
			if (squares[k].y < 0) squares[k].y = 0;
			if (squares[k].x >screen_width-1) squares[k].x = screen_width-1;
			if (squares[k].y > screen_height-1) squares[k].y = screen_height-1;
		}
	}
	double difference(SDL_Surface * a, SDL_Surface * b)
	{
		double difference = 0.0;
		Pixel * o = (Pixel *)a->pixels;
		Pixel * p = (Pixel *)b->pixels;
		for (int i = 0; i < screen_height * screen_width; i++)
		{
				difference += pow(o[i].r - p[i].r, 2) + pow(o[i].g - p[i].g, 2) + pow(o[i].b - p[i].b, 2)/* + pow(o[i].a - p[i].a, 2)*/;
		}
		difference = difference / (screen_width*screen_height);
		return difference;

	}

	void blend(SDL_Surface * a, Square * d)
	{
		Pixel * p = (Pixel *)a->pixels;
		int * visited = new int[screen_width*screen_height];
		for (int i = 0; i < screen_width*screen_height;i++)
		{
			visited[i] = 0;
		}
		for (int i = 0; i < n_boxes; i++)
		{
			for (int j = d[i].y; j < d[i].y + d[i].h;j++)
			{
				for (int k = d[i].x; k < d[i].x + d[i].w; k++)
				{
					p[j * screen_width + k].r = (p[j * screen_width + k].r + d[i].color.r) / 2;
					p[j * screen_width + k].g = (p[j * screen_width + k].g + d[i].color.g) / 2;
					p[j * screen_width + k].b = (p[j * screen_width + k].b + d[i].color.b) / 2;
					/*
					p[j * screen_width + k].r += d[i].color.r;
					p[j * screen_width + k].g += d[i].color.g;
					p[j * screen_width + k].b += d[i].color.b;*/
					//p[j * screen_width + k].a += d[i].color.a;
					//visited[j * screen_width + k]+= 1;
				}
			}
		}
		/*
		for (int i = 0; i < screen_width*screen_height;i++)
		{
			if (visited[i] != 0)
			{
				p[i].r = p[i].r / visited[i];
				p[i].g = p[i].g / visited[i];
				p[i].b = p[i].b / visited[i];
				//p[i].a = p[i].a / visited[i];
			}

		}*/
	}


	void init()
	{
		SDL_Init(SDL_INIT_VIDEO);

		prev_key_state[256];
		keys = (unsigned char*)SDL_GetKeyboardState(NULL);

		window = SDL_CreateWindow(
			"SDL Image Simulated Annealing",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			screen_width, screen_height, SDL_WINDOW_SHOWN);

		renderer = SDL_CreateRenderer(window,
			-1, SDL_RENDERER_SOFTWARE);



		squares = (Square*)malloc(n_boxes * sizeof(Square));
		original = IMG_Load("mickey_mouse.jpg");
		printf("%d\n", original->format->BitsPerPixel);
		Image = SDL_CreateRGBSurfaceWithFormat(0, screen_width,screen_height, 24, SDL_PIXELFORMAT_RGB24);
		SDL_BlitScaled(original, NULL, Image, NULL);
		tmp = SDL_CreateRGBSurfaceWithFormat(0, screen_width, screen_width, 24, SDL_PIXELFORMAT_RGB24);
		scratch = SDL_CreateRGBSurfaceWithFormat(0, screen_width, screen_width, 24, SDL_PIXELFORMAT_RGB24);

		screen = SDL_GetWindowSurface(window);
		/*
		grid = (int*)malloc(scratch->w*scratch->h * sizeof(int));
		memset(grid, 0, sizeof(int)*scratch->w*scratch->h);

		px = (int*)malloc(n_walkers * sizeof(int));
		py = (int*)malloc(n_walkers * sizeof(int));
		color = (Pixel*)malloc(n_walkers * sizeof(Pixel));*

		for (int i = 0; i < n_walkers; i++)
		{
			px[i] = rand() % screen_width;
			py[i] = rand() % screen_height;
			color[i].r = rand() % 256;
			color[i].g = rand() % 256;
			color[i].b = rand() % 256;
		}

		color[0].r = 0;
		color[0].g = 0;
		color[0].b = 0;*/

		for (int i = 0; i < n_boxes; i++)
		{
			squares[i].w = rand() % 50;
			squares[i].h = rand() % 50;
			squares[i].x = rand() % (screen_width-50);
			squares[i].y = rand() % (screen_height-50);
			squares[i].color.r = rand() % 256;
			squares[i].color.g = rand() % 256;
			squares[i].color.b = rand() % 256;
			squares[i].color.a = 255;
		}

		blend(scratch, squares);
		fitness = difference(Image, scratch);
		
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
		SDL_BlitSurface(scratch, NULL, tmp, NULL);
		permuteBoxes();
		blend(tmp, squares);
		double tmpFitness = difference(Image, tmp);
		float p = exp((tmpFitness - fitness) / temperature);
		float r = (double)rand() / RAND_MAX;
		if (tmpFitness < fitness)
		{

			SDL_BlitSurface(tmp, NULL, scratch, NULL);
			fitness = tmpFitness;
		}
		else if (r < p)
		{
			
			SDL_BlitSurface(tmp, NULL, scratch, NULL);
			fitness = tmpFitness;
		}
		temperature *= temperature_decay;
		cout << "Fitness: " << fitness << endl;

		/*
		for (int i = 0; i < n_walkers; i++)
		{

			int k = rand() % 4;
			if (k == 0)
			{
				py[i]--;
				if (py[i] < 0) py[i] = 0;
			}
			else if (k == 1)
			{
				px[i]++;
				if (px[i] > screen_width - 1) px[i] = screen_width - 1;
			}
			else if (k == 2)
			{
				py[i]++;
				if (py[i] > screen_height - 1) py[i] = screen_height - 1;
			}
			else if (k == 3)
			{
				px[i]--;
				if (px[i] < 0) px[i] = 0;
			}

			grid[py[i] *screen_width + px[i]] = i;
		}*/
	}


	void draw_1()
	{
		//clear screen with white
		/*
		for (int i = 0; i < scratch->w*scratch->h; i++)
		{
			Pixel *p = (Pixel *)scratch->pixels;

			p[i].r = color[grid[i]].r;
			p[i].g = color[grid[i]].g;
			p[i].b = color[grid[i]].b;
		}*/

		SDL_BlitScaled(scratch, NULL, screen, NULL);

		//clear screen with white
		for (int i = 0; i < scratch->w*scratch->h; i++)
		{
			Pixel *p = (Pixel *)scratch->pixels;

			p[i].r = 0;
			p[i].g = 0;
			p[i].b = 0;
		}
		


		SDL_UpdateWindowSurface(window);

	}



}
int main(int argc, char **argv)
{

	srand(time(NULL));

	Game::init();


	for(;;)
	{
		Game::update();
		
		Game::draw_1();
	}



	return 0;
}