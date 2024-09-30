#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "SDL/SDL.h"

/**
 * Immortals Map Editor - Version 1.1.0
 * 
 * This is a simple map editing software for project Immortals.
 * It's quite limited;
 *  - The map size is static (18t*25t)
 *  - Metadata for NPC/Item spawns is unfinished
 *  - Cannot make larger maps than 18x25 right now
 *  - Map data is plaintext and can be easily broken
 * 
 * Fortunately, this is only version 1.1.0, so most of this
 * can and will be resolved down the road as the game and this
 * editor expand and become more featureful.
 */

#define MAJOR_VERSION 1
#define MINOR_VERSION 1
#define PATCH_VERSION 0

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define TILE_SIZE 32
#define GRID_WIDTH (SCREEN_WIDTH / TILE_SIZE)
#define GRID_HEIGHT (SCREEN_HEIGHT / TILE_SIZE)
#define MAX_SECTIONS 255
#define MAX_METADATA_LEN 50

typedef struct {
    int type;
    char metadata[MAX_METADATA_LEN];
} Tile;

// Initalize map data
Tile map[MAX_SECTIONS][GRID_HEIGHT][GRID_WIDTH] = {0};
int current_section = 0;
int cursor_x = 0, cursor_y = 0;

// Initalize SDL2
void init_SDL(SDL_Window **window, SDL_Renderer **renderer) { 
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL cannot initalize!\nError: %s\n", SDL_GetError());
        exit(1);
    }

    char EditorVersion[20];  // Ensure this is large enough to hold the version string
    snprintf(EditorVersion, sizeof(EditorVersion), "%d.%d.%d", MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);
    
    char title[50];  // Ensure this is large enough for the title
    snprintf(title, sizeof(title), "Spork %s", EditorVersion);

    *window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
}

// Prompt for metadata (via console input)
void prompt_metadata(char *metadata, const char *prompt) {
    printf("%s: ", prompt);
    fgets(metadata, MAX_METADATA_LEN, stdin);
    metadata[strcspn(metadata, "\n")] = 0;
}

// Save map data to *.mp file
void save_map(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Cannot open file for saving!\n");
        return;
    }

    for (int s = 0; s < MAX_SECTIONS; s++) {
        int section_empty = 1;

        // Check if section contains non-zero vals
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                if (map[s][y][x].type != 0) {
                    section_empty = 0;
                    break;
                }
            }
        }

        if (!section_empty) {
            fprintf(file, "S%d\n", s);
            for (int y = 0; y < GRID_HEIGHT; y++) {
                for (int x = 0; x < GRID_WIDTH; x++) {
                    fprintf(file, "%d ", map[s][y][x].type);
                }

                fprintf(file, "\n");
            }

            // Save metadata for special tiles
            for (int y = 0; y < GRID_HEIGHT; y++) {
                for (int x = 0; x < GRID_WIDTH; x++) {
                    if (map[s][y][x].type == 6 || map[s][y][x].type == 3 || map[s][y][x].type == 4) {
                        fprintf(file, "meta (%d, %d): %s\n", x, y, map[s][y][x].metadata);
                    }
                }
            }
        }
    }

    fclose(file);
    printf("Map data saved to %s\n", filename);
}

bool check_if_map(const char *filename) {
    const char *ext = strrchr(filename, '.');
    return ext && strcmp(ext, ".mp") == 0;
}

// Load *.mp file
void load_map(const char *filename) {
    if (!check_if_map(filename)) {
        printf("Map file must have .mp as its extension!\n");
        return;
    }

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Could not open map file!\n");
        return;
    }

    int section = -1;
    char buffer[256];

    while (fgets(buffer, sizeof(buffer), file)) {
        if (buffer[0] == 'S') {
            sscanf(buffer, "S%d", &section);
        } else if (strstr(buffer, "meta")) {
            int x, y;
            char meta[MAX_METADATA_LEN];
            sscanf(buffer, "meta (%d, %d): %[^\n]", &x, &y, meta);
            strcpy(map[section][y][x].metadata, meta);
        } else if (section >= 0) {
            for (int y = 0; y < GRID_HEIGHT; y++) {
                for (int x = 0; x < GRID_WIDTH; x++) {
                    fscanf(file, "%d", &map[section][y][x].type);
                }
            }
        }
    }
    
    fclose(file);
    printf("Map loaded from %s\n", filename);
}

