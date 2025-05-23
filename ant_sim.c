#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <raylib.h>

// Constants for the simulation
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_ANTS_PER_COLONY 50
#define NUM_COLONIES 2
#define MAX_FOOD_SOURCES 20
#define MAX_FOOD_PER_SOURCE 100
#define MAP_WIDTH 800
#define MAP_HEIGHT 600
#define ANT_SPEED 1.0f
#define ANT_SIZE 4
#define FOOD_SIZE 6
#define COLONY_SIZE 20
#define PHEROMONE_EVAPORATION_RATE 0.001f
#define PHEROMONE_DEPOSIT_AMOUNT 1.0f
#define MAX_PHEROMONES 5000
#define PHEROMONE_SIZE 2
#define ANT_VISION_RANGE 50.0f
#define ANT_VISION_ANGLE (M_PI / 2) // 90 degrees field of view
#define TURN_ANGLE (M_PI / 10) // How much ants turn each step
#define MIN_FOOD_COLONY_DISTANCE 100.0f  // Minimum distance between food and colonies

// Colony colors
#define COLONY1_COLOR (Color){220, 0, 0, 255}    // Red
#define COLONY2_COLOR (Color){0, 0, 220, 255}    // Blue
#define PHEROMONE1_COLOR (Color){255, 200, 200, 100} // Light red
#define PHEROMONE2_COLOR (Color){200, 200, 255, 100} // Light blue
#define FOOD_COLOR (Color){0, 200, 0, 255}       // Green
#define BACKGROUND_COLOR (Color){50, 50, 50, 255} // Dark gray

// Structs
typedef struct {
    float x, y;
} Vector2D;

typedef struct {
    Vector2D position;
    int amount;
} FoodSource;

typedef struct {
    Vector2D position;
    float strength;
    int colony_id;
} Pheromone;

typedef enum {
    ACTION_MOVE_FORWARD,
    ACTION_TURN_LEFT,
    ACTION_TURN_RIGHT,
    ACTION_DROP_PHEROMONE
} AntAction;

typedef struct {
    Vector2D position;
    float direction; // In radians
    int colony_id;   // 0 or 1
    bool has_food;
    float energy;
} Ant;

typedef struct {
    Vector2D position;
    int food_collected;
    int ants_alive;
} Colony;

typedef struct {
    // Simulation state
    Colony colonies[NUM_COLONIES];
    Ant ants[NUM_COLONIES][MAX_ANTS_PER_COLONY];
    FoodSource food_sources[MAX_FOOD_SOURCES];
    Pheromone pheromones[MAX_PHEROMONES];
    int num_pheromones;
    int num_food_sources;
    
    // Simulation parameters
    float time_elapsed;
    int iteration;
} Simulation;

// Function prototypes
void init_simulation(Simulation *sim);
void update_simulation(Simulation *sim);
void render_simulation(Simulation *sim);
AntAction get_ant_action_policy1(Ant *ant, Simulation *sim);
AntAction get_ant_action_policy2(Ant *ant, Simulation *sim);
void execute_ant_action(Ant *ant, AntAction action, Simulation *sim);
float get_angle_to_colony(Ant *ant, Simulation *sim);
float get_strongest_pheromone_direction(Ant *ant, Simulation *sim);
Vector2D get_closest_food_direction(Ant *ant, Simulation *sim);
bool is_in_vision(Ant *ant, Vector2D target);
void add_pheromone(Simulation *sim, Vector2D position, int colony_id);
float random_float(float min, float max);
float wrap_angle(float angle);

int main(void) {
    // Initialize the window
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Ant Colony Simulator");
    SetTargetFPS(60);
    
    // Initialize simulation
    Simulation sim;
    init_simulation(&sim);
    
    // Main game loop
    while (!WindowShouldClose()) {
        // Update the simulation
        update_simulation(&sim);
        
        // Draw everything
        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
        render_simulation(&sim);
        DrawText(TextFormat("Colony 1 Food: %d", sim.colonies[0].food_collected), 20, 20, 20, COLONY1_COLOR);
        DrawText(TextFormat("Colony 2 Food: %d", sim.colonies[1].food_collected), 20, 50, 20, COLONY2_COLOR);
        DrawText(TextFormat("Time: %.1f", sim.time_elapsed), WINDOW_WIDTH - 120, 20, 20, RAYWHITE);
        EndDrawing();
    }
    
    // Close the window
    CloseWindow();
    
    return 0;
}

