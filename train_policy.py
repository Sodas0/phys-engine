#!/usr/bin/env python3
"""
Ball balancing training using Stable Baselines 3 PPO.
Uses Gymnasium environment wrapper for the custom physics engine.
"""

import numpy as np
import gymnasium as gym
from gymnasium import spaces
import sim_bindings
from stable_baselines3 import PPO
from stable_baselines3.common.callbacks import BaseCallback
from stable_baselines3.common.vec_env import DummyVecEnv
import os
import torch

# ============================================================
# Gymnasium Environment Wrapper
# ============================================================

class BalanceBeamEnv(gym.Env):
    """
    Gymnasium environment for beam-ball balancing task.
    
    Observation:
        - beam_angle (rad): angle of the beam relative to horizontal
        - beam_angular_velocity (rad/s): angular velocity of the beam
        - ball_position (px): ball position along beam (0 = center)
        - ball_velocity (px/s): ball velocity along beam
    
    Action:
        - torque (continuous): torque applied to beam, range [-1, 1]
    
    Reward:
        Dense reward based on keeping ball centered and beam level.
    """
    
    metadata = {'render_modes': ['human'], 'render_fps': 60}
    
    def __init__(self, scene_path, seed=0, dt=1/60, headless=True, random_init=True):
        super().__init__()
        
        self.scene_path = scene_path
        self.dt = dt
        self.headless = headless
        self.random_init = random_init
        self.rng = np.random.RandomState(seed)
        self._seed = seed
        
        # Initialize environment
        self.env = sim_bindings.Environment(scene_path, seed, dt, headless=headless)
        self.max_steps = 2000
        self.steps = 0
        
        # Get observation dimensions
        obs = self.env.reset()
        self.obs_dim = len(obs)
        
        # Observation normalization statistics
        # [angle, ang_vel, pos, vel]
        self.obs_scale = np.array([1.0, 0.5, 0.005, 0.01], dtype=np.float32)
        
        # Define action and observation spaces for Gymnasium
        # Action space: continuous torque in [-1, 1]
        self.action_space = spaces.Box(
            low=-1.0, high=1.0, shape=(1,), dtype=np.float32
        )
        
        # Observation space: scaled observations (approximately [-1, 1] range)
        self.observation_space = spaces.Box(
            low=-np.inf, high=np.inf, shape=(self.obs_dim,), dtype=np.float32
        )
    
    def reset(self, seed=None, options=None):
        """Reset environment to initial state."""
        if seed is not None:
            self._seed = seed
            self.rng = np.random.RandomState(seed)
        
        self.steps = 0
        
        # Use random seed for each episode if random_init is enabled
        if self.random_init:
            new_seed = self.rng.randint(0, 2**31 - 1)
            # Recreate environment with new seed for proper randomization
            self.env = sim_bindings.Environment(
                self.scene_path, new_seed, self.dt, headless=self.headless
            )
        
        obs = np.array(self.env.reset(), dtype=np.float32)
        obs = obs * self.obs_scale  # Normalize
        
        # Gymnasium requires returning (observation, info)
        return obs, {}
    
    def step(self, action):
        """Execute one step in the environment."""
        # Handle both scalar and array actions
        if isinstance(action, np.ndarray):
            action = float(action[0])
        else:
            action = float(action)
        
        # Clip action to valid range
        action = np.clip(action, -1.0, 1.0)
        
        # Step the environment
        obs, reward, terminated, truncated = self.env.step(action)
        self.steps += 1
        
        # Check for episode truncation
        if self.steps >= self.max_steps:
            truncated = True
        
        # Normalize observation
        obs = np.array(obs, dtype=np.float32) * self.obs_scale
        
        # Gymnasium requires returning (observation, reward, terminated, truncated, info)
        return obs, reward, terminated, truncated, {}
    
    def render(self):
        """Render the environment."""
        if not self.headless:
            self.env.render()
    
    def close(self):
        """Clean up resources."""
        pass


# ============================================================
# Custom Callback for Logging
# ============================================================

class TrainingCallback(BaseCallback):
    """
    Custom callback for logging training progress.
    """
    
    def __init__(self, verbose=0):
        super().__init__(verbose)
        self.episode_rewards = []
        self.episode_lengths = []
    
    def _on_step(self) -> bool:
        # Log episode statistics when available
        if len(self.model.ep_info_buffer) > 0 and len(self.model.ep_info_buffer) > len(self.episode_rewards):
            for info in self.model.ep_info_buffer[len(self.episode_rewards):]:
                self.episode_rewards.append(info['r'])
                self.episode_lengths.append(info['l'])
        
        # Print progress every 10k steps
        if self.num_timesteps % 10000 == 0:
            if len(self.episode_rewards) > 0:
                recent_rewards = self.episode_rewards[-100:]
                recent_lengths = self.episode_lengths[-100:]
                
                print(f"\n{'='*80}")
                print(f"Step: {self.num_timesteps:,}")
                print(f"Episodes: {len(self.episode_rewards)}")
                print(f"Mean Return (last 100 eps): {np.mean(recent_rewards):.2f} ± {np.std(recent_rewards):.2f}")
                print(f"Mean Length (last 100 eps): {np.mean(recent_lengths):.1f}")
                print(f"{'='*80}\n")
        
        return True


