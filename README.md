# Ant Colony Simulation with Reinforcement Learning

This project simulates ant colonies using reinforcement learning to develop efficient foraging strategies. The simulation uses PyTorch for the RL implementation and PyX for visualization.

## Features

- Multiple ant colonies competing for food resources
- Deep Q-Learning (DQN) for ant behavior optimization
- Pheromone-based communication between ants
- Real-time visualization using PyX
- Configurable simulation parameters
- Save/load trained models

## Requirements

- Python 3.8+
- PyTorch
- NumPy
- PyX
- Gymnasium
- Matplotlib

Install the required packages:

```bash
pip install -r requirements.txt
```

## Project Structure

```
ant_sim/
├── __init__.py
├── environment.py    # Simulation environment
├── agent.py         # RL agent implementation
└── visualizer.py    # PyX-based visualization
train.py            # Training script
requirements.txt    # Project dependencies
```

## Usage

### Training

To train the ant colonies:

```bash
python train.py
```

This will:
1. Initialize the simulation environment
2. Create DQN agents for each colony
3. Run the training loop
4. Save model checkpoints periodically
5. Generate visualization frames

### Configuration

You can modify the following parameters in `train.py`:

- `episodes`: Number of training episodes
- `render_interval`: How often to save visualization frames
- `save_interval`: How often to save model checkpoints
- `model_dir`: Directory for saved models
- `render_dir`: Directory for visualization frames

## How It Works

### Environment

The simulation environment (`AntColonyEnv`) implements a Gymnasium interface with:

- State space: Ant position, direction, food status, and environmental information
- Action space: Move forward, turn left, turn right, drop pheromone
- Reward function: Based on food collection and delivery

### RL Agent

The DQN agent (`DQNAgent`) uses:

- Experience replay buffer
- Target network for stable learning
- Epsilon-greedy exploration
- Neural network policy

### Visualization

The visualizer (`AntVisualizer`) renders:

- Ant colonies (red and blue)
- Individual ants with direction indicators
- Food sources with remaining amounts
- Pheromone trails with varying intensities
- Real-time statistics

## Contributing

Feel free to submit issues and enhancement requests!