void init_simulation(Simulation *sim) {
    // Seed the random number generator
    srand(time(NULL));
    
    // Initialize simulation parameters
    sim->time_elapsed = 0.0f;
    sim->iteration = 0;
    sim->num_pheromones = 0;
    
    // Initialize colonies
    sim->colonies[0].position = (Vector2D){WINDOW_WIDTH / 4, WINDOW_HEIGHT / 2};
    sim->colonies[1].position = (Vector2D){3 * WINDOW_WIDTH / 4, WINDOW_HEIGHT / 2};
    
    for (int i = 0; i < NUM_COLONIES; i++) {
        sim->colonies[i].food_collected = 0;
        sim->colonies[i].ants_alive = MAX_ANTS_PER_COLONY;
        
        // Initialize ants for each colony
        for (int j = 0; j < MAX_ANTS_PER_COLONY; j++) {
            sim->ants[i][j].position = sim->colonies[i].position;
            sim->ants[i][j].direction = random_float(0, 2 * M_PI);
            sim->ants[i][j].colony_id = i;
            sim->ants[i][j].has_food = false;
            sim->ants[i][j].energy = 100.0f;
        }
    }
    
    // Place food sources randomly, ensuring minimum distance from colonies
    sim->num_food_sources = MAX_FOOD_SOURCES;
    for (int i = 0; i < sim->num_food_sources; i++) {
        bool valid_position = false;
        int attempts = 0;
        const int MAX_ATTEMPTS = 100;  // Prevent infinite loops
        
        while (!valid_position && attempts < MAX_ATTEMPTS) {
            sim->food_sources[i].position.x = random_float(50, WINDOW_WIDTH - 50);
            sim->food_sources[i].position.y = random_float(50, WINDOW_HEIGHT - 50);
            
            // Check distance from all colonies
            valid_position = true;
            for (int j = 0; j < NUM_COLONIES; j++) {
                float dx = sim->food_sources[i].position.x - sim->colonies[j].position.x;
                float dy = sim->food_sources[i].position.y - sim->colonies[j].position.y;
                float distance = sqrt(dx * dx + dy * dy);
                
                if (distance < MIN_FOOD_COLONY_DISTANCE) {
                    valid_position = false;
                    break;
                }
            }
            attempts++;
        }
        
        // If we couldn't find a valid position after max attempts, place it anyway
        // but log a warning
        if (!valid_position) {
            printf("Warning: Could not find valid position for food source %d after %d attempts\n", 
                   i, MAX_ATTEMPTS);
        }
        
        sim->food_sources[i].amount = MAX_FOOD_PER_SOURCE;
    }
}