# ============================================================
# Main Training Function
# ============================================================

def train_sb3_ppo(scene_path, total_timesteps=300_000, save_path="ppo_balance_model"):
    """
    Train a PPO agent using Stable Baselines 3.
    
    Args:
        scene_path: Path to the scene JSON file
        total_timesteps: Total number of training timesteps
        save_path: Path to save the trained model
    """
    
    print("=" * 80)
    print("Training with Stable Baselines 3 PPO".center(80))
    print("=" * 80)
    print(f"Scene: {scene_path}")
    print(f"Total timesteps: {total_timesteps:,}")
    print(f"Model will be saved to: {save_path}.zip")
    print("=" * 80)
    print()
    
    # Create environment
    env = BalanceBeamEnv(
        scene_path=scene_path,
        seed=0,
        headless=True,
        random_init=True
    )
    
    # Wrap in DummyVecEnv (required by SB3)
    env = DummyVecEnv([lambda: env])
    
    # Create PPO model with good hyperparameters for continuous control
    model = PPO(
        "MlpPolicy",
        env,
        learning_rate=3e-4,
        n_steps=2048,          # Number of steps to collect before update
        batch_size=64,         # Minibatch size
        n_epochs=10,           # Number of epochs for policy update
        gamma=0.99,            # Discount factor
        gae_lambda=0.95,       # GAE lambda
        clip_range=0.2,        # PPO clip parameter
        ent_coef=0.01,         # Entropy coefficient (for exploration)
        vf_coef=0.5,           # Value function coefficient
        max_grad_norm=0.5,     # Gradient clipping
        verbose=1,
        tensorboard_log="./tensorboard_logs/",
        policy_kwargs={
            "net_arch": [128, 128],  # Neural network architecture
            "activation_fn": torch.nn.Tanh
        }
    )
    
    # Create callback for progress monitoring
    callback = TrainingCallback()
    
    # Train the model
    print("Starting training...")
    print()
    model.learn(
        total_timesteps=total_timesteps,
        callback=callback,
        progress_bar=True
    )
    
    # Save the model
    model.save(save_path)
    print()
    print("=" * 80)
    print("Training Complete!".center(80))
    print("=" * 80)
    print(f"Model saved to: {save_path}.zip")
    
    if len(callback.episode_rewards) > 0:
        print(f"Total episodes: {len(callback.episode_rewards)}")
        print(f"Final mean return (last 100 eps): {np.mean(callback.episode_rewards[-100:]):.2f}")
        print(f"Best return: {np.max(callback.episode_rewards):.2f}")
    
    print("=" * 80)
    print()
    
    return model


# ============================================================
# Evaluation Function
# ============================================================

def evaluate_model(model, scene_path, n_episodes=5, render=True):
    """
    Evaluate a trained model.
    
    Args:
        model: Trained SB3 model
        scene_path: Path to scene JSON file
        n_episodes: Number of episodes to evaluate
        render: Whether to render the environment
    """
    # Create evaluation environment
    env = BalanceBeamEnv(
        scene_path=scene_path,
        seed=42,
        headless=not render,
        random_init=False
    )
    
    print("=" * 80)
    print("Evaluating Model".center(80))
    print("=" * 80)
    
    for episode in range(n_episodes):
        obs, _ = env.reset()
        done = False
        total_reward = 0
        steps = 0
        
        while not done:
            # Get action from model (deterministic for evaluation)
            action, _ = model.predict(obs, deterministic=True)
            obs, reward, terminated, truncated, _ = env.step(action)
            done = terminated or truncated
            
            total_reward += reward
            steps += 1
            
            if render:
                env.render()
                import time
                time.sleep(1/60)
        
        print(f"Episode {episode+1}: Steps={steps}, Return={total_reward:.2f}")
    
    print("=" * 80)
    print()


# ============================================================
# Main Entry Point
# ============================================================

def main():
    import torch
    
    scene = "scenes/fulcrum.json"
    
    # Train the model
    model = train_sb3_ppo(
        scene_path=scene,
        total_timesteps=300_000,
        save_path="ppo_balance_model"
    )
    

if __name__ == "__main__":
    main()
