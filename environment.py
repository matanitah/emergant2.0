import numpy as np
import gymnasium as gym
from gymnasium import spaces
from dataclasses import dataclass
from typing import List, Tuple, Optional
import math

@dataclass
class Vector2D:
    x: float
    y: float

@dataclass
class FoodSource:
    position: Vector2D
    amount: int

@dataclass
class Pheromone:
    position: Vector2D
    strength: float
    colony_id: int

@dataclass
class Ant:
    position: Vector2D
    direction: float  # In radians
    colony_id: int
    has_food: bool
    energy: float

@dataclass
class Colony:
    position: Vector2D
    food_collected: int
    ants_alive: int

class AntColonyEnv(gym.Env):
    """
    Ant Colony Simulation Environment
    """
    metadata = {'render.modes': ['human']}

    def __init__(self, 
                 window_width: int = 800,
                 window_height: int = 600,
                 max_ants_per_colony: int = 50,
                 num_colonies: int = 2,
                 max_food_sources: int = 20,
                 max_food_per_source: int = 100,
                 ant_speed: float = 1.0,
                 ant_vision_range: float = 50.0,
                 ant_vision_angle: float = math.pi / 2,
                 turn_angle: float = math.pi / 10,
                 min_food_colony_distance: float = 100.0):
        
        super().__init__()
        
        # Constants
        self.WINDOW_WIDTH = window_width
        self.WINDOW_HEIGHT = window_height
        self.MAX_ANTS_PER_COLONY = max_ants_per_colony
        self.NUM_COLONIES = num_colonies
        self.MAX_FOOD_SOURCES = max_food_sources
        self.MAX_FOOD_PER_SOURCE = max_food_per_source
        self.ANT_SPEED = ant_speed
        self.ANT_VISION_RANGE = ant_vision_range
        self.ANT_VISION_ANGLE = ant_vision_angle
        self.TURN_ANGLE = turn_angle
        self.MIN_FOOD_COLONY_DISTANCE = min_food_colony_distance
        
        # Initialize state
        self.colonies: List[Colony] = []
        self.ants: List[List[Ant]] = []
        self.food_sources: List[FoodSource] = []
        self.pheromones: List[Pheromone] = []
        
        # Define action and observation spaces
        # Action space: [move_forward, turn_left, turn_right, drop_pheromone]
        self.action_space = spaces.Discrete(4)
        
        # Observation space: [ant_x, ant_y, ant_direction, has_food, 
        #                    closest_food_x, closest_food_y,
        #                    strongest_pheromone_x, strongest_pheromone_y,
        #                    colony_x, colony_y]
        self.observation_space = spaces.Box(
            low=np.array([0, 0, -math.pi, 0, 0, 0, 0, 0, 0, 0]),
            high=np.array([window_width, window_height, math.pi, 1, 
                          window_width, window_height, 
                          window_width, window_height,
                          window_width, window_height]),
            dtype=np.float32
        )

    def reset(self, seed: Optional[int] = None) -> Tuple[np.ndarray, dict]:
        """Reset the environment to initial state"""
        super().reset(seed=seed)
        
        # Initialize colonies
        self.colonies = [
            Colony(Vector2D(self.WINDOW_WIDTH / 4, self.WINDOW_HEIGHT / 2), 0, self.MAX_ANTS_PER_COLONY),
            Colony(Vector2D(3 * self.WINDOW_WIDTH / 4, self.WINDOW_HEIGHT / 2), 0, self.MAX_ANTS_PER_COLONY)
        ]
        
        # Initialize ants
        self.ants = []
        for colony_id in range(self.NUM_COLONIES):
            colony_ants = []
            for _ in range(self.MAX_ANTS_PER_COLONY):
                ant = Ant(
                    position=self.colonies[colony_id].position,
                    direction=np.random.uniform(0, 2 * math.pi),
                    colony_id=colony_id,
                    has_food=False,
                    energy=100.0
                )
                colony_ants.append(ant)
            self.ants.append(colony_ants)
        
        # Initialize food sources
        self.food_sources = []
        for _ in range(self.MAX_FOOD_SOURCES):
            valid_position = False
            attempts = 0
            while not valid_position and attempts < 100:
                pos = Vector2D(
                    np.random.uniform(50, self.WINDOW_WIDTH - 50),
                    np.random.uniform(50, self.WINDOW_HEIGHT - 50)
                )
                
                valid_position = True
                for colony in self.colonies:
                    dx = pos.x - colony.position.x
                    dy = pos.y - colony.position.y
                    distance = math.sqrt(dx * dx + dy * dy)
                    if distance < self.MIN_FOOD_COLONY_DISTANCE:
                        valid_position = False
                        break
                attempts += 1
            
            if valid_position:
                self.food_sources.append(FoodSource(pos, self.MAX_FOOD_PER_SOURCE))
        
        # Initialize empty pheromone list
        self.pheromones = []
        
        return self._get_observation(), {}

    def step(self, action: int) -> Tuple[np.ndarray, float, bool, bool, dict]:
        """Execute one time step within the environment"""
        # Execute action for each ant
        for colony_id in range(self.NUM_COLONIES):
            for ant in self.ants[colony_id]:
                self._execute_ant_action(ant, action)
        
        # Update pheromones (evaporation)
        self._update_pheromones()
        
        # Check for food collection and delivery
        self._check_food_interactions()
        
        # Calculate reward (can be customized based on your needs)
        reward = self._calculate_reward()
        
        # Get new observation
        observation = self._get_observation()
        
        # Check if episode is done
        done = self._is_done()
        
        return observation, reward, done, False, {}

    def _execute_ant_action(self, ant: Ant, action: int):
        """Execute a single action for an ant"""
        if action == 0:  # Move forward
            ant.position.x += self.ANT_SPEED * math.cos(ant.direction)
            ant.position.y += self.ANT_SPEED * math.sin(ant.direction)
        elif action == 1:  # Turn left
            ant.direction -= self.TURN_ANGLE
        elif action == 2:  # Turn right
            ant.direction += self.TURN_ANGLE
        elif action == 3:  # Drop pheromone
            self._add_pheromone(ant.position, ant.colony_id)
        
        # Wrap around screen edges
        ant.position.x = ant.position.x % self.WINDOW_WIDTH
        ant.position.y = ant.position.y % self.WINDOW_HEIGHT
        
        # Normalize direction to [-pi, pi]
        ant.direction = (ant.direction + math.pi) % (2 * math.pi) - math.pi

    def _update_pheromones(self):
        """Update pheromone strengths and remove evaporated ones"""
        self.pheromones = [p for p in self.pheromones if p.strength > 0.001]
        for p in self.pheromones:
            p.strength -= 0.001

    def _check_food_interactions(self):
        """Check for food collection and delivery"""
        for colony_id in range(self.NUM_COLONIES):
            for ant in self.ants[colony_id]:
                if not ant.has_food:
                    # Check for food collection
                    for food in self.food_sources:
                        if food.amount > 0:
                            dx = food.position.x - ant.position.x
                            dy = food.position.y - ant.position.y
                            distance = math.sqrt(dx * dx + dy * dy)
                            if distance < 10:  # Collection radius
                                ant.has_food = True
                                food.amount -= 1
                                break
                else:
                    # Check for food delivery
                    colony = self.colonies[colony_id]
                    dx = colony.position.x - ant.position.x
                    dy = colony.position.y - ant.position.y
                    distance = math.sqrt(dx * dx + dy * dy)
                    if distance < 20:  # Delivery radius
                        ant.has_food = False
                        colony.food_collected += 1

    def _add_pheromone(self, position: Vector2D, colony_id: int):
        """Add a new pheromone to the environment"""
        self.pheromones.append(Pheromone(position, 1.0, colony_id))

    def _get_observation(self) -> np.ndarray:
        """Get the current observation of the environment"""
        # For now, return a simple observation of the first ant
        ant = self.ants[0][0]
        colony = self.colonies[0]
        
        # Find closest food
        closest_food = Vector2D(0, 0)
        min_food_dist = float('inf')
        for food in self.food_sources:
            if food.amount > 0:
                dx = food.position.x - ant.position.x
                dy = food.position.y - ant.position.y
                dist = math.sqrt(dx * dx + dy * dy)
                if dist < min_food_dist:
                    min_food_dist = dist
                    closest_food = food.position
        
        # Find strongest pheromone
        strongest_pheromone = Vector2D(0, 0)
        max_strength = 0
        for p in self.pheromones:
            if p.colony_id == ant.colony_id:
                dx = p.position.x - ant.position.x
                dy = p.position.y - ant.position.y
                dist = math.sqrt(dx * dx + dy * dy)
                if dist < self.ANT_VISION_RANGE and p.strength > max_strength:
                    max_strength = p.strength
                    strongest_pheromone = p.position
        
        return np.array([
            ant.position.x,
            ant.position.y,
            ant.direction,
            float(ant.has_food),
            closest_food.x,
            closest_food.y,
            strongest_pheromone.x,
            strongest_pheromone.y,
            colony.position.x,
            colony.position.y
        ], dtype=np.float32)

    def _calculate_reward(self) -> float:
        """Calculate the reward for the current state"""
        # TODO improve this function significantly
        # Simple reward based on food collected
        return sum(colony.food_collected for colony in self.colonies)

    def _is_done(self) -> bool:
        """Check if the episode is done"""
        # Episode ends when all food is collected
        return all(food.amount == 0 for food in self.food_sources)

    def render(self):
        """Render the environment"""
        # This will be implemented using PyX
        pass