void update_simulation(Simulation *sim) {
    // Update simulation time
    sim->time_elapsed += GetFrameTime();
    sim->iteration++;
    
    // Update ants
    for (int i = 0; i < NUM_COLONIES; i++) {
        for (int j = 0; j < sim->colonies[i].ants_alive; j++) {
            Ant *ant = &sim->ants[i][j];
            
            // Get action based on policy
            AntAction action;
            if (i == 0) {
                action = get_ant_action_policy1(ant, sim);
            } else {
                action = get_ant_action_policy2(ant, sim);
            }
            
            // Execute the action
            execute_ant_action(ant, action, sim);
            
            // Check for food collection
            if (!ant->has_food) {
                for (int k = 0; k < sim->num_food_sources; k++) {
                    FoodSource *food = &sim->food_sources[k];
                    if (food->amount > 0) {
                        float dx = food->position.x - ant->position.x;
                        float dy = food->position.y - ant->position.y;
                        float distance = sqrt(dx * dx + dy * dy);
                        
                        if (distance < ANT_SIZE + FOOD_SIZE) {
                            ant->has_food = true;
                            food->amount--;
                            break;
                        }
                    }
                }
            }
            
            // Check for food delivery to colony
            if (ant->has_food) {
                float dx = sim->colonies[i].position.x - ant->position.x;
                float dy = sim->colonies[i].position.y - ant->position.y;
                float distance = sqrt(dx * dx + dy * dy);
                
                if (distance < ANT_SIZE + COLONY_SIZE) {
                    ant->has_food = false;
                    sim->colonies[i].food_collected++;
                    
                    // Drop pheromone trail when returning with food
                    add_pheromone(sim, ant->position, ant->colony_id);
                }
            }
            
            // Always move forward
            ant->position.x += ANT_SPEED * cos(ant->direction);
            ant->position.y += ANT_SPEED * sin(ant->direction);
            
            // Wrap around the edges of the screen
            if (ant->position.x < 0) ant->position.x = WINDOW_WIDTH;
            if (ant->position.x > WINDOW_WIDTH) ant->position.x = 0;
            if (ant->position.y < 0) ant->position.y = WINDOW_HEIGHT;
            if (ant->position.y > WINDOW_HEIGHT) ant->position.y = 0;
        }
    }
    
    // Update pheromones (evaporation)
    for (int i = 0; i < sim->num_pheromones; i++) {
        sim->pheromones[i].strength -= PHEROMONE_EVAPORATION_RATE;
        
        // Remove pheromones that have evaporated completely
        if (sim->pheromones[i].strength <= 0) {
            // Replace with the last pheromone in the array
            sim->pheromones[i] = sim->pheromones[sim->num_pheromones - 1];
            sim->num_pheromones--;
            i--; // Check the replaced pheromone in the next iteration
        }
    }
}

void render_simulation(Simulation *sim) {
    // Draw colonies
    for (int i = 0; i < NUM_COLONIES; i++) {
        Color colony_color = (i == 0) ? COLONY1_COLOR : COLONY2_COLOR;
        DrawCircle(sim->colonies[i].position.x, sim->colonies[i].position.y, COLONY_SIZE, colony_color);
    }
    
    // Draw food sources
    for (int i = 0; i < sim->num_food_sources; i++) {
        if (sim->food_sources[i].amount > 0) {
            DrawCircle(sim->food_sources[i].position.x, sim->food_sources[i].position.y, 
                      FOOD_SIZE, FOOD_COLOR);
            
            // Draw food amount text
            if (sim->food_sources[i].amount > 1) {
                DrawText(TextFormat("%d", sim->food_sources[i].amount), 
                        sim->food_sources[i].position.x - 10, 
                        sim->food_sources[i].position.y - 10, 
                        12, RAYWHITE);
            }
        }
    }
    
    // Draw pheromones (with some transparency)
    for (int i = 0; i < sim->num_pheromones; i++) {
        Color pheromone_color;
        if (sim->pheromones[i].colony_id == 0) {
            pheromone_color = PHEROMONE1_COLOR;
        } else {
            pheromone_color = PHEROMONE2_COLOR;
        }
        
        // Adjust alpha based on strength
        pheromone_color.a = (unsigned char)(100 * sim->pheromones[i].strength);
        
        DrawCircle(sim->pheromones[i].position.x, sim->pheromones[i].position.y, 
                  PHEROMONE_SIZE, pheromone_color);
    }
    
    // Draw ants
    for (int i = 0; i < NUM_COLONIES; i++) {
        Color ant_color = (i == 0) ? COLONY1_COLOR : COLONY2_COLOR;
        
        for (int j = 0; j < sim->colonies[i].ants_alive; j++) {
            Ant *ant = &sim->ants[i][j];
            
            // Draw ant body
            DrawCircle(ant->position.x, ant->position.y, ANT_SIZE, ant->has_food ? FOOD_COLOR : ant_color);
            
            // Draw direction indicator
            float dir_x = ant->position.x + (ANT_SIZE * 1.5f) * cos(ant->direction);
            float dir_y = ant->position.y + (ANT_SIZE * 1.5f) * sin(ant->direction);
            DrawLine(ant->position.x, ant->position.y, dir_x, dir_y, RAYWHITE);
        }
    }
}

