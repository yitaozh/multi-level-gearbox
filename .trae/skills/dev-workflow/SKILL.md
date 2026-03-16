---
name: "dev-workflow"
description: "Guides the user through the code modification, compilation, and testing workflow for the Multi-Level Gearbox project. Invoke when user asks how to build, test, or modify the code."
---

# Development Workflow

This skill provides step-by-step instructions for modifying, compiling, and testing the Multi-Level Gearbox project within the NS-2 environment.

## Prerequisites
- **NS-2 Installation**: Ensure `ns-2.34` is installed.
- **Environment**: NS-2 is located at `/home/yitao.zhou/workspace/ns-allinone-2.34/ns-2.34`.

## 1. Modify Code
- Edit the `.cc` and `.h` files in the repository (e.g., `gearbox-one-level.cc`).
- **Important**: Changes here must be reflected in the NS-2 source tree.
- **Update TCL Script**: If you change queue parameters (e.g., FIFO count), update `tcp-HRCC.tcl` to ensure trace file suffixes match the new configuration (e.g., `1x32`, `2x16`).
- **Weight Modification Rule**: When modifying weights in Gearbox files, **ignore the number of levels** and simply change to the requested weight distribution. Do not proactively add more weights for 4/5 levels (e.g., if requested weights are `{1, 2, 4}`, use exactly these 3 weights for all levels).

## 2. Sync with NS-2
- Create symlinks from the project directory to the NS-2 `queue` directory if not already done.
- Example:
  ```bash
  ln -sf ~/workspace/gearbox/multi-level-gearbox/gearbox-one-level.cc ~/ns-allinone-2.34/ns-2.34/queue/
  ln -sf ~/workspace/gearbox/multi-level-gearbox/gearbox-one-level.h ~/ns-allinone-2.34/ns-2.34/queue/
  ```
- Ensure the `Makefile` in NS-2 includes the new object files (e.g., `gearbox-one-level.o`).

## 3. Compile
- Navigate to the NS-2 directory:
  ```bash
  cd ~/ns-allinone-2.34/ns-2.34
  ```
- Run the build command:
  ```bash
  make clean && make
  ```
- Verify the build was successful (check for errors).

## 4. Test / Run Simulation
- Navigate back to the project root:
  ```bash
  cd ~/workspace/gearbox/multi-level-gearbox
  ```
- Run a simulation using `ns`:
  ```bash
  # Format: ns <script> <flow_num> <load> <topology> <algorithm>
  ns tcp-HRCC.tcl 500 0.5 Topology-8hosts-NSDI21.tcl GearboxOneLevel
  ```
- Alternatively, use the provided scripts:
  ```bash
  bash tcp-HRCC.sh 500 0.5 Topology-8hosts-NSDI21.tcl
  ```

## 5. Verify & Organize Results
- **Check Output**: Verify trace files are generated with correct suffixes (e.g., `*_1x32.tr`).
- **Organize**: Move results to a structured directory to avoid clutter and confusion.
  - Recommended structure: `results/weight/<distribution>/<load>/`
  - Example: `results/weight/1_1_1/0.7/`

## 6. Analyze & Visualize
### 6.1 Check Level Distribution
Use `check_level_distribution.py` to analyze how flows are distributed across different Gearbox levels. This helps verify if the weight configuration is effectively isolating flows.

**Usage:**
```bash
python3 check_level_distribution.py <results_folder> [options]
```

**Parameters:**
- `<results_folder>`: Directory containing the trace files (Required).
- `--files`: Comma-separated list of specific trace files to analyze (Optional).

**Example:**
```bash
python3 check_level_distribution.py results/weight/1_1_4/0.9/
```

### 6.2 Generate FCT Plot
Use the `analyze_fct.py` script to generate FCT (Flow Completion Time) plots for comparison.

**Usage:**
```bash
python3 analyze_fct.py --folder <results_folder> --algs <algo1,algo2> --load <load> [options]
```

**Parameters:**
- `--folder`: Path to the directory containing `.tr` trace files.
- `--algs`: Comma-separated list of algorithms to compare (e.g., `1x32,2x16`).
- `--load`: Load factor (e.g., `0.5`, `0.7`, `0.9`).
- `--topo`: Topology filename (default: `Topology-8hosts-NSDI21.tcl`).
- `--flow_num`: **IMPORTANT**: Must match the flow number used in simulation (e.g., `500`). Defaults to 1000 if not set, which will cause "File not found" errors if your filenames use 500.
- `--output`: Output filename for the plot. **Naming Convention**: `comparison_<TotalQ>Q_<FlowNum>F_<Load>L_<Workload>_<WeightDist>.png`

**Example:**
```bash
python3 analyze_fct.py --folder ./results/weight/1_1_1/0.7/ --algs 1x32,2x16,3x10,4x8,5x6 --load 0.7 --flow_num 500 --output comparison_32Q_500F_0.7L_WebSearch_1-1-1.png
```

## 7. Organize Plots
- **Move Plots**: After generating the plot, **immediately move** the `.png` file from the root directory to the corresponding result directory (e.g., `results/weight/<distribution>/<load>/`).
- Do not leave plot files in the project root.

## Summary of Iterative Workflow
1. **Modify**: Update `.cc`/`.h` (Queue structure) and `.tcl` (suffixes).
2. **Compile**: Rebuild NS-2 (`make clean && make`).
3. **Simulate**: Run `ns` script to generate new results.
4. **Organize**: Move trace files to structured folder (e.g., `results/weight/1_1_1/0.7/`).
5. **Analyze**: Run `check_level_distribution.py` to verify flow distribution.
6. **Visualize**: Run `python3 analyze_fct.py` with correct arguments.
7. **Compare**: Check the generated PNG file.
