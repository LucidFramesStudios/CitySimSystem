# CitySimSystem
### Real-Time Deterministic Multi-Agent Simulation (Unreal Engine C++)

A system-driven city simulation where autonomous agents make decisions, interact with environments, and behave consistently under a deterministic architecture.

---

##  Demo


Demo Video => https://www.youtube.com/watch?v=RLmhAbE9Z78

<img width="2752" height="1534" alt="ArchitectureBreakdown" src="https://github.com/user-attachments/assets/2a223095-7b81-47dd-a2e3-d7dba6a90fa3" />

<img width="1067" height="660" alt="Screenshot 2026-03-25 193737" src="https://github.com/user-attachments/assets/4414b0c4-ba30-465e-9c5b-17e11578ba70" />

<img width="1064" height="646" alt="Screenshot 2026-03-25 190434" src="https://github.com/user-attachments/assets/c21963da-f8fb-48db-9859-34f76099a58e" />

<img width="1066" height="662" alt="Screenshot 2026-03-25 190539" src="https://github.com/user-attachments/assets/857f176d-3232-4615-b97a-ad20457033d5" />

<img width="1072" height="658" alt="Screenshot 2026-03-25 190636" src="https://github.com/user-attachments/assets/f094b2b1-544e-4a7d-9404-9ceb8c6ddf21" />





---

##  What This System Demonstrates

- Agents operate on a **full daily lifecycle** (home → work → leisure)
- Decisions are made **in real time** using goal-driven AI
- Behavior emerges from systems, not hardcoded sequences
- The entire simulation is **deterministic and reproducible**

---

##  Why This Is Different

Most simulations focus only on behavior.

This system focuses on **control, observability, and scalability**:

- Every agent decision is visible  
- Every system can be manipulated live  
- Every behavior can be traced and debugged  

---

##  Core Systems

- **Deterministic Lifecycle System**  
  Time-driven scheduling (Work, Sleep, Leisure)

- **Goal-Driven AI**  
  Utility-based decision making with validated targets

- **Interaction System**  
  Slot-based smart objects with spatial partitioning

- **Transit & Routing**  
  Custom A* pathfinding for pedestrians and vehicles

- **Real-Time Simulation Control**  
  Time scaling, weather events, and system triggers

---

##  Debug & Observability

The system exposes internal state in real time:

- Agent state, goals, and decisions  
- Pathfinding routes and targets  
- Density heatmaps and flow visualization  
- Spatial grid and interaction zones  
- Real-time performance metrics  

### Debug Commands


Sim_DebugAgents
Sim_DebugPaths
Sim_DebugDensity
Sim_DebugSpatialGrid
Sim_DebugStats
Sim_DebugVehicles
Sim_DebugTransit
Sim_DebugSocial
Sim_DebugZones


---

##  Architecture

The simulation is built using decoupled subsystems:

- `UWorldSimulationSubsystem` → master clock & tick orchestration  
- `UNeedsSubsystem` → agent data & need evaluation  
- `USimAISubsystem` → decision making & movement logic  
- `UInteractionSubsystem` → smart object management  
- `ULifeSchedulerSubsystem` → deterministic lifecycle  

### Pipeline


Decision → Target → Path → Movement


---

##  Controls

**Camera**
- `1` → Eagle Eye  
- `2` → Free  
- `3` → Follow  

**Simulation**
- `7` → 1x speed  
- `8` → 5x speed  
- `9` → 20x speed  
- `R` → Toggle Rain
- Arrow Left and Right Keys → Switch Between Previous and Next Agent 

**Movement**
- `WASDQE` → Move  
- Mouse → Look  

---

##  Performance & Scaling

- Designed for **hundreds of agents in real time**  
- Deterministic batching and subsystem updates  
- Async pathfinding to prevent frame spikes  

> Current limitation: visual layer uses `ACharacter`, which limits scaling.  
> Future: MassEntity / instancing for 10k+ agents.

---

##  Contact

studioslucidframes@gmail.com  
Mumbai, India