AntAction get_ant_action_policy1(Ant *ant, Simulation *sim) {
    // Policy 1: More focus on following pheremones
    // If the ant has food, head back to the colony
    if (ant->has_food) {
        float angle_to_colony = get_angle_to_colony(ant, sim);
        float angle_diff = wrap_angle(angle_to_colony - ant->direction);
        
        if (angle_diff > 0.1) return ACTION_TURN_RIGHT;
        if (angle_diff < -0.1) return ACTION_TURN_LEFT;
        
        // Occasionally drop pheromone when returning with food
        if (rand() % 20 == 0) return ACTION_DROP_PHEROMONE;
        
        return ACTION_MOVE_FORWARD;
    } 
    // If the ant doesn't have food
    else {
        // Check if there's food in sight
        Vector2D food_dir = get_closest_food_direction(ant, sim);
        if (food_dir.x != 0 || food_dir.y != 0) {
            float food_angle = atan2(food_dir.y, food_dir.x);
            float angle_diff = wrap_angle(food_angle - ant->direction);
            
            if (angle_diff > 0.1) return ACTION_TURN_RIGHT;
            if (angle_diff < -0.1) return ACTION_TURN_LEFT;
            
            return ACTION_MOVE_FORWARD;
        }
        
        // Follow pheromones with high probability (80%)
        if (rand() % 100 < 80) {
            float pheromone_direction = get_strongest_pheromone_direction(ant, sim);
            if (pheromone_direction != -1) {
                float angle_diff = wrap_angle(pheromone_direction - ant->direction);
                
                if (angle_diff > 0.1) return ACTION_TURN_RIGHT;
                if (angle_diff < -0.1) return ACTION_TURN_LEFT;
                
                return ACTION_MOVE_FORWARD;
            }
        }
        
        // Random walk (with occasional pheromone drops)
        if (rand() % 100 < 10) return ACTION_DROP_PHEROMONE;
        if (rand() % 100 < 25) return ACTION_TURN_LEFT;
        if (rand() % 100 < 25) return ACTION_TURN_RIGHT;
        
        return ACTION_MOVE_FORWARD;
    }
}

AntAction get_ant_action_policy2(Ant *ant, Simulation *sim) {
    // Policy 2: More explorative behavior, less pheromone following
    // If the ant has food, head back to the colony
    if (ant->has_food) {
        float angle_to_colony = get_angle_to_colony(ant, sim);
        float angle_diff = wrap_angle(angle_to_colony - ant->direction);
        
        if (angle_diff > 0.1) return ACTION_TURN_RIGHT;
        if (angle_diff < -0.1) return ACTION_TURN_LEFT;
        
        // Frequently drop pheromone when returning with food
        if (rand() % 10 == 0) return ACTION_DROP_PHEROMONE;
        
        return ACTION_MOVE_FORWARD;
    } 
    // If the ant doesn't have food
    else {
        // Check if there's food in sight
        Vector2D food_dir = get_closest_food_direction(ant, sim);
        if (food_dir.x != 0 || food_dir.y != 0) {
            float food_angle = atan2(food_dir.y, food_dir.x);
            float angle_diff = wrap_angle(food_angle - ant->direction);
            
            if (angle_diff > 0.1) return ACTION_TURN_RIGHT;
            if (angle_diff < -0.1) return ACTION_TURN_LEFT;
            
            return ACTION_MOVE_FORWARD;
        }
        
        // Follow pheromones with medium probability (40%)
        if (rand() % 100 < 40) {
            float pheromone_direction = get_strongest_pheromone_direction(ant, sim);
            if (pheromone_direction != -1) {
                float angle_diff = wrap_angle(pheromone_direction - ant->direction);
                
                if (angle_diff > 0.1) return ACTION_TURN_RIGHT;
                if (angle_diff < -0.1) return ACTION_TURN_LEFT;
                
                return ACTION_MOVE_FORWARD;
            }
        }
        
        // More random movements for exploration
        if (rand() % 100 < 5) return ACTION_DROP_PHEROMONE;
        if (rand() % 100 < 40) return ACTION_TURN_LEFT;
        if (rand() % 100 < 40) return ACTION_TURN_RIGHT;
        
        return ACTION_MOVE_FORWARD;
    }
}