void prompt_filename(char *filename, const char *prompt) {
    printf("%s: ", prompt);
    fgets(filename, 256, stdin);
    filename[strcspn(filename, "\n")] = 0;

    if (strlen(filename) < 3 || strcmp(&filename[strlen(filename) - 3], ".mp") != 0) {
        strcat(filename, ".mp");
    }
}

// Handle user input to modify the map
void handle_input(SDL_Event *e) {
    static bool ctrl_pressed = false;

    if (e->type == SDL_KEYDOWN) {

        // Check is CTRL is pressed
        if (e->key.keysym.sym == SDLK_LCTRL || e->key.keysym.sym == SDLK_RCTRL) {
            ctrl_pressed = true;
        }

        switch (e->key.keysym.sym) {
            case SDLK_UP:
                if (cursor_y > 0) cursor_y--;
                break;
            case SDLK_DOWN:
                if (cursor_y < GRID_HEIGHT - 1) cursor_y++;
                break;
            case SDLK_LEFT:
                if (cursor_x > 0) cursor_x--;
                break;
            case SDLK_RIGHT:
                if (cursor_x < GRID_WIDTH - 1) cursor_x++;
                break;
            case SDLK_0: case SDLK_1: case SDLK_2: case SDLK_3: 
            case SDLK_4: case SDLK_5: case SDLK_6:
                map[current_section][cursor_y][cursor_x].type = e->key.keysym.sym - SDLK_0;
                // Prompt for metadata if special tile is placed
                if (map[current_section][cursor_y][cursor_x].type == 6) {
                    prompt_metadata(map[current_section][cursor_y][cursor_x].metadata, "Enter WarpTile destination (e.g., S1)");
                } else if (map[current_section][cursor_y][cursor_x].type == 3) {
                    prompt_metadata(map[current_section][cursor_y][cursor_x].metadata, "Enter Item ID");
                } else if (map[current_section][cursor_y][cursor_x].type == 4) {
                    prompt_metadata(map[current_section][cursor_y][cursor_x].metadata, "Enter NPC ID");
                }
                break;
            case SDLK_s:  // Save map
                if(ctrl_pressed) {
                    char filename[256];
                    prompt_filename(filename, " Enter map file name to save: ");
                    save_map(filename);
                }
                break;
            case SDLK_l:  // Load map
                if(ctrl_pressed) {
                    char filename[256];
                    prompt_filename(filename, " Enter map file name to open: ");
                    load_map(filename);
                }
                break;
        }
    } else if (e->type == SDL_KEYUP) {
        if (e->key.keysym.sym == SDLK_LCTRL || e->key.keysym.sym == SDLK_RCTRL) {
            ctrl_pressed = false;
        }
    }
}

// Set tile colour based on key system
/*
0 = nothing
1 = Wall
2 = Door
3 = Item Spawn
4 = NPC Spawn
5 = Player Spawn
6 = WarpTile
*/
void set_tile_colour(SDL_Renderer *renderer, int type) {
    switch (type) {
        case 0: SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); break;          // Black    => Walkable
        case 1: SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); break;    // White    => Wall
        case 2: SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); break;        // Blue     => Interactable Door
        case 3: SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); break;        // Green    => Item Spawn
        case 4: SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); break;      // Yellow   => NPC Spawn
        case 5: SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255); break;      // Orange   => Player Spawn
        case 6: SDL_SetRenderDrawColor(renderer, 255, 105, 180, 255); break;    // Pink     => Warptile (Sx -> Sy)
        default: SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); break;   // Grey     => Invalid/Broken Tile
    }
}

void render(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    
    // Render grid editor
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            SDL_Rect tile = { x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };

            // Set colour based on existing tile type
            set_tile_colour(renderer, map[current_section][y][x].type);
            SDL_RenderFillRect(renderer, &tile);

            // Draw tile outline
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &tile);
        }
    }
    
    // Draw cursor
    SDL_Rect cursor = { cursor_x * TILE_SIZE, cursor_y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &cursor);

    SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[]) {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    init_SDL(&window, &renderer);

    SDL_Event e;
    int quit = 0;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = 1;
            handle_input(&e);
        }
        render(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}