void execute_ant_action(Ant *ant, AntAction action, Simulation *sim) {
    switch (action) {
        case ACTION_MOVE_FORWARD:
            // Movement handled in update_simulation
            break;
        case ACTION_TURN_LEFT:
            ant->direction -= TURN_ANGLE;
            ant->direction = wrap_angle(ant->direction);
            break;
        case ACTION_TURN_RIGHT:
            ant->direction += TURN_ANGLE;
            ant->direction = wrap_angle(ant->direction);
            break;
        case ACTION_DROP_PHEROMONE:
            add_pheromone(sim, ant->position, ant->colony_id);
            break;
    }
}

float get_angle_to_colony(Ant *ant, Simulation *sim) {
    Vector2D colony_pos = sim->colonies[ant->colony_id].position;
    float dx = colony_pos.x - ant->position.x;
    float dy = colony_pos.y - ant->position.y;
    
    return atan2(dy, dx);
}

float get_strongest_pheromone_direction(Ant *ant, Simulation *sim) {
    float strongest_strength = 0;
    float strongest_direction = -1;
    
    for (int i = 0; i < sim->num_pheromones; i++) {
        // Only follow pheromones from own colony
        if (sim->pheromones[i].colony_id != ant->colony_id) continue;
        
        float dx = sim->pheromones[i].position.x - ant->position.x;
        float dy = sim->pheromones[i].position.y - ant->position.y;
        float distance_squared = dx * dx + dy * dy;
        
        // Only consider pheromones within vision range
        if (distance_squared <= ANT_VISION_RANGE * ANT_VISION_RANGE) {
            Vector2D pheromone_pos = sim->pheromones[i].position;
            if (is_in_vision(ant, pheromone_pos)) {
                float strength = sim->pheromones[i].strength / (distance_squared + 1);
                
                if (strength > strongest_strength) {
                    strongest_strength = strength;
                    strongest_direction = atan2(dy, dx);
                }
            }
        }
    }
    
    return strongest_direction;
}

Vector2D get_closest_food_direction(Ant *ant, Simulation *sim) {
    Vector2D result = {0, 0};
    float closest_distance_squared = ANT_VISION_RANGE * ANT_VISION_RANGE;
    
    for (int i = 0; i < sim->num_food_sources; i++) {
        if (sim->food_sources[i].amount <= 0) continue;
        
        float dx = sim->food_sources[i].position.x - ant->position.x;
        float dy = sim->food_sources[i].position.y - ant->position.y;
        float distance_squared = dx * dx + dy * dy;
        
        if (distance_squared < closest_distance_squared) {
            Vector2D food_pos = sim->food_sources[i].position;
            if (is_in_vision(ant, food_pos)) {
                closest_distance_squared = distance_squared;
                result.x = dx;
                result.y = dy;
            }
        }
    }
    
    return result;
}

bool is_in_vision(Ant *ant, Vector2D target) {
    float dx = target.x - ant->position.x;
    float dy = target.y - ant->position.y;
    float distance_squared = dx * dx + dy * dy;
    
    // Check if within vision range
    if (distance_squared > ANT_VISION_RANGE * ANT_VISION_RANGE) {
        return false;
    }
    
    // Check if within vision angle
    float target_angle = atan2(dy, dx);
    float angle_diff = wrap_angle(target_angle - ant->direction);
    
    return fabs(angle_diff) <= ANT_VISION_ANGLE / 2;
}

void add_pheromone(Simulation *sim, Vector2D position, int colony_id) {
    // Make sure we don't exceed the maximum number of pheromones
    if (sim->num_pheromones >= MAX_PHEROMONES) {
        // Replace the oldest (first) pheromone
        for (int i = 0; i < sim->num_pheromones - 1; i++) {
            sim->pheromones[i] = sim->pheromones[i + 1];
        }
        sim->num_pheromones--;
    }
    
    // Add new pheromone
    sim->pheromones[sim->num_pheromones].position = position;
    sim->pheromones[sim->num_pheromones].strength = PHEROMONE_DEPOSIT_AMOUNT;
    sim->pheromones[sim->num_pheromones].colony_id = colony_id;
    sim->num_pheromones++;
}

float random_float(float min, float max) {
    return min + (max - min) * ((float)rand() / RAND_MAX);
}

float wrap_angle(float angle) {
    while (angle > M_PI) angle -= 2 * M_PI;
    while (angle < -M_PI) angle += 2 * M_PI;
    return angle